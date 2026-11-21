// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbtunnel/core/klb_tunnel_wire.h"

#include <string.h>


int klb_tunnel_encode(uint8_t* p_out, int out_max, uint8_t typ,
    const uint8_t* p_payload, int payload_len)
{
    int need = 0;

    if (NULL == p_out)
    {
        return -1;
    }

    if (payload_len < 0)
    {
        payload_len = 0;
    }

    if (payload_len > 65535)
    {
        return -1;
    }

    if (NULL == p_payload && payload_len > 0)
    {
        return -1;
    }

    need = KLB_TUNNEL_HDR_LEN + payload_len;

    if (out_max < need)
    {
        return -1;
    }

    p_out[0] = KLB_TUNNEL_MAGIC0;
    p_out[1] = KLB_TUNNEL_MAGIC1;
    p_out[2] = KLB_TUNNEL_VER;
    p_out[3] = typ;
    p_out[4] = (uint8_t)((payload_len >> 8) & 0xFF);
    p_out[5] = (uint8_t)(payload_len & 0xFF);

    if (payload_len > 0)
    {
        memcpy(p_out + KLB_TUNNEL_HDR_LEN, p_payload, (size_t)payload_len);
    }

    return need;
}


int klb_tunnel_try_decode(const uint8_t* p_buf, int buf_len, klb_tunnel_frame_t* p_frame)
{
    int payload_len = 0;

    if (NULL == p_buf || NULL == p_frame)
    {
        return -1;
    }

    memset(p_frame, 0, sizeof(*p_frame));

    if (buf_len < KLB_TUNNEL_HDR_LEN)
    {
        return 1;
    }

    if (KLB_TUNNEL_MAGIC0 != p_buf[0] || KLB_TUNNEL_MAGIC1 != p_buf[1])
    {
        return -1;
    }

    if (KLB_TUNNEL_VER != p_buf[2])
    {
        return -1;
    }

    payload_len = ((int)p_buf[4] << 8) | (int)p_buf[5];

    if (buf_len < KLB_TUNNEL_HDR_LEN + payload_len)
    {
        return 1;
    }

    p_frame->ver = p_buf[2];
    p_frame->type = p_buf[3];
    p_frame->payload_len = payload_len;
    p_frame->frame_len = KLB_TUNNEL_HDR_LEN + payload_len;
    p_frame->p_payload = (payload_len > 0) ? (p_buf + KLB_TUNNEL_HDR_LEN) : NULL;
    return 0;
}


int klb_tunnel_encode_connect_payload(uint8_t* p_out, int out_max,
    const char* p_host, int host_len, uint16_t port)
{
    if (NULL == p_out || NULL == p_host || host_len <= 0)
    {
        return -1;
    }

    if (host_len > KLB_TUNNEL_HOST_MAX)
    {
        host_len = KLB_TUNNEL_HOST_MAX;
    }

    if (out_max < 1 + host_len + 2)
    {
        return -1;
    }

    p_out[0] = (uint8_t)host_len;
    memcpy(p_out + 1, p_host, (size_t)host_len);
    p_out[1 + host_len] = (uint8_t)((port >> 8) & 0xFF);
    p_out[2 + host_len] = (uint8_t)(port & 0xFF);
    return 1 + host_len + 2;
}


int klb_tunnel_parse_connect_payload(const uint8_t* p, int len,
    char* p_host, int host_max, uint16_t* p_port)
{
    int n = 0;

    if (NULL == p || NULL == p_host || NULL == p_port || host_max <= 1)
    {
        return -1;
    }

    p_host[0] = '\0';
    *p_port = 0;

    if (len < 3)
    {
        return -1;
    }

    n = (int)p[0];

    if (len < 1 + n + 2 || 0 == n || n >= host_max)
    {
        return -1;
    }

    memcpy(p_host, p + 1, (size_t)n);
    p_host[n] = '\0';
    *p_port = (uint16_t)((p[1 + n] << 8) | p[2 + n]);
    return 0;
}

// end
