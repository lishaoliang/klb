// Doc Encode : UTF-8 BOM, Unix(LF)
// OSTA CS0 compressed Unicode -> UTF-8 (simplified)
#include "udf/pfs_udf_internal.h"
#include "pfs_endian.h"

#include <string.h>

#define PFS_UDF_UNICODE_MAX           0x10FFFFu
#define PFS_UDF_PLANE_SIZE            0x10000u
#define PFS_UDF_SURROGATE_MASK        0xFFFFF800u
#define PFS_UDF_SURROGATE_PAIR        0x0000D800u
#define PFS_UDF_SURROGATE_LOW         0x00000400u
#define PFS_UDF_SURROGATE_CHAR_BITS   10
#define PFS_UDF_SURROGATE_CHAR_MASK   ((1u << PFS_UDF_SURROGATE_CHAR_BITS) - 1u)

static int pfs_udf_utf32_to_utf8(uint32_t cp, char *out, int out_cap)
{
    if (NULL == out || out_cap <= 0)
    {
        return 0;
    }
    if (cp <= 0x7Fu)
    {
        out[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7FFu)
    {
        if (out_cap < 2)
        {
            return -1;
        }
        out[0] = (char)(0xC0u | (cp >> 6));
        out[1] = (char)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp <= 0xFFFFu)
    {
        if (out_cap < 3)
        {
            return -1;
        }
        out[0] = (char)(0xE0u | (cp >> 12));
        out[1] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (char)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    if (cp <= PFS_UDF_UNICODE_MAX)
    {
        if (out_cap < 4)
        {
            return -1;
        }
        out[0] = (char)(0xF0u | (cp >> 18));
        out[1] = (char)(0x80u | ((cp >> 12) & 0x3Fu));
        out[2] = (char)(0x80u | ((cp >> 6) & 0x3Fu));
        out[3] = (char)(0x80u | (cp & 0x3Fu));
        return 4;
    }
    return -1;
}

static int pfs_udf_get_cs0_char(const uint8_t *raw, int raw_len, int idx, int u_ch,
        uint32_t *p_cp, int *p_consumed)
{
    if (NULL == raw || NULL == p_cp || NULL == p_consumed)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (idx >= raw_len)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    int i = idx;
    uint32_t c = raw[i];
    if (u_ch > 1)
    {
        if (i + 1 >= raw_len)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        c = (c << 8) | raw[i + 1];
        i += 2;
    }
    else
    {
        i += 1;
    }

    if (u_ch > 1 && ((c & PFS_UDF_SURROGATE_MASK) == PFS_UDF_SURROGATE_PAIR))
    {
        uint32_t next;
        if (i + 1 >= raw_len)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        if (0 != (c & PFS_UDF_SURROGATE_LOW))
        {
            return PFS_UDF_RET_EILSEQ;
        }
        next = ((uint32_t)raw[i] << 8) | raw[i + 1];
        i += 2;
        if (((next & PFS_UDF_SURROGATE_MASK) != PFS_UDF_SURROGATE_PAIR) ||
                0 == (next & PFS_UDF_SURROGATE_LOW))
        {
            return PFS_UDF_RET_EILSEQ;
        }
        c = PFS_UDF_PLANE_SIZE +
                ((c & PFS_UDF_SURROGATE_CHAR_MASK) << PFS_UDF_SURROGATE_CHAR_BITS) +
                (next & PFS_UDF_SURROGATE_CHAR_MASK);
    }

    if (0 == c || c > PFS_UDF_UNICODE_MAX)
    {
        c = (uint32_t)'?';
    }
    *p_cp = c;
    *p_consumed = i - idx;
    return PFS_UDF_RET_OK;
}

int pfs_udf_unicode_to_utf8(const uint8_t *raw, int raw_len, char *name_out, int name_cap)
{
    if (NULL == raw || raw_len < 0 || NULL == name_out || name_cap <= 1)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == raw_len)
    {
        name_out[0] = '\0';
        return 0;
    }

    uint8_t cmp_id = raw[0];
    if (8 != cmp_id && 16 != cmp_id)
    {
        return PFS_UDF_RET_EILSEQ;
    }
    int u_ch = (int)(cmp_id >> 3);
    raw++;
    raw_len--;
    if (raw_len <= 0 || 0 != (raw_len % u_ch))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    int out_len = 0;
    name_out[0] = '\0';
    int idx = 0;
    while (idx < raw_len)
    {
        uint32_t cp;
        int consumed;
        int n;
        char tmp[4];

        int rc = pfs_udf_get_cs0_char(raw, raw_len, idx, u_ch, &cp, &consumed);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
        idx += consumed;
        if ('/' == cp)
        {
            cp = (uint32_t)'?';
        }
        n = pfs_udf_utf32_to_utf8(cp, tmp, (int)sizeof(tmp));
        if (n < 0)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        if (out_len + n >= name_cap)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        memcpy(name_out + out_len, tmp, (size_t)n);
        out_len += n;
        name_out[out_len] = '\0';
    }
    return out_len;
}

bool pfs_udf_name_equal(const char *a, const char *b)
{
    if (NULL == a || NULL == b)
    {
        return false;
    }
    return (0 == strcmp(a, b));
}

static int pfs_udf_utf8_next_cp(const char *utf8, int *p_i, uint32_t *p_cp)
{
    const unsigned char *p;
    unsigned char c0;
    int i;

    if (NULL == utf8 || NULL == p_i || NULL == p_cp)
    {
        return PFS_UDF_RET_EINVAL;
    }
    p = (const unsigned char *)utf8;
    i = *p_i;
    if ('\0' == p[i])
    {
        return PFS_UDF_RET_EBADMSG;
    }
    c0 = p[i];
    if (c0 <= 0x7Fu)
    {
        *p_cp = c0;
        i++;
        *p_i = i;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xE0u) == 0xC0u)
    {
        if (0 == p[i + 1])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x1Fu) << 6) | (uint32_t)(p[i + 1] & 0x3Fu);
        i += 2;
        *p_i = i;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xF0u) == 0xE0u)
    {
        if (0 == p[i + 1] || 0 == p[i + 2])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x0Fu) << 12) |
                ((uint32_t)(p[i + 1] & 0x3Fu) << 6) |
                (uint32_t)(p[i + 2] & 0x3Fu);
        i += 3;
        *p_i = i;
        return PFS_UDF_RET_OK;
    }
    if ((c0 & 0xF8u) == 0xF0u)
    {
        if (0 == p[i + 1] || 0 == p[i + 2] || 0 == p[i + 3])
        {
            return PFS_UDF_RET_EILSEQ;
        }
        *p_cp = ((uint32_t)(c0 & 0x07u) << 18) |
                ((uint32_t)(p[i + 1] & 0x3Fu) << 12) |
                ((uint32_t)(p[i + 2] & 0x3Fu) << 6) |
                (uint32_t)(p[i + 3] & 0x3Fu);
        i += 4;
        *p_i = i;
        return PFS_UDF_RET_OK;
    }
    return PFS_UDF_RET_EILSEQ;
}

static int pfs_udf_utf8_to_cps(const char *utf8, uint32_t *cps, int cps_cap, int *p_n)
{
    if (NULL == utf8 || NULL == cps || cps_cap <= 0 || NULL == p_n)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int i = 0;
    int n = 0;
    if (0 == utf8[0])
    {
        *p_n = 0;
        return PFS_UDF_RET_OK;
    }
    if ((unsigned char)utf8[0] == 0xEFu &&
            (unsigned char)utf8[1] == 0xBBu &&
            (unsigned char)utf8[2] == 0xBFu)
    {
        i = 3;
    }
    while ('\0' != utf8[i])
    {
        uint32_t cp;
        if (n >= cps_cap)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        int rc = pfs_udf_utf8_next_cp(utf8, &i, &cp);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
        if (0xFEFFu == cp)
        {
            continue;
        }
        cps[n] = cp;
        n++;
    }
    *p_n = n;
    return PFS_UDF_RET_OK;
}

int pfs_udf_utf8_to_cs0(const char *utf8, uint8_t *out, int out_cap, int *p_out_len)
{
    if (NULL == utf8 || NULL == out || out_cap <= 1 || NULL == p_out_len)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint32_t cps[128];
    int n_cp = 0;
    int rc = pfs_udf_utf8_to_cps(utf8, cps, (int)(sizeof(cps) / sizeof(cps[0])), &n_cp);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    bool use16 = false;
    for (int i = 0; i < n_cp; i++)
    {
        if (cps[i] >= 0x100u)
        {
            use16 = true;
            break;
        }
    }
    if (!use16)
    {
        if (1 + n_cp > out_cap)
        {
            return PFS_UDF_RET_ENAMETOOLONG;
        }
        out[0] = 8;
        for (int i = 0; i < n_cp; i++)
        {
            out[1 + i] = (uint8_t)cps[i];
        }
        *p_out_len = 1 + n_cp;
        return PFS_UDF_RET_OK;
    }
    if (1 + n_cp * 2 > out_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    out[0] = 16;
    int o = 1;
    for (int i = 0; i < n_cp; i++)
    {
        pfs_write_be16(out + o, (uint16_t)cps[i]);
        o += 2;
    }
    *p_out_len = o;
    return PFS_UDF_RET_OK;
}

// end
