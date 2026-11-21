// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsocks5/core/klb_socks5_wire.h"

#include <stdio.h>
#include <string.h>


static int is_ipv4_host_(const char* p_host, int host_len, uint8_t* p_octets)
{
    int i = 0;
    int part = 0;
    int val = 0;
    int digits = 0;

    if (NULL == p_host || host_len <= 0 || NULL == p_octets)
    {
        return 0;
    }

    for (i = 0; i < host_len; ++i)
    {
        char ch = p_host[i];

        if ('.' == ch)
        {
            if (0 == digits || val > 255 || part >= 3)
            {
                return 0;
            }

            p_octets[part] = (uint8_t)val;
            part += 1;
            val = 0;
            digits = 0;
        }
        else if (ch >= '0' && ch <= '9')
        {
            val = val * 10 + (ch - '0');
            digits += 1;

            if (digits > 3 || val > 255)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    if (0 == digits || val > 255 || 3 != part)
    {
        return 0;
    }

    p_octets[part] = (uint8_t)val;
    return 1;
}


static int parse_addr_(const uint8_t* p, int len, uint8_t atyp,
    char* p_host, int host_max, uint16_t* p_port, int* p_consumed)
{
    int need = 0;

    if (NULL == p || NULL == p_host || NULL == p_port || NULL == p_consumed)
    {
        return -1;
    }

    p_host[0] = '\0';
    *p_port = 0;
    *p_consumed = 0;

    if (KLB_SOCKS5_ATYP_IPV4 == atyp)
    {
        need = 4 + 2;

        if (len < need)
        {
            return 1;
        }

        if (host_max < 16)
        {
            return -1;
        }

        // dotted IPv4
        {
            int n = 0;
            n = snprintf(p_host, (size_t)host_max, "%u.%u.%u.%u",
                (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[3]);

            if (n <= 0 || n >= host_max)
            {
                return -1;
            }
        }

        *p_port = (uint16_t)((p[4] << 8) | p[5]);
        *p_consumed = need;
        return 0;
    }
    else if (KLB_SOCKS5_ATYP_DOMAIN == atyp)
    {
        uint8_t n = 0;

        if (len < 1)
        {
            return 1;
        }

        n = p[0];
        need = 1 + (int)n + 2;

        if (len < need)
        {
            return 1;
        }

        if (0 == n || (int)n >= host_max)
        {
            return -1;
        }

        memcpy(p_host, p + 1, (size_t)n);
        p_host[n] = '\0';
        *p_port = (uint16_t)((p[1 + n] << 8) | p[2 + n]);
        *p_consumed = need;
        return 0;
    }
    else if (KLB_SOCKS5_ATYP_IPV6 == atyp)
    {
        int i = 0;
        int pos = 0;

        need = 16 + 2;

        if (len < need)
        {
            return 1;
        }

        // simplified hex groups (matches script path)
        for (i = 0; i < 16; i += 2)
        {
            int n = 0;

            if (0 != i)
            {
                if (pos + 1 >= host_max)
                {
                    return -1;
                }

                p_host[pos++] = ':';
            }

            n = snprintf(p_host + pos, (size_t)(host_max - pos), "%02x%02x",
                (unsigned)p[i], (unsigned)p[i + 1]);

            if (n <= 0 || pos + n >= host_max)
            {
                return -1;
            }

            pos += n;
        }

        p_host[pos] = '\0';
        *p_port = (uint16_t)((p[16] << 8) | p[17]);
        *p_consumed = need;
        return 0;
    }

    return -1;
}


int klb_socks5_encode_greeting(uint8_t* p_out, int out_max, int with_userpass)
{
    if (NULL == p_out)
    {
        return -1;
    }

    if (0 != with_userpass)
    {
        if (out_max < 4)
        {
            return -1;
        }

        p_out[0] = KLB_SOCKS5_VER;
        p_out[1] = 0x02;
        p_out[2] = KLB_SOCKS5_METHOD_NONE;
        p_out[3] = KLB_SOCKS5_METHOD_USERPASS;
        return 4;
    }

    if (out_max < 3)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_VER;
    p_out[1] = 0x01;
    p_out[2] = KLB_SOCKS5_METHOD_NONE;
    return 3;
}


int klb_socks5_parse_method_select(const uint8_t* p, int len, uint8_t* p_method)
{
    if (NULL == p || NULL == p_method)
    {
        return -1;
    }

    if (len < 2)
    {
        return 1;
    }

    if (KLB_SOCKS5_VER != p[0])
    {
        return -1;
    }

    *p_method = p[1];
    return 0;
}


int klb_socks5_encode_userpass(uint8_t* p_out, int out_max,
    const char* p_user, int user_len, const char* p_pass, int pass_len)
{
    int need = 0;

    if (NULL == p_out)
    {
        return -1;
    }

    if (user_len < 0)
    {
        user_len = 0;
    }

    if (pass_len < 0)
    {
        pass_len = 0;
    }

    if (user_len > 255 || pass_len > 255)
    {
        return -1;
    }

    if (NULL == p_user && user_len > 0)
    {
        return -1;
    }

    if (NULL == p_pass && pass_len > 0)
    {
        return -1;
    }

    need = 1 + 1 + user_len + 1 + pass_len;

    if (out_max < need)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_AUTH_VER;
    p_out[1] = (uint8_t)user_len;

    if (user_len > 0)
    {
        memcpy(p_out + 2, p_user, (size_t)user_len);
    }

    p_out[2 + user_len] = (uint8_t)pass_len;

    if (pass_len > 0)
    {
        memcpy(p_out + 3 + user_len, p_pass, (size_t)pass_len);
    }

    return need;
}


int klb_socks5_parse_userpass_status(const uint8_t* p, int len, uint8_t* p_status)
{
    if (NULL == p || NULL == p_status)
    {
        return -1;
    }

    if (len < 2)
    {
        return 1;
    }

    if (KLB_SOCKS5_AUTH_VER != p[0])
    {
        return -1;
    }

    *p_status = p[1];
    return 0;
}


int klb_socks5_encode_connect(uint8_t* p_out, int out_max,
    const char* p_host, int host_len, uint16_t port)
{
    uint8_t octets[4];
    int off = 0;

    if (NULL == p_out || NULL == p_host || host_len <= 0)
    {
        return -1;
    }

    if (out_max < 4)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_VER;
    p_out[1] = KLB_SOCKS5_CMD_CONNECT;
    p_out[2] = 0x00;
    off = 3;

    if (0 != is_ipv4_host_(p_host, host_len, octets))
    {
        if (out_max < off + 1 + 4 + 2)
        {
            return -1;
        }

        p_out[off++] = KLB_SOCKS5_ATYP_IPV4;
        memcpy(p_out + off, octets, 4);
        off += 4;
    }
    else
    {
        if (host_len > 255)
        {
            return -1;
        }

        if (out_max < off + 1 + 1 + host_len + 2)
        {
            return -1;
        }

        p_out[off++] = KLB_SOCKS5_ATYP_DOMAIN;
        p_out[off++] = (uint8_t)host_len;
        memcpy(p_out + off, p_host, (size_t)host_len);
        off += host_len;
    }

    p_out[off++] = (uint8_t)((port >> 8) & 0xFF);
    p_out[off++] = (uint8_t)(port & 0xFF);
    return off;
}


int klb_socks5_encode_method_select(uint8_t* p_out, int out_max, uint8_t method)
{
    if (NULL == p_out || out_max < 2)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_VER;
    p_out[1] = method;
    return 2;
}


int klb_socks5_parse_greeting(const uint8_t* p, int len,
    uint8_t* p_methods, int methods_max, int* p_nmethods, int* p_consumed)
{
    int nmethods = 0;

    if (NULL == p || NULL == p_nmethods || NULL == p_consumed)
    {
        return -1;
    }

    *p_nmethods = 0;
    *p_consumed = 0;

    if (len < 2)
    {
        return 1;
    }

    if (KLB_SOCKS5_VER != p[0])
    {
        return -1;
    }

    nmethods = (int)p[1];

    if (len < 2 + nmethods)
    {
        return 1;
    }

    if (NULL != p_methods)
    {
        int copy_n = nmethods;

        if (copy_n > methods_max)
        {
            copy_n = methods_max;
        }

        if (copy_n > 0)
        {
            memcpy(p_methods, p + 2, (size_t)copy_n);
        }
    }

    *p_nmethods = nmethods;
    *p_consumed = 2 + nmethods;
    return 0;
}


uint8_t klb_socks5_choose_method(const uint8_t* p_methods, int nmethods, int want_userpass)
{
    int i = 0;
    int has_none = 0;
    int has_user = 0;

    if (NULL == p_methods || nmethods <= 0)
    {
        return KLB_SOCKS5_METHOD_NO_ACCEPT;
    }

    for (i = 0; i < nmethods; ++i)
    {
        if (KLB_SOCKS5_METHOD_NONE == p_methods[i])
        {
            has_none = 1;
        }
        else if (KLB_SOCKS5_METHOD_USERPASS == p_methods[i])
        {
            has_user = 1;
        }
    }

    if (0 != want_userpass)
    {
        if (0 != has_user)
        {
            return KLB_SOCKS5_METHOD_USERPASS;
        }

        return KLB_SOCKS5_METHOD_NO_ACCEPT;
    }

    if (0 != has_none)
    {
        return KLB_SOCKS5_METHOD_NONE;
    }

    return KLB_SOCKS5_METHOD_NO_ACCEPT;
}


int klb_socks5_parse_userpass_req(const uint8_t* p, int len,
    char* p_user, int user_max, int* p_user_len,
    char* p_pass, int pass_max, int* p_pass_len, int* p_consumed)
{
    int ulen = 0;
    int plen = 0;
    int need = 0;

    if (NULL == p || NULL == p_user || NULL == p_pass
        || NULL == p_user_len || NULL == p_pass_len || NULL == p_consumed)
    {
        return -1;
    }

    *p_user_len = 0;
    *p_pass_len = 0;
    *p_consumed = 0;
    p_user[0] = '\0';
    p_pass[0] = '\0';

    if (len < 2)
    {
        return 1;
    }

    if (KLB_SOCKS5_AUTH_VER != p[0])
    {
        return -1;
    }

    ulen = (int)p[1];
    need = 2 + ulen + 1;

    if (len < need)
    {
        return 1;
    }

    plen = (int)p[2 + ulen];
    need = 2 + ulen + 1 + plen;

    if (len < need)
    {
        return 1;
    }

    if (ulen >= user_max || plen >= pass_max)
    {
        return -1;
    }

    if (ulen > 0)
    {
        memcpy(p_user, p + 2, (size_t)ulen);
    }

    p_user[ulen] = '\0';

    if (plen > 0)
    {
        memcpy(p_pass, p + 3 + ulen, (size_t)plen);
    }

    p_pass[plen] = '\0';
    *p_user_len = ulen;
    *p_pass_len = plen;
    *p_consumed = need;
    return 0;
}


int klb_socks5_encode_userpass_status(uint8_t* p_out, int out_max, uint8_t status)
{
    if (NULL == p_out || out_max < 2)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_AUTH_VER;
    p_out[1] = status;
    return 2;
}


int klb_socks5_parse_request(const uint8_t* p, int len, klb_socks5_request_t* p_req)
{
    int rc = 0;
    int addr_consumed = 0;

    if (NULL == p || NULL == p_req)
    {
        return -1;
    }

    memset(p_req, 0, sizeof(*p_req));

    if (len < 4)
    {
        return 1;
    }

    if (KLB_SOCKS5_VER != p[0])
    {
        return -1;
    }

    p_req->cmd = p[1];
    p_req->atyp = p[3];

    rc = parse_addr_(p + 4, len - 4, p_req->atyp,
        p_req->host, (int)sizeof(p_req->host), &p_req->port, &addr_consumed);

    if (0 != rc)
    {
        return rc;
    }

    p_req->consumed = 4 + addr_consumed;
    return 0;
}


int klb_socks5_parse_reply(const uint8_t* p, int len, klb_socks5_reply_t* p_rsp)
{
    int rc = 0;
    int addr_consumed = 0;

    if (NULL == p || NULL == p_rsp)
    {
        return -1;
    }

    memset(p_rsp, 0, sizeof(*p_rsp));

    if (len < 4)
    {
        return 1;
    }

    if (KLB_SOCKS5_VER != p[0])
    {
        return -1;
    }

    p_rsp->rep = p[1];
    p_rsp->atyp = p[3];

    rc = parse_addr_(p + 4, len - 4, p_rsp->atyp,
        p_rsp->host, (int)sizeof(p_rsp->host), &p_rsp->port, &addr_consumed);

    if (0 != rc)
    {
        return rc;
    }

    p_rsp->consumed = 4 + addr_consumed;
    return 0;
}


int klb_socks5_encode_reply(uint8_t* p_out, int out_max, uint8_t rep)
{
    if (NULL == p_out || out_max < 10)
    {
        return -1;
    }

    p_out[0] = KLB_SOCKS5_VER;
    p_out[1] = rep;
    p_out[2] = 0x00;
    p_out[3] = KLB_SOCKS5_ATYP_IPV4;
    p_out[4] = 0;
    p_out[5] = 0;
    p_out[6] = 0;
    p_out[7] = 0;
    p_out[8] = 0;
    p_out[9] = 0;
    return 10;
}

// end
