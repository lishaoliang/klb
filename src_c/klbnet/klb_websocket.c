// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_websocket.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/// @brief 读大端流16位, uint16_t
#ifndef KLB_RB16
#   define KLB_RB16(x)                            \
        ((((const uint8_t*)(x))[0] << 8) |        \
        ((const uint8_t*)(x))[1])
#endif

/// @brief 读大端流32位, uint32_t
#ifndef KLB_RB32
#   define KLB_RB32(x)                              \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |   \
     (((const uint8_t*)(x))[1] << 16) |             \
     (((const uint8_t*)(x))[2] <<  8) |             \
     ((const uint8_t*)(x))[3])
#endif

/// @brief 读大端流64位, uint64_t
#ifndef KLB_RB64
#   define KLB_RB64(x)                                  \
    ( ((uint64_t)(((const uint8_t*)(x))[0]) << 56) |    \
      ((uint64_t)(((const uint8_t*)(x))[1]) << 48) |    \
      ((uint64_t)(((const uint8_t*)(x))[2]) << 40) |    \
      ((uint64_t)(((const uint8_t*)(x))[3]) << 32) |    \
      ((uint64_t)(((const uint8_t*)(x))[4]) << 24) |    \
      ((uint64_t)(((const uint8_t*)(x))[5]) << 16) |    \
      ((uint64_t)(((const uint8_t*)(x))[6]) <<  8) |    \
      ((uint64_t)((const uint8_t*)(x))[7]) )
#endif

/// @brief 写 uint16_t, 大端流16位
#ifndef KLB_WB16
#   define KLB_WB16(p, darg) do {               \
        int16_t d = (darg);                     \
        ((uint8_t*)(p))[1] = (d);               \
        ((uint8_t*)(p))[0] = (d)>>8;            \
    } while(0)
#endif

/// @brief 写 uint32_t, 大端流32位
#ifndef KLB_WB32
#   define KLB_WB32(p, darg) do {                \
        uint32_t d = (darg);                     \
        ((uint8_t*)(p))[3] = (d);                \
        ((uint8_t*)(p))[2] = (d)>>8;             \
        ((uint8_t*)(p))[1] = (d)>>16;            \
        ((uint8_t*)(p))[0] = (d)>>24;            \
    } while(0)
#endif

/// @brief 写 uint64_t, 大端流64位
#ifndef KLB_WB64
#   define KLB_WB64(p, darg) do {                \
        uint64_t d = (darg);                     \
        ((uint8_t*)(p))[7] = (d);                \
        ((uint8_t*)(p))[6] = (d)>>8;             \
        ((uint8_t*)(p))[5] = (d)>>16;            \
        ((uint8_t*)(p))[4] = (d)>>24;            \
        ((uint8_t*)(p))[3] = (d)>>32;            \
        ((uint8_t*)(p))[2] = (d)>>40;            \
        ((uint8_t*)(p))[1] = (d)>>48;            \
        ((uint8_t*)(p))[0] = (d)>>56;            \
    } while(0)
#endif


int klb_websocket_parse(klb_websocket_t* p_ws, const uint8_t* p_data, int data_len)
{
    if (data_len < KLB_WEBSOCKET_HEAD_MIN) return 1;

    klb_websocket_t ws = { 0 };

    // 大端字节序
    uint8_t b0 = p_data[0];
    ws.fin = (b0 & 0x80) >> 7;
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

    ptr[pos] = ((uint8_t)(0x1) << 7) | (opcode & 0xF);          pos += 1;
    ptr[pos] = ((mask & 0x1) << 7) | (payload_len & 0x7F);      pos += 1;

    if (127 == payload_len)
    {
        KLB_WB64(ptr + pos, data_len);                           pos += 8;
    }
    else if (126 == payload_len)
    {
        KLB_WB16(ptr + pos, data_len);                           pos += 2;
    }

    // mask
    if (0 != mask)
    {
        memcpy(ptr + pos, p_mask_key, 4);                       pos += 4;
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
