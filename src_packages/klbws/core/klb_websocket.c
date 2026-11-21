// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbws/core/klb_websocket.h"
#include "klbplatform/klb_time.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int klb_websocket_parse(klb_websocket_t* p_ws, const uint8_t* p_data, int data_len)
{
    if (data_len < KLB_WEBSOCKET_HEAD_MIN) return 1;

    klb_websocket_t ws = { 0 };

    // 大端字节序
    uint8_t b0 = p_data[0];
    ws.fin = (b0 & 0x80) >> 7;
    ws.rsv1 = (b0 & 0x40) >> 6;
    ws.rsv2 = (b0 & 0x20) >> 5;
    ws.rsv3 = (b0 & 0x10) >> 4;
    ws.opcode = b0 & 0xF;

    uint8_t b1 = p_data[1];
    ws.mask = (b1 & 0x80) >> 7;
    uint8_t payload_len = b1 & 0x7F;

    int pos = 2;
    if (126 == payload_len)
    {
        if (data_len < pos + 2) return 1;

        ws.payload_len = KLB_RB16(p_data + pos);  pos += 2;
    }
    else if (127 == payload_len)
    {
        if (data_len < pos + 8) return 1;

        ws.payload_len = KLB_RB64(p_data + pos);  pos += 8;
    }
    else
    {
        ws.payload_len = payload_len;
    }

    if (0 != ws.mask)
    {
        if (data_len < pos + 4) return 1;

        memcpy(ws.mask_key, p_data + pos, 4);
        pos += 4;
    }

    ws.head_len = pos;
    assert(KLB_WEBSOCKET_HEAD_MIN <= pos);
    assert(pos <= KLB_WEBSOCKET_HEAD_MAX);

    memcpy(p_ws, &ws, sizeof(klb_websocket_t));
    return 0;
}

void klb_websocket_mask(const uint8_t* p_mask_key, uint8_t* p_data, int data_len)
{
    for (int i = 0; i < data_len; i++)
    {
        p_data[i] ^= p_mask_key[i & 0x3];
    }
}

klb_buf_t* klb_websocket_pack_fin(uint8_t opcode, const uint8_t* p_mask_key, const char* p_data, int data_len)
{
    klb_buf_t* p_buf = klb_buf_malloc(data_len + KLB_WEBSOCKET_HEAD_MAX, false);

    uint8_t mask = (NULL == p_mask_key) ? 0 : 0x1;

    uint8_t* ptr = (uint8_t*)p_buf->p_buf;
    int pos = 0;

    uint8_t payload_len = 0;
    if (65535 < data_len)
    {
        payload_len = 127;
    }
    else if (125 < data_len)
    {
        payload_len = 126;
    }
    else
    {
        payload_len = data_len;
    }

    ptr[pos] = ((uint8_t)(0x1) << 7) | (opcode & 0xF);      pos += 1;
    ptr[pos] = ((mask & 0x1) << 7) | (payload_len & 0x7F);  pos += 1;

    if (127 == payload_len)
    {
        KLB_WB64(ptr + pos, data_len);                      pos += 8;
    }
    else if (126 == payload_len)
    {
        KLB_WB16(ptr + pos, data_len);                      pos += 2;
    }

    // mask
    if (0 != mask)
    {
        memcpy(ptr + pos, p_mask_key, 4);                   pos += 4;
    }

    // copy data
    if (0 < data_len)
    {
        memcpy(ptr + pos, p_data, data_len);

        if (0 != mask)
        {
            klb_websocket_mask(p_mask_key, ptr + pos, data_len);
        }

        pos += data_len;
    }

    p_buf->end = pos;
    assert(p_buf->end <= p_buf->buf_len);

    return p_buf;
}


//////////////////////////////////////////////////////////////////////////
// 握手: SHA1 / Base64 / Accept

typedef struct klb_ws_sha1_t_
{
    uint32_t                    state[5];
    uint64_t                    nbits;
    uint8_t                     buffer[64];
    int                         nbuf;
}klb_ws_sha1_t;


static uint32_t rotl32_klb_websocket(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static void sha1_block_klb_websocket(uint32_t state[5], const uint8_t block[64])
{
    uint32_t w[80];
    for (int i = 0; i < 16; i++)
    {
        w[i] = ((uint32_t)block[i * 4] << 24) |
            ((uint32_t)block[i * 4 + 1] << 16) |
            ((uint32_t)block[i * 4 + 2] << 8) |
            ((uint32_t)block[i * 4 + 3]);
    }

    for (int i = 16; i < 80; i++)
    {
        uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
        w[i] = rotl32_klb_websocket(x, 1);
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    for (int i = 0; i < 80; i++)
    {
        uint32_t f = 0;
        uint32_t k = 0;
        if (20 > i)
        {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        }
        else if (40 > i)
        {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        }
        else if (60 > i)
        {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        }
        else
        {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }

        uint32_t temp = rotl32_klb_websocket(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl32_klb_websocket(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

static void sha1_init_klb_websocket(klb_ws_sha1_t* p_sha)
{
    p_sha->state[0] = 0x67452301u;
    p_sha->state[1] = 0xEFCDAB89u;
    p_sha->state[2] = 0x98BADCFEu;
    p_sha->state[3] = 0x10325476u;
    p_sha->state[4] = 0xC3D2E1F0u;
    p_sha->nbits = 0;
    p_sha->nbuf = 0;
}

static void sha1_update_klb_websocket(klb_ws_sha1_t* p_sha, const uint8_t* p_data, int data_len)
{
    for (int i = 0; i < data_len; i++)
    {
        p_sha->buffer[p_sha->nbuf] = p_data[i];
        p_sha->nbuf += 1;
        p_sha->nbits += 8;
        if (64 == p_sha->nbuf)
        {
            sha1_block_klb_websocket(p_sha->state, p_sha->buffer);
            p_sha->nbuf = 0;
        }
    }
}

static void sha1_final_klb_websocket(klb_ws_sha1_t* p_sha, uint8_t out[20])
{
    uint64_t bits = p_sha->nbits;
    uint8_t one = 0x80;
    sha1_update_klb_websocket(p_sha, &one, 1);

    uint8_t zero = 0;
    while (56 != p_sha->nbuf)
    {
        sha1_update_klb_websocket(p_sha, &zero, 1);
    }

    uint8_t lenb[8];
    for (int i = 0; i < 8; i++)
    {
        lenb[7 - i] = (uint8_t)((bits >> (i * 8)) & 0xFFu);
    }

    sha1_update_klb_websocket(p_sha, lenb, 8);

    for (int i = 0; i < 5; i++)
    {
        out[i * 4] = (uint8_t)(p_sha->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(p_sha->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(p_sha->state[i] >> 8);
        out[i * 4 + 3] = (uint8_t)(p_sha->state[i]);
    }
}

void klb_websocket_random(uint8_t* p_out, int n)
{
    if (NULL == p_out || 0 >= n)
    {
        return;
    }

    static uint32_t s_state = 0;
    if (0 == s_state)
    {
        s_state = klb_tick_count() ^ 0xA5A5u;
        if (0 == s_state)
        {
            s_state = 0x12345678u;
        }
    }

    for (int i = 0; i < n; i++)
    {
        s_state = s_state * 1664525u + 1013904223u;
        p_out[i] = (uint8_t)(s_state >> 24);
    }
}

int klb_websocket_b64(const uint8_t* p_in, int in_len, char* p_out, int out_cap)
{
    static const char s_b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (NULL == p_in || NULL == p_out || 0 > in_len)
    {
        return 1;
    }

    int need = ((in_len + 2) / 3) * 4;
    if (out_cap < need + 1)
    {
        return 1;
    }

    int i = 0;
    int j = 0;
    while (i + 3 <= in_len)
    {
        uint32_t v = ((uint32_t)p_in[i] << 16) |
            ((uint32_t)p_in[i + 1] << 8) |
            (uint32_t)p_in[i + 2];
        p_out[j] = s_b64[(v >> 18) & 63];
        j += 1;
        p_out[j] = s_b64[(v >> 12) & 63];
        j += 1;
        p_out[j] = s_b64[(v >> 6) & 63];
        j += 1;
        p_out[j] = s_b64[v & 63];
        j += 1;
        i += 3;
    }

    if (i < in_len)
    {
        uint32_t v = ((uint32_t)p_in[i] << 16);
        if (i + 1 < in_len)
        {
            v |= ((uint32_t)p_in[i + 1] << 8);
        }

        p_out[j] = s_b64[(v >> 18) & 63];
        j += 1;
        p_out[j] = s_b64[(v >> 12) & 63];
        j += 1;
        if (i + 1 < in_len)
        {
            p_out[j] = s_b64[(v >> 6) & 63];
            j += 1;
            p_out[j] = '=';
            j += 1;
        }
        else
        {
            p_out[j] = '=';
            j += 1;
            p_out[j] = '=';
            j += 1;
        }
    }

    p_out[j] = '\0';
    return 0;
}

int klb_websocket_accept(const char* p_key, char* p_out, int out_cap)
{
    static const char s_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    if (NULL == p_key || NULL == p_out || 29 > out_cap)
    {
        return 1;
    }

    klb_ws_sha1_t sha;
    sha1_init_klb_websocket(&sha);
    sha1_update_klb_websocket(&sha, (const uint8_t*)p_key, (int)strlen(p_key));
    sha1_update_klb_websocket(&sha, (const uint8_t*)s_guid, (int)(sizeof(s_guid) - 1));

    uint8_t dig[20];
    sha1_final_klb_websocket(&sha, dig);
    return klb_websocket_b64(dig, 20, p_out, out_cap);
}

void klb_websocket_mask_key(uint8_t key[4])
{
    klb_websocket_random(key, 4);
}

static char ascii_lower_klb_websocket(char c)
{
    if ('A' <= c && 'Z' >= c)
    {
        return (char)(c - 'A' + 'a');
    }

    return c;
}

bool klb_websocket_header_get(const char* p_header, const char* p_name, char* p_out, int out_cap)
{
    if (NULL == p_header || NULL == p_name || NULL == p_out || 2 > out_cap)
    {
        return false;
    }

    p_out[0] = '\0';
    int name_len = (int)strlen(p_name);
    if (0 >= name_len)
    {
        return false;
    }

    const char* p = p_header;
    while ('\0' != *p)
    {
        bool match = true;
        for (int i = 0; i < name_len; i++)
        {
            if ('\0' == p[i] || '\n' == p[i] || '\r' == p[i])
            {
                match = false;
                break;
            }

            if (ascii_lower_klb_websocket(p[i]) != ascii_lower_klb_websocket(p_name[i]))
            {
                match = false;
                break;
            }
        }

        if (match && ':' == p[name_len])
        {
            p += name_len + 1;
            while (' ' == *p || '\t' == *p)
            {
                p += 1;
            }

            int n = 0;
            while ('\0' != *p && '\r' != *p && '\n' != *p)
            {
                if (n >= out_cap - 1)
                {
                    p_out[0] = '\0';
                    return false;
                }

                p_out[n] = *p;
                n += 1;
                p += 1;
            }

            while (0 < n && (' ' == p_out[n - 1] || '\t' == p_out[n - 1]))
            {
                n -= 1;
            }

            p_out[n] = '\0';
            return 0 < n;
        }

        while ('\0' != *p && '\n' != *p)
        {
            p += 1;
        }

        if ('\n' == *p)
        {
            p += 1;
        }
    }

    return false;
}

bool klb_websocket_header_has(const char* p_header, const char* p_name, const char* p_token)
{
    char value[256];
    if (!klb_websocket_header_get(p_header, p_name, value, (int)sizeof(value)))
    {
        return false;
    }

    if (NULL == p_token || '\0' == p_token[0])
    {
        return false;
    }

    int token_len = (int)strlen(p_token);
    for (int i = 0; '\0' != value[i]; i++)
    {
        int j = 0;
        while (j < token_len && '\0' != value[i + j] &&
            ascii_lower_klb_websocket(value[i + j]) == ascii_lower_klb_websocket(p_token[j]))
        {
            j += 1;
        }

        if (j == token_len)
        {
            return true;
        }
    }

    return false;
}

bool klb_websocket_frame_ok(const klb_websocket_t* p_ws, bool from_client)
{
    if (NULL == p_ws)
    {
        return false;
    }

    if (0 != p_ws->rsv1 || 0 != p_ws->rsv2 || 0 != p_ws->rsv3)
    {
        return false;
    }

    uint8_t opcode = p_ws->opcode;
    bool ctrl = (KLB_WEBSOCKET_OPCODE_CLOSE == opcode ||
        KLB_WEBSOCKET_OPCODE_PING == opcode ||
        KLB_WEBSOCKET_OPCODE_PONG == opcode);
    bool data = (KLB_WEBSOCKET_OPCODE_CONTINUATION == opcode ||
        KLB_WEBSOCKET_OPCODE_TEXT == opcode ||
        KLB_WEBSOCKET_OPCODE_BINARY == opcode);

    if (!ctrl && !data)
    {
        return false;
    }

    if (ctrl)
    {
        if (0 == p_ws->fin || 125 < p_ws->payload_len)
        {
            return false;
        }
    }
    else if ((uint64_t)KLB_WEBSOCKET_MAX_PAYLOAD < p_ws->payload_len)
    {
        return false;
    }

    if (from_client)
    {
        if (0 == p_ws->mask)
        {
            return false;
        }
    }
    else
    {
        if (0 != p_ws->mask)
        {
            return false;
        }
    }

    return true;
}

bool klb_websocket_utf8_ok(const uint8_t* p_data, int data_len)
{
    if (NULL == p_data || 0 > data_len)
    {
        return false;
    }

    int i = 0;
    while (i < data_len)
    {
        uint8_t c = p_data[i];
        if (0x80 > c)
        {
            i += 1;
            continue;
        }

        int need = 0;
        if (0xC2 <= c && 0xDF >= c)
        {
            need = 2;
        }
        else if (0xE0 <= c && 0xEF >= c)
        {
            need = 3;
        }
        else if (0xF0 <= c && 0xF4 >= c)
        {
            need = 4;
        }
        else
        {
            return false;
        }

        if (i + need > data_len)
        {
            return false;
        }

        for (int k = 1; k < need; ++k)
        {
            if (0x80 != (p_data[i + k] & 0xC0))
            {
                return false;
            }
        }

        if (3 == need && 0xE0 == c && 0xA0 > p_data[i + 1])
        {
            return false;
        }

        if (3 == need && 0xED == c && 0x9F < p_data[i + 1])
        {
            return false;
        }

        if (4 == need && 0xF0 == c && 0x90 > p_data[i + 1])
        {
            return false;
        }

        if (4 == need && 0xF4 == c && 0x8F < p_data[i + 1])
        {
            return false;
        }

        i += need;
    }

    return true;
}

bool klb_websocket_close_code_ok(int code)
{
    if (1000 <= code && 1003 >= code)
    {
        return true;
    }

    if (1007 <= code && 1014 >= code)
    {
        return true;
    }

    if (3000 <= code && 4999 >= code)
    {
        return true;
    }

    return false;
}

bool klb_websocket_close_ok(const uint8_t* p_data, int data_len, int* p_code)
{
    if (NULL == p_code)
    {
        return false;
    }

    if (NULL == p_data || 0 == data_len)
    {
        *p_code = KLB_WEBSOCKET_CLOSE_NOSTATUS;
        return true;
    }

    if (2 > data_len)
    {
        return false;
    }

    int code = (int)KLB_RB16(p_data);
    if (!klb_websocket_close_code_ok(code))
    {
        return false;
    }

    if (!klb_websocket_utf8_ok(p_data + 2, data_len - 2))
    {
        return false;
    }

    *p_code = code;
    return true;
}

bool klb_websocket_protocol_has(const char* p_list, const char* p_token)
{
    if (NULL == p_list || NULL == p_token || '\0' == p_token[0])
    {
        return false;
    }

    int token_len = (int)strlen(p_token);
    const char* p = p_list;
    while ('\0' != *p)
    {
        while (' ' == *p || '\t' == *p)
        {
            p += 1;
        }

        const char* start = p;
        while ('\0' != *p && ',' != *p)
        {
            p += 1;
        }

        const char* end = p;
        while (end > start && (' ' == *(end - 1) || '\t' == *(end - 1)))
        {
            end -= 1;
        }

        int n = (int)(end - start);
        if (n == token_len && 0 == memcmp(start, p_token, (size_t)n))
        {
            return true;
        }

        if (',' == *p)
        {
            p += 1;
        }
    }

    return false;
}

// end
