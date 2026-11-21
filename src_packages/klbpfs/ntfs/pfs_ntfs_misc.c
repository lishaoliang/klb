// Doc Encode : UTF-8 BOM, Unix(LF)
// 辅助与杂项 (linux-7.1 fs/ntfs unistr.c)
#include "ntfs/pfs_ntfs_internal.h"

#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

static const uint8_t g_pfs_ntfs_legal_ansi_char_array[0x40] = {
    0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x17, 0x07, 0x18, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x17, 0x18, 0x16, 0x16, 0x17, 0x07, 0x00,
    0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x17, 0x04, 0x16, 0x18, 0x16, 0x18, 0x18,
};

static int pfs_ntfs_utf8_encode(uint32_t cp, char *out, int out_cap)
{
    if (NULL == out || out_cap <= 0)
    {
        return -1;
    }
    if (cp < 0x80u)
    {
        if (out_cap < 1)
        {
            return -1;
        }
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
    return -1;
}

static int pfs_ntfs_utf8_decode_one(const char *s, int len, int *p_consumed, uint32_t *p_cp)
{
    if (NULL == s || len <= 0 || NULL == p_consumed || NULL == p_cp)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    const uint8_t *p = (const uint8_t *)s;
    if (p[0] < 0x80u)
    {
        *p_cp = p[0];
        *p_consumed = 1;
        return PFS_NTFS_RET_OK;
    }

    uint32_t cp;
    int need;
    if ((p[0] & 0xE0u) == 0xC0u)
    {
        need = 2;
        cp = p[0] & 0x1Fu;
    }
    else if ((p[0] & 0xF0u) == 0xE0u)
    {
        need = 3;
        cp = p[0] & 0x0Fu;
    }
    else if ((p[0] & 0xF8u) == 0xF0u)
    {
        return PFS_NTFS_RET_EILSEQ;
    }
    else
    {
        return PFS_NTFS_RET_EILSEQ;
    }

    if (len < need)
    {
        return PFS_NTFS_RET_EILSEQ;
    }

    for (int i = 1; i < need; i++)
    {
        if ((p[i] & 0xC0u) != 0x80u)
        {
            return PFS_NTFS_RET_EILSEQ;
        }
        cp = (cp << 6) | (p[i] & 0x3Fu);
    }

    if (cp <= 0xFFFFu)
    {
        *p_cp = cp;
        *p_consumed = need;
        return PFS_NTFS_RET_OK;
    }

    return PFS_NTFS_RET_EILSEQ;
}

int pfs_ntfs_utf16_to_utf8(const uint16_t *p_uni, uint32_t uni_len, char *out, int out_cap)
{
    if (NULL == p_uni || NULL == out || out_cap <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    int out_len = 0;
    for (uint32_t i = 0; i < uni_len; i++)
    {
        uint16_t ch = p_uni[i];
        char seq[4];
        int n;

        if (0U == ch)
        {
            break;
        }

        n = pfs_ntfs_utf8_encode(ch, seq, (int)sizeof(seq));
        if (0 > n || out_len + n >= out_cap)
        {
            return PFS_NTFS_RET_ERANGE;
        }

        memcpy(out + out_len, seq, (size_t)n);
        out_len += n;
    }

    out[out_len] = '\0';
    return out_len;
}

int pfs_ntfs_utf8_to_uni(pfs_ntfs_sbi_t *p_sbi, const char *utf8, int len, pfs_ntfs_uni_name_t *p_uni)
{
    int pos = 0;

    (void)p_sbi;

    if (NULL == utf8 || NULL == p_uni)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (len < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 == len)
    {
        len = (int)strlen(utf8);
    }

    p_uni->len = 0;
    while (pos < len && '\0' != utf8[pos])
    {
        int consumed;
        uint32_t cp;

        if (p_uni->len >= NTFS_MAXIMUM_FILE_NAME_LENGTH)
        {
            return PFS_NTFS_RET_ENAMETOOLONG;
        }

        int rc = pfs_ntfs_utf8_decode_one(utf8 + pos, len - pos, &consumed, &cp);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        p_uni->name[p_uni->len++] = (uint16_t)cp;
        pos += consumed;
    }

    return PFS_NTFS_RET_OK;
}

uint16_t pfs_ntfs_upcase_char(const pfs_ntfs_upcase_t *p_up, uint16_t ch)
{
    if (NULL == p_up || !p_up->valid || NULL == p_up->p_entries)
    {
        return ch;
    }
    if ((uint32_t)ch < p_up->entries_count)
    {
        return p_up->p_entries[ch];
    }
    return ch;
}

// 名可同时充当 DOS 8.3 时用 WIN32_AND_DOS (与 mkfs 系统名一致; 避免 CHKDSK 报索引项不正确)
int pfs_ntfs_uni_is_dos_compatible(const pfs_ntfs_uni_name_t *p_uni)
{
    if (NULL == p_uni || 0U == p_uni->len || p_uni->len > 12U)
    {
        return 0;
    }

    uint32_t base_len = 0;
    uint32_t ext_len = 0;
    int seen_dot = 0;

    for (uint32_t i = 0; i < p_uni->len; i++)
    {
        uint16_t c = p_uni->name[i];

        if (c > 0x7FU)
        {
            return 0;
        }
        if ('.' == (char)c)
        {
            if (0 != seen_dot || 0U == base_len)
            {
                return 0;
            }
            seen_dot = 1;
            continue;
        }
        // DOS 文件名允许: A-Z a-z 0-9 与部分符号; 禁空格与 " * + , / : ; < = > ? [ \ ] |
        if ((c >= (uint16_t)'A' && c <= (uint16_t)'Z') ||
                (c >= (uint16_t)'a' && c <= (uint16_t)'z') ||
                (c >= (uint16_t)'0' && c <= (uint16_t)'9') ||
                '$' == (char)c || '%' == (char)c || '\'' == (char)c || '-' == (char)c ||
                '_' == (char)c || '@' == (char)c || '~' == (char)c || '`' == (char)c ||
                '!' == (char)c || '(' == (char)c || ')' == (char)c || '{' == (char)c ||
                '}' == (char)c || '^' == (char)c || '#' == (char)c || '&' == (char)c)
        {
            if (0 == seen_dot)
            {
                base_len++;
                if (base_len > 8U)
                {
                    return 0;
                }
            }
            else
            {
                ext_len++;
                if (ext_len > 3U)
                {
                    return 0;
                }
            }
            continue;
        }
        return 0;
    }

    if (0U == base_len)
    {
        return 0;
    }
    return 1;
}

static int pfs_ntfs_dos_char_ok(uint16_t c)
{
    if ((c >= (uint16_t)'A' && c <= (uint16_t)'Z') ||
            (c >= (uint16_t)'0' && c <= (uint16_t)'9') ||
            '$' == (char)c || '%' == (char)c || '\'' == (char)c || '-' == (char)c ||
            '_' == (char)c || '@' == (char)c || '~' == (char)c || '`' == (char)c ||
            '!' == (char)c || '(' == (char)c || ')' == (char)c || '{' == (char)c ||
            '}' == (char)c || '^' == (char)c || '#' == (char)c || '&' == (char)c)
    {
        return 1;
    }
    return 0;
}

// 映射为 DOS 可用大写字符; 0=跳过 (空格等); 1=写入 *p_out
static int pfs_ntfs_dos_map_char(const pfs_ntfs_upcase_t *p_up, uint16_t c, uint16_t *p_out)
{
    if (NULL == p_out)
    {
        return 0;
    }
    if (' ' == (char)c || '.' == (char)c)
    {
        return 0;
    }
    uint16_t u = pfs_ntfs_upcase_char(p_up, c);
    if (u >= (uint16_t)'a' && u <= (uint16_t)'z')
    {
        u = (uint16_t)(u - (uint16_t)'a' + (uint16_t)'A');
    }
    if (pfs_ntfs_dos_char_ok(u))
    {
        *p_out = u;
        return 1;
    }
    // 非法字符变下划线 (对齐 Windows 短名惯例)
    *p_out = (uint16_t)'_';
    return 1;
}

static uint32_t pfs_ntfs_dos_num_digits(uint32_t n)
{
    uint32_t d = 1U;

    while (n >= 10U)
    {
        n /= 10U;
        d++;
    }
    return d;
}

static void pfs_ntfs_dos_write_num(uint16_t *p_dst, uint32_t n, uint32_t digits)
{

    for (uint32_t i = digits; i > 0U; i--)
    {
        p_dst[i - 1U] = (uint16_t)((uint16_t)'0' + (uint16_t)(n % 10U));
        n /= 10U;
    }
}

// 长名 -> 唯一 DOS 8.3 (大写 + ~n); 供 WIN32 旁路第二条 $FILE_NAME / $I30
int pfs_ntfs_uni_make_dos_name(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_long, pfs_ntfs_uni_name_t *p_dos)
{
    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_long || NULL == p_dos || 0U == p_long->len)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    const pfs_ntfs_upcase_t *p_up = &p_sbi->upcase;
    uint16_t base[8];
    uint16_t ext[3];
    uint32_t base_len = 0U;
    uint32_t ext_len = 0U;
    uint32_t last_dot;
    uint32_t start;
    uint16_t mapped;
    int has_dot = 0;

    memset(base, 0, sizeof(base));
    memset(ext, 0, sizeof(ext));
    memset(p_dos, 0, sizeof(*p_dos));

    // 去前导 '.'
    start = 0U;
    while (start < p_long->len && '.' == (char)p_long->name[start])
    {
        start++;
    }
    if (start >= p_long->len)
    {
        base[0] = (uint16_t)'_';
        base_len = 1U;
    }
    else
    {
        last_dot = p_long->len;
        for (uint32_t i = start; i < p_long->len; i++)
        {
            if ('.' == (char)p_long->name[i])
            {
                last_dot = i;
                has_dot = 1;
            }
        }

        for (int i = start; i < (has_dot ? last_dot : p_long->len); i++)
        {
            if (0 == pfs_ntfs_dos_map_char(p_up, p_long->name[i], &mapped))
            {
                continue;
            }
            if (base_len < 8U)
            {
                base[base_len++] = mapped;
            }
        }
        if (0 != has_dot)
        {
            for (int i = last_dot + 1U; i < p_long->len; i++)
            {
                if (0 == pfs_ntfs_dos_map_char(p_up, p_long->name[i], &mapped))
                {
                    continue;
                }
                if (ext_len < 3U)
                {
                    ext[ext_len++] = mapped;
                }
            }
        }
        if (0U == base_len)
        {
            base[0] = (uint16_t)'_';
            base_len = 1U;
        }
    }

    // ~1 .. ~999999 直至父目录无同名 (含已有 DOS 项)
    for (uint32_t n = 1U; n < 1000000U; n++)
    {
        uint32_t ndig = pfs_ntfs_dos_num_digits(n);
        uint32_t keep;
        uint32_t pos = 0U;
        int rc_has;

        if (ndig >= 8U)
        {
            return PFS_NTFS_RET_ENOSPC;
        }
        keep = 8U - 1U - ndig;
        if (keep > base_len)
        {
            keep = base_len;
        }
        if (0U == keep)
        {
            keep = 1U;
            if (keep > base_len)
            {
                keep = base_len;
            }
        }

        for (int i = 0U; i < keep; i++)
        {
            p_dos->name[pos++] = base[i];
        }
        p_dos->name[pos++] = (uint16_t)'~';
        pfs_ntfs_dos_write_num(p_dos->name + pos, n, ndig);
        pos += ndig;
        if (ext_len > 0U)
        {
            p_dos->name[pos++] = (uint16_t)'.';
            for (int i = 0U; i < ext_len; i++)
            {
                p_dos->name[pos++] = ext[i];
            }
        }
        p_dos->len = pos;

        rc_has = pfs_ntfs_index_has_filename(p_sbi, p_dir_ino, p_dos);
        if (PFS_NTFS_RET_ENOENT == rc_has)
        {
            return PFS_NTFS_RET_OK;
        }
        if (PFS_NTFS_RET_OK == rc_has)
        {
            continue;
        }
        return rc_has;
    }
    return PFS_NTFS_RET_ENOSPC;
}

int pfs_ntfs_are_names_equal(const uint16_t *s1, uint32_t s1_len, const uint16_t *s2, uint32_t s2_len,
        int ic, const pfs_ntfs_upcase_t *p_up)
{

    if (NULL == s1 || NULL == s2)
    {
        return 0;
    }
    if (s1_len != s2_len)
    {
        return 0;
    }
    if (PFS_NTFS_CASE_SENSITIVE == ic)
    {
        for (uint32_t i = 0; i < s1_len; i++)
        {
            if (s1[i] != s2[i])
            {
                return 0;
            }
        }
        return 1;
    }

    if (NULL == p_up || !p_up->valid)
    {
        return 0;
    }
    for (int i = 0; i < s1_len; i++)
    {
        if (pfs_ntfs_upcase_char(p_up, s1[i]) != pfs_ntfs_upcase_char(p_up, s2[i]))
        {
            return 0;
        }
    }
    return 1;
}

int pfs_ntfs_collate_names(const uint16_t *name1, uint32_t name1_len, const uint16_t *name2,
        uint32_t name2_len, int err_val, int ic, const pfs_ntfs_upcase_t *p_up)
{
    if (NULL == name1 || NULL == name2)
    {
        return err_val;
    }

    uint32_t min_len = name1_len;
    if (name1_len > name2_len)
    {
        min_len = name2_len;
    }

    for (uint32_t cnt = 0; cnt < min_len; cnt++)
    {
        uint16_t c1 = name1[cnt];
        uint16_t c2 = name2[cnt];
        if (PFS_NTFS_IGNORE_CASE == ic)
        {
            if (NULL != p_up && p_up->valid)
            {
                c1 = pfs_ntfs_upcase_char(p_up, c1);
                c2 = pfs_ntfs_upcase_char(p_up, c2);
            }
        }
        if (c1 < 64U && 0U != (g_pfs_ntfs_legal_ansi_char_array[c1] & 8U))
        {
            return err_val;
        }
        if (c1 < c2)
        {
            return -1;
        }
        if (c1 > c2)
        {
            return 1;
        }
    }

    if (name1_len < name2_len)
    {
        return -1;
    }
    if (name1_len == name2_len)
    {
        return 0;
    }

    uint16_t c1 = name1[name1_len];
    if (c1 < 64U && 0U != (g_pfs_ntfs_legal_ansi_char_array[c1] & 8U))
    {
        return err_val;
    }
    return 1;
}

uint64_t pfs_ntfs_current_time_le(void)
{
#if defined(_WIN32) || defined(_WIN64)
    FILETIME ft;
    ULARGE_INTEGER uli;

    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return pfs_cpu_to_le64(uli.QuadPart);
#else
    // 固定非零值: 2020-01-01 00:00:00 UTC (1601 纪元)
    return pfs_cpu_to_le64(132120578000000000ULL);
#endif
}

// end
