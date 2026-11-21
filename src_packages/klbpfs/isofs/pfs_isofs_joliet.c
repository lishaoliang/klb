// Doc Encode : UTF-8 BOM, Unix(LF)
// Joliet 文件名: UTF-16BE -> UTF-8 (对照 linux fs/isofs/joliet.c)
#include "isofs/pfs_isofs_internal.h"

#include <string.h>

static int pfs_isofs_utf16be_to_utf8(const uint8_t *src, int nchars, char *dst, int dst_cap)
{
    if (NULL == src || NULL == dst || 1 >= dst_cap || 0 > nchars)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    int out = 0;
    for (int i = 0; i < nchars; i++)
    {
        uint16_t ch = ((uint16_t)src[0] << 8) | (uint16_t)src[1];
        src += 2;
        if (0 == ch)
        {
            break;
        }
        if (ch < 0x80u)
        {
            if (out + 1 >= dst_cap)
            {
                break;
            }
            dst[out] = (char)ch;
            out++;
        }
        else if (ch < 0x800u)
        {
            if (out + 2 >= dst_cap)
            {
                break;
            }
            dst[out] = (char)(0xC0u | (ch >> 6));
            out++;
            dst[out] = (char)(0x80u | (ch & 0x3Fu));
            out++;
        }
        else
        {
            if (out + 3 >= dst_cap)
            {
                break;
            }
            dst[out] = (char)(0xE0u | (ch >> 12));
            out++;
            dst[out] = (char)(0x80u | ((ch >> 6) & 0x3Fu));
            out++;
            dst[out] = (char)(0x80u | (ch & 0x3Fu));
            out++;
        }
    }
    dst[out] = '\0';
    return out;
}

int pfs_isofs_joliet_name(const char *raw, int raw_bytes, char *name_out, int name_cap)
{
    if (NULL == raw || 0 > raw_bytes || NULL == name_out || 1 >= name_cap)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    int nchars = raw_bytes >> 1;
    int len = pfs_isofs_utf16be_to_utf8((const uint8_t *)raw, nchars, name_out, name_cap);
    if (0 > len)
    {
        return len;
    }
    if (len > 2 && ';' == name_out[len - 2] && '1' == name_out[len - 1])
    {
        len -= 2;
        name_out[len] = '\0';
    }
    while (len >= 1 && '.' == name_out[len - 1])
    {
        len--;
        name_out[len] = '\0';
    }
    return len;
}

// end
