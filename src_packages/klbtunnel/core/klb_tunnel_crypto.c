// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbtunnel/core/klb_tunnel_crypto.h"
#include "klbmem/klb_mem.h"

#include <stdint.h>
#include <string.h>
#include <time.h>


//////////////////////////////////////////////////////////////////////////
// Compact SHA-256 (public-domain style; matches script crypto.lua)


static uint32_t rotr32_(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32 - n));
}


static void sha256_transform_(uint32_t* p_h, const uint8_t* p_chunk)
{
    static const uint32_t k[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
    };

    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i = 0;

    for (i = 0; i < 16; ++i)
    {
        w[i] = ((uint32_t)p_chunk[i * 4] << 24)
            | ((uint32_t)p_chunk[i * 4 + 1] << 16)
            | ((uint32_t)p_chunk[i * 4 + 2] << 8)
            | ((uint32_t)p_chunk[i * 4 + 3]);
    }

    for (i = 16; i < 64; ++i)
    {
        uint32_t s0 = rotr32_(w[i - 15], 7) ^ rotr32_(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr32_(w[i - 2], 17) ^ rotr32_(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = p_h[0];
    b = p_h[1];
    c = p_h[2];
    d = p_h[3];
    e = p_h[4];
    f = p_h[5];
    g = p_h[6];
    h = p_h[7];

    for (i = 0; i < 64; ++i)
    {
        uint32_t S1 = rotr32_(e, 6) ^ rotr32_(e, 11) ^ rotr32_(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + S1 + ch + k[i] + w[i];
        uint32_t S0 = rotr32_(a, 2) ^ rotr32_(a, 13) ^ rotr32_(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    p_h[0] += a;
    p_h[1] += b;
    p_h[2] += c;
    p_h[3] += d;
    p_h[4] += e;
    p_h[5] += f;
    p_h[6] += g;
    p_h[7] += h;
}


void klb_tunnel_sha256(const uint8_t* p_msg, int msg_len, uint8_t* p_out)
{
    uint32_t h[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    uint8_t chunk[64];
    uint64_t bit_len = 0;
    int off = 0;
    int i = 0;

    if (NULL == p_out)
    {
        return;
    }

    if (NULL == p_msg)
    {
        msg_len = 0;
    }

    if (msg_len < 0)
    {
        msg_len = 0;
    }

    bit_len = (uint64_t)msg_len * 8ull;

    while (off + 64 <= msg_len)
    {
        sha256_transform_(h, p_msg + off);
        off += 64;
    }

    memset(chunk, 0, sizeof(chunk));

    if (msg_len > off)
    {
        memcpy(chunk, p_msg + off, (size_t)(msg_len - off));
    }

    chunk[msg_len - off] = 0x80;

    if ((msg_len - off) >= 56)
    {
        sha256_transform_(h, chunk);
        memset(chunk, 0, sizeof(chunk));
    }

    chunk[56] = (uint8_t)((bit_len >> 56) & 0xFF);
    chunk[57] = (uint8_t)((bit_len >> 48) & 0xFF);
    chunk[58] = (uint8_t)((bit_len >> 40) & 0xFF);
    chunk[59] = (uint8_t)((bit_len >> 32) & 0xFF);
    chunk[60] = (uint8_t)((bit_len >> 24) & 0xFF);
    chunk[61] = (uint8_t)((bit_len >> 16) & 0xFF);
    chunk[62] = (uint8_t)((bit_len >> 8) & 0xFF);
    chunk[63] = (uint8_t)(bit_len & 0xFF);
    sha256_transform_(h, chunk);

    for (i = 0; i < 8; ++i)
    {
        p_out[i * 4] = (uint8_t)((h[i] >> 24) & 0xFF);
        p_out[i * 4 + 1] = (uint8_t)((h[i] >> 16) & 0xFF);
        p_out[i * 4 + 2] = (uint8_t)((h[i] >> 8) & 0xFF);
        p_out[i * 4 + 3] = (uint8_t)(h[i] & 0xFF);
    }
}


static void put_u32be_(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8) & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}


static void keystream_block_(const uint8_t* p_key32, uint32_t seq, uint32_t block_i, uint8_t* p_out32)
{
    uint8_t msg[32 + 1 + 4 + 1 + 4];
    int off = 0;

    memcpy(msg, p_key32, 32);
    off = 32;
    msg[off++] = '|';
    put_u32be_(msg + off, seq);
    off += 4;
    msg[off++] = '|';
    put_u32be_(msg + off, block_i);
    off += 4;
    klb_tunnel_sha256(msg, off, p_out32);
}


static void xor_stream_(const uint8_t* p_key32, uint32_t seq,
    const uint8_t* p_in, int in_len, uint8_t* p_out)
{
    uint8_t ks[32];
    uint32_t bi = 0;
    int i = 0;
    int j = 0;

    while (i < in_len)
    {
        keystream_block_(p_key32, seq, bi, ks);
        bi += 1;

        for (j = 0; j < 32 && i < in_len; ++j, ++i)
        {
            p_out[i] = (uint8_t)(p_in[i] ^ ks[j]);
        }
    }
}


// EtM 标签: SHA256(SHA256("alua-tunnel-aead-mac|" + key) + "|" + seq + "|" + ct)[0..15]
static void aead_tag_(const uint8_t* p_key32, uint32_t seq,
    const uint8_t* p_ct, int ct_len, uint8_t* p_tag16)
{
    static const char prefix[] = "alua-tunnel-aead-mac|";
    uint8_t mac_key[KLB_TUNNEL_SHA256_LEN];
    uint8_t mac_msg[sizeof(prefix) - 1 + KLB_TUNNEL_SHA256_LEN];
    uint8_t* p_msg = NULL;
    uint8_t dig[KLB_TUNNEL_SHA256_LEN];
    int prefix_len = (int)(sizeof(prefix) - 1);
    int need = 0;
    int off = 0;

    if (NULL == p_tag16)
    {
        return;
    }

    memcpy(mac_msg, prefix, (size_t)prefix_len);
    memcpy(mac_msg + prefix_len, p_key32, KLB_TUNNEL_SHA256_LEN);
    klb_tunnel_sha256(mac_msg, prefix_len + KLB_TUNNEL_SHA256_LEN, mac_key);

    need = KLB_TUNNEL_SHA256_LEN + 1 + 4 + 1 + ct_len;
    p_msg = (uint8_t*)klb_malloc((size_t)need);

    memcpy(p_msg, mac_key, KLB_TUNNEL_SHA256_LEN);
    off = KLB_TUNNEL_SHA256_LEN;
    p_msg[off++] = '|';
    put_u32be_(p_msg + off, seq);
    off += 4;
    p_msg[off++] = '|';

    if (ct_len > 0)
    {
        memcpy(p_msg + off, p_ct, (size_t)ct_len);
        off += ct_len;
    }

    klb_tunnel_sha256(p_msg, off, dig);
    memcpy(p_tag16, dig, KLB_TUNNEL_TAG_LEN);
    KLB_FREE(p_msg);
}


void klb_tunnel_derive_session_key(const uint8_t* p_psk, int psk_len,
    const uint8_t* p_nc, int nc_len, const uint8_t* p_ns, int ns_len, uint8_t* p_out32)
{
    static const char prefix[] = "alua-tunnel-sess|";
    uint8_t buf[512];
    int off = 0;
    int prefix_len = (int)(sizeof(prefix) - 1);

    if (NULL == p_out32)
    {
        return;
    }

    if (NULL == p_psk)
    {
        psk_len = 0;
    }

    if (NULL == p_nc)
    {
        nc_len = 0;
    }

    if (NULL == p_ns)
    {
        ns_len = 0;
    }

    if (psk_len < 0)
    {
        psk_len = 0;
    }

    if (nc_len < 0)
    {
        nc_len = 0;
    }

    if (ns_len < 0)
    {
        ns_len = 0;
    }

    if (prefix_len + psk_len + 1 + nc_len + 1 + ns_len > (int)sizeof(buf))
    {
        // truncate psk to fit
        int room = (int)sizeof(buf) - prefix_len - 2 - nc_len - ns_len;

        if (room < 0)
        {
            room = 0;
        }

        if (psk_len > room)
        {
            psk_len = room;
        }
    }

    memcpy(buf + off, prefix, (size_t)prefix_len);
    off += prefix_len;

    if (psk_len > 0)
    {
        memcpy(buf + off, p_psk, (size_t)psk_len);
        off += psk_len;
    }

    buf[off++] = '|';

    if (nc_len > 0)
    {
        memcpy(buf + off, p_nc, (size_t)nc_len);
        off += nc_len;
    }

    buf[off++] = '|';

    if (ns_len > 0)
    {
        memcpy(buf + off, p_ns, (size_t)ns_len);
        off += ns_len;
    }

    klb_tunnel_sha256(buf, off, p_out32);
}


int klb_tunnel_seal(uint8_t* p_out, int out_max, const uint8_t* p_key32,
    uint32_t seq, const uint8_t* p_plain, int plain_len)
{
    if (NULL == p_out || NULL == p_key32)
    {
        return -1;
    }

    if (plain_len < 0)
    {
        plain_len = 0;
    }

    if (NULL == p_plain && plain_len > 0)
    {
        return -1;
    }

    if (out_max < 4 + plain_len + KLB_TUNNEL_TAG_LEN)
    {
        return -1;
    }

    put_u32be_(p_out, seq);

    if (plain_len > 0)
    {
        xor_stream_(p_key32, seq, p_plain, plain_len, p_out + 4);
    }

    aead_tag_(p_key32, seq, p_out + 4, plain_len, p_out + 4 + plain_len);

    return 4 + plain_len + KLB_TUNNEL_TAG_LEN;
}


int klb_tunnel_open(uint8_t* p_out, int out_max, const uint8_t* p_key32,
    const uint8_t* p_blob, int blob_len, uint32_t* p_seq)
{
    uint32_t seq = 0;
    int ct_len = 0;
    uint8_t tag_expect[KLB_TUNNEL_TAG_LEN];

    if (NULL == p_out || NULL == p_key32 || NULL == p_blob
        || blob_len < 4 + KLB_TUNNEL_TAG_LEN)
    {
        return -1;
    }

    seq = ((uint32_t)p_blob[0] << 24)
        | ((uint32_t)p_blob[1] << 16)
        | ((uint32_t)p_blob[2] << 8)
        | ((uint32_t)p_blob[3]);

    ct_len = blob_len - 4 - KLB_TUNNEL_TAG_LEN;

    if (out_max < ct_len)
    {
        return -1;
    }

    aead_tag_(p_key32, seq, p_blob + 4, ct_len, tag_expect);

    if (0 != memcmp(tag_expect, p_blob + 4 + ct_len, KLB_TUNNEL_TAG_LEN))
    {
        return -1;
    }

    if (ct_len > 0)
    {
        xor_stream_(p_key32, seq, p_blob + 4, ct_len, p_out);
    }

    if (NULL != p_seq)
    {
        *p_seq = seq;
    }

    return ct_len;
}


int klb_tunnel_random_nonce(uint8_t* p_out, int out_len)
{
    uint8_t dig[32];
    uint8_t seed[64];
    int off = 0;
    int produced = 0;
    time_t now = 0;
    clock_t clk = 0;
    uintptr_t ptr = 0;

    if (NULL == p_out || out_len <= 0)
    {
        return -1;
    }

    now = time(NULL);
    clk = clock();
    ptr = (uintptr_t)p_out;

    put_u32be_(seed + off, (uint32_t)now);
    off += 4;
    put_u32be_(seed + off, (uint32_t)clk);
    off += 4;
    put_u32be_(seed + off, (uint32_t)(ptr & 0xFFFFFFFFu));
    off += 4;
    put_u32be_(seed + off, (uint32_t)((ptr >> 16) & 0xFFFFFFFFu));
    off += 4;

    klb_tunnel_sha256(seed, off, dig);

    while (produced < out_len)
    {
        int take = 32;

        if (take > out_len - produced)
        {
            take = out_len - produced;
        }

        memcpy(p_out + produced, dig, (size_t)take);
        produced += take;
        klb_tunnel_sha256(dig, 32, dig);
    }

    return out_len;
}


int klb_tunnel_encode_hello_crypto(uint8_t* p_out, int out_max,
    const uint8_t* p_token, int token_len, const uint8_t* p_nonce_c)
{
    int need = 0;

    if (NULL == p_out || NULL == p_nonce_c)
    {
        return -1;
    }

    if (token_len < 0)
    {
        token_len = 0;
    }

    if (NULL == p_token && token_len > 0)
    {
        return -1;
    }

    if (token_len > 65535)
    {
        return -1;
    }

    need = 1 + 2 + token_len + KLB_TUNNEL_NONCE_LEN;

    if (out_max < need)
    {
        return -1;
    }

    p_out[0] = KLB_TUNNEL_FLAG_CRYPTO;
    p_out[1] = (uint8_t)((token_len >> 8) & 0xFF);
    p_out[2] = (uint8_t)(token_len & 0xFF);

    if (token_len > 0)
    {
        memcpy(p_out + 3, p_token, (size_t)token_len);
    }

    memcpy(p_out + 3 + token_len, p_nonce_c, KLB_TUNNEL_NONCE_LEN);
    return need;
}


int klb_tunnel_parse_hello_payload(const uint8_t* p, int len,
    int* p_crypto, char* p_token, int token_max, int* p_token_len,
    uint8_t* p_nonce_c)
{
    if (NULL == p_crypto || NULL == p_token || NULL == p_token_len)
    {
        return -1;
    }

    *p_crypto = 0;
    *p_token_len = 0;
    p_token[0] = '\0';

    if (NULL == p)
    {
        len = 0;
    }

    if (len >= 1 + 2 + KLB_TUNNEL_NONCE_LEN && NULL != p && KLB_TUNNEL_FLAG_CRYPTO == p[0])
    {
        int tlen = ((int)p[1] << 8) | (int)p[2];

        if (len < 3 + tlen + KLB_TUNNEL_NONCE_LEN)
        {
            return -1;
        }

        if (tlen >= token_max)
        {
            return -1;
        }

        if (tlen > 0)
        {
            memcpy(p_token, p + 3, (size_t)tlen);
        }

        p_token[tlen] = '\0';
        *p_token_len = tlen;
        *p_crypto = 1;

        if (NULL != p_nonce_c)
        {
            memcpy(p_nonce_c, p + 3 + tlen, KLB_TUNNEL_NONCE_LEN);
        }

        return 0;
    }

    if (len >= token_max)
    {
        return -1;
    }

    if (len > 0 && NULL != p)
    {
        memcpy(p_token, p, (size_t)len);
    }

    p_token[len] = '\0';
    *p_token_len = len;
    return 0;
}


int klb_tunnel_encode_hello_ok_crypto(uint8_t* p_out, int out_max, const uint8_t* p_nonce_s)
{
    if (NULL == p_out || NULL == p_nonce_s || out_max < 1 + KLB_TUNNEL_NONCE_LEN)
    {
        return -1;
    }

    p_out[0] = KLB_TUNNEL_FLAG_CRYPTO;
    memcpy(p_out + 1, p_nonce_s, KLB_TUNNEL_NONCE_LEN);
    return 1 + KLB_TUNNEL_NONCE_LEN;
}


int klb_tunnel_parse_hello_ok_payload(const uint8_t* p, int len,
    int* p_crypto, uint8_t* p_nonce_s)
{
    if (NULL == p_crypto)
    {
        return -1;
    }

    *p_crypto = 0;

    if (NULL == p)
    {
        len = 0;
    }

    if (len >= 1 + KLB_TUNNEL_NONCE_LEN && NULL != p && KLB_TUNNEL_FLAG_CRYPTO == p[0])
    {
        *p_crypto = 1;

        if (NULL != p_nonce_s)
        {
            memcpy(p_nonce_s, p + 1, KLB_TUNNEL_NONCE_LEN);
        }

        return 0;
    }

    return 0;
}

// end
