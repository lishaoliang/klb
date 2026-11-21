// Doc Encode : UTF-8 BOM, Unix(LF)
// 路径与 lookup/create/mkdir/unlink/rmdir/rename (linux-7.1 fs/fat/namei_vfat.c)
#include "fat/pfs_fat_internal.h"

#include <stdio.h>
#include <string.h>

void pfs_fat_fill_root_info(pfs_fat_sbi_t *p_sbi, pfs_fat_dir_entry_t *p_info)
{
    const pfs_fat_bs_t *p_bs;

    if (NULL == p_sbi || NULL == p_info)
    {
        return;
    }

    p_bs = PFS_FAT_SBI_BS(p_sbi);
    memset(p_info, 0, sizeof(*p_info));
    p_info->type = PFS_FAT_TYPE_DIR;
    p_info->attr = FAT_ATTR_DIR;
    p_info->size = 0;
    p_info->is_root = 1;
    p_info->sfn_cpos = -1;
    p_info->parent_clu = 0;
    p_info->parent_fixed_root = 0;

    if (pfs_fat_is_fat32(p_sbi))
    {
        p_info->start_clu = p_bs->root_cluster;
        p_info->is_fixed_root = 0;
    }
    else
    {
        // FAT16/12: 固定根目录扇区区
        p_info->start_clu = 0;
        p_info->is_fixed_root = 1;
    }
}

int pfs_fat_lookup_path(pfs_fat_sbi_t *p_sbi, const char *path, pfs_fat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == path || NULL == p_info)
    {
        return PFS_FAT_RET_EINVAL;
    }

    const char *p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        pfs_fat_fill_root_info(p_sbi, p_info);
        return PFS_FAT_RET_OK;
    }

    pfs_fat_dir_entry_t cur;
    pfs_fat_fill_root_info(p_sbi, &cur);
    uint32_t dir_clu = cur.start_clu;
    int is_fixed_root = cur.is_fixed_root;
    int have_cur = 0;

    pfs_fat_dir_entry_t found;
    char comp[FAT_LFN_LEN + 1];

    while ('\0' != *p)
    {
        int comp_len = 0;
        while ('\0' != *p && '/' != *p)
        {
            if (comp_len >= FAT_LFN_LEN)
            {
                return PFS_FAT_RET_ENAMETOOLONG;
            }
            comp[comp_len++] = *p++;
        }
        comp[comp_len] = '\0';

        while ('/' == *p)
        {
            p++;
        }
        int is_last = ('\0' == *p) ? 1 : 0;

        if (0 == comp_len || (1 == comp_len && '.' == comp[0]))
        {
            if (is_last)
            {
                if (have_cur)
                {
                    *p_info = cur;
                }
                else
                {
                    pfs_fat_fill_root_info(p_sbi, p_info);
                }
                return PFS_FAT_RET_OK;
            }
            continue;
        }
        if (2 == comp_len && '.' == comp[0] && '.' == comp[1])
        {
            return PFS_FAT_RET_EOPNOTSUPP;
        }

        int rc = pfs_fat_find_dir_entry(p_sbi, dir_clu, is_fixed_root, comp, &found);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        have_cur = 1;
        cur = found;
        cur.is_root = 0;
        cur.is_fixed_root = 0;

        if (is_last)
        {
            *p_info = cur;
            return PFS_FAT_RET_OK;
        }

        if (PFS_FAT_TYPE_DIR != cur.type)
        {
            return PFS_FAT_RET_ENOENT;
        }

        dir_clu = cur.start_clu;
        is_fixed_root = 0;
        if (0 == dir_clu || !pfs_fat_is_valid_cluster(p_sbi, dir_clu))
        {
            return PFS_FAT_RET_EBADMSG;
        }
    }

    pfs_fat_fill_root_info(p_sbi, p_info);
    return PFS_FAT_RET_OK;
}

static int pfs_fat_utf8_to_utf16(const char *utf8, int len, uint16_t *out, int out_cap)
{
    int i = 0;
    int o = 0;

    if (NULL == utf8 || NULL == out || out_cap <= 0 || len < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    while (i < len && o + 1 < out_cap)
    {
        unsigned char c = (unsigned char)utf8[i];
        uint32_t cp;

        if (c < 0x80u)
        {
            cp = c;
            i++;
        }
        else if (c < 0xE0u && i + 1 < len)
        {
            cp = ((uint32_t)(c & 0x1Fu) << 6) | ((uint32_t)utf8[i + 1] & 0x3Fu);
            i += 2;
        }
        else if (c < 0xF0u && i + 2 < len)
        {
            cp = ((uint32_t)(c & 0x0Fu) << 12) | (((uint32_t)utf8[i + 1] & 0x3Fu) << 6) |
                    ((uint32_t)utf8[i + 2] & 0x3Fu);
            i += 3;
        }
        else
        {
            return PFS_FAT_RET_EILSEQ;
        }
        if (cp > 0xFFFFu)
        {
            return PFS_FAT_RET_EILSEQ;
        }
        out[o++] = (uint16_t)cp;
    }
    if (i < len)
    {
        return PFS_FAT_RET_ENAMETOOLONG;
    }
    out[o] = 0;
    return o;
}

static char pfs_fat_to_upper_ascii(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static bool pfs_fat_is_short_char(char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
            c == '\'' || c == '(' || c == ')' || c == '-' || c == '@' ||
            c == '^' || c == '_' || c == '`' || c == '{' || c == '}' || c == '~')
    {
        return true;
    }
    return false;
}

// 从 UTF-8 抽出 8.3 基底 (大写短名字符); *p_lossy=1 表示无法无损映射为纯 8.3
static void pfs_fat_build_short_basis(const char *utf8, uint8_t base[8], int *p_base_len,
        uint8_t ext[3], int *p_ext_len, int *p_lossy)
{
    const char *dot;
    int base_n = 0;
    int ext_n = 0;
    int lossy = 0;

    memset(base, ' ', 8);
    memset(ext, ' ', 3);
    *p_base_len = 0;
    *p_ext_len = 0;
    *p_lossy = 0;

    if (NULL == utf8)
    {
        base[0] = '_';
        *p_base_len = 1;
        *p_lossy = 1;
        return;
    }

    dot = strrchr(utf8, '.');
    if (NULL != dot && dot != utf8 && '\0' != dot[1])
    {
        for (int i = 0; utf8 + i < dot; i++)
        {
            char c = pfs_fat_to_upper_ascii(utf8[i]);

            if ('.' == c || ' ' == c)
            {
                lossy = 1;
                continue;
            }
            if (!pfs_fat_is_short_char(c))
            {
                c = '_';
                lossy = 1;
            }
            if (base_n < 8)
            {
                base[base_n++] = (uint8_t)c;
            }
            else
            {
                lossy = 1;
            }
        }
        for (int i = 1; '\0' != dot[i]; i++)
        {
            char c = pfs_fat_to_upper_ascii(dot[i]);

            if (!pfs_fat_is_short_char(c))
            {
                c = '_';
                lossy = 1;
            }
            if (ext_n < 3)
            {
                ext[ext_n++] = (uint8_t)c;
            }
            else
            {
                lossy = 1;
            }
        }
    }
    else
    {
        for (int i = 0; '\0' != utf8[i]; i++)
        {
            char c = pfs_fat_to_upper_ascii(utf8[i]);

            if ('.' == c || ' ' == c)
            {
                lossy = 1;
                continue;
            }
            if (!pfs_fat_is_short_char(c))
            {
                c = '_';
                lossy = 1;
            }
            if (base_n < 8)
            {
                base[base_n++] = (uint8_t)c;
            }
            else
            {
                lossy = 1;
            }
        }
    }
    if (0 == base_n)
    {
        base[0] = '_';
        base_n = 1;
        lossy = 1;
    }
    *p_base_len = base_n;
    *p_ext_len = ext_n;
    *p_lossy = lossy;
}

static void pfs_fat_short_from_basis(uint8_t out[FAT_MSDOS_NAME], const uint8_t base[8],
        int base_len, const uint8_t ext[3], int ext_len)
{

    memset(out, ' ', FAT_MSDOS_NAME);
    for (int i = 0; i < base_len && i < 8; i++)
    {
        out[i] = base[i];
    }
    for (int i = 0; i < ext_len && i < 3; i++)
    {
        out[8 + i] = ext[i];
    }
}

// 将 ~n 写入基名尾部 (占 8 字符窗); n 为 1..999999
static int pfs_fat_short_with_tilde(uint8_t out[FAT_MSDOS_NAME], const uint8_t base[8],
        int base_len, const uint8_t ext[3], int ext_len, unsigned int n)
{
    char num[8];
    int nd;
    int prefix;

    if (0 == n || n > 999999u)
    {
        return PFS_FAT_RET_EINVAL;
    }
    nd = (int)snprintf(num, sizeof(num), "%u", n);
    if (nd <= 0 || nd >= (int)sizeof(num))
    {
        return PFS_FAT_RET_EINVAL;
    }
    // 8 = prefix + '~' + digits
    prefix = 8 - 1 - nd;
    if (prefix < 1)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (prefix > base_len)
    {
        prefix = base_len;
    }

    memset(out, ' ', FAT_MSDOS_NAME);
    for (int i = 0; i < prefix; i++)
    {
        out[i] = base[i];
    }
    out[prefix] = (uint8_t)'~';
    for (int i = 0; i < nd; i++)
    {
        out[prefix + 1 + i] = (uint8_t)num[i];
    }
    for (int i = 0; i < ext_len && i < 3; i++)
    {
        out[8 + i] = ext[i];
    }
    return PFS_FAT_RET_OK;
}

static int64_t pfs_fat_dir_byte_limit(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root)
{
    const pfs_fat_bs_t *p_bs;
    uint32_t clu;
    uint32_t n;

    p_bs = PFS_FAT_SBI_BS(p_sbi);
    if (is_fixed_root)
    {
        return (int64_t)p_bs->rootdir_sectors * (int64_t)p_bs->sector_size;
    }
    clu = start_clu;
    n = 0;
    while (pfs_fat_is_valid_cluster(p_sbi, clu) && n < 0x100000u)
    {
        n++;
        int rc = pfs_fat_get_next_cluster(p_sbi, &clu);
        if (PFS_FAT_RET_OK != rc || FAT_ENT_EOF == clu)
        {
            break;
        }
    }
    return (int64_t)n * (int64_t)p_bs->cluster_size;
}

static int pfs_fat_short_name_exists(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        const uint8_t name[FAT_MSDOS_NAME])
{
    int64_t limit;
    uint8_t raw[32];

    if (NULL == p_sbi || NULL == name)
    {
        return PFS_FAT_RET_EINVAL;
    }

    limit = pfs_fat_dir_byte_limit(p_sbi, start_clu, is_fixed_root);
    for (uint32_t cpos = 0; cpos + 32 <= limit; cpos += 32)
    {
        int rc = pfs_fat_dir_read_raw(p_sbi, start_clu, is_fixed_root, cpos, raw);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (0x00 == raw[0])
        {
            return 0;
        }
        if (FAT_DELETED_FLAG == raw[0])
        {
            continue;
        }
        // LFN 槽 attr==0x0F
        if (FAT_ATTR_EXT == (raw[11] & 0x3F))
        {
            continue;
        }
        if (0 != (raw[11] & FAT_ATTR_VOLUME))
        {
            continue;
        }
        if (0 == memcmp(raw, name, FAT_MSDOS_NAME))
        {
            return 1;
        }
    }
    return 0;
}

// 生成目录内唯一的 8.3 短名; 优先无损短名, 否则 ~1/~2/...
static int pfs_fat_make_unique_short_name(pfs_fat_sbi_t *p_sbi, uint32_t start_clu,
        int is_fixed_root, const char *utf8, uint8_t out[FAT_MSDOS_NAME])
{
    uint8_t base[8];
    uint8_t ext[3];
    int base_len;
    int ext_len;
    int lossy;
    int exists;

    if (NULL == out)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_build_short_basis(utf8, base, &base_len, ext, &ext_len, &lossy);

    if (0 == lossy)
    {
        pfs_fat_short_from_basis(out, base, base_len, ext, ext_len);
        exists = pfs_fat_short_name_exists(p_sbi, start_clu, is_fixed_root, out);
        if (exists < 0)
        {
            return exists;
        }
        if (0 == exists)
        {
            return PFS_FAT_RET_OK;
        }
    }

    for (unsigned int n = 1; n <= 999999u; n++)
    {
        int rc = pfs_fat_short_with_tilde(out, base, base_len, ext, ext_len, n);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        exists = pfs_fat_short_name_exists(p_sbi, start_clu, is_fixed_root, out);
        if (exists < 0)
        {
            return exists;
        }
        if (0 == exists)
        {
            return PFS_FAT_RET_OK;
        }
    }
    return PFS_FAT_RET_ENOSPC;
}

static int pfs_fat_search_empty_slots(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int need, int64_t skip_start, int skip_slots, int64_t *p_cpos)
{
    int64_t limit;
    int64_t run_start;
    int64_t skip_end;
    int run;
    uint8_t raw[32];

    if (NULL == p_cpos || need <= 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    limit = pfs_fat_dir_byte_limit(p_sbi, start_clu, is_fixed_root);
    skip_end = (skip_slots > 0) ? (skip_start + (int64_t)skip_slots * 32) : -1;
    run_start = 0;
    run = 0;
    for (uint32_t cpos = 0; cpos + 32 <= limit; cpos += 32)
    {
        if (skip_slots > 0 && cpos >= skip_start && cpos < skip_end)
        {
            run = 0;
            continue;
        }
        int rc = pfs_fat_dir_read_raw(p_sbi, start_clu, is_fixed_root, cpos, raw);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (0x00 == raw[0])
        {
            if (0 == run)
            {
                run_start = cpos;
            }
            if ((int)((limit - run_start) / 32) >= need)
            {
                *p_cpos = run_start;
                return PFS_FAT_RET_OK;
            }
            return PFS_FAT_RET_ENOSPC;
        }
        if (FAT_DELETED_FLAG == raw[0])
        {
            if (0 == run)
            {
                run_start = cpos;
            }
            run++;
            if (run >= need)
            {
                *p_cpos = run_start;
                return PFS_FAT_RET_OK;
            }
            continue;
        }
        run = 0;
    }
    return PFS_FAT_RET_ENOSPC;
}

static void pfs_fat_fill_lfn_slot(fat_dir_slot_t *p_slot, int id, int is_last,
        uint8_t alias_chksum, const uint16_t *uname, int uname_off)
{
    uint16_t ch;

    memset(p_slot, 0, sizeof(*p_slot));
    p_slot->id = (uint8_t)(id | (is_last ? 0x40 : 0));
    p_slot->attr = FAT_ATTR_EXT;
    p_slot->alias_checksum = alias_chksum;
    p_slot->start = 0;

    for (int i = 0; i < 5; i++)
    {
        ch = (uname_off + i < FAT_MAX_UNI_CHARS) ? uname[uname_off + i] : 0xFFFF;
        p_slot->name0_4[i * 2] = (uint8_t)(ch & 0xFF);
        p_slot->name0_4[i * 2 + 1] = (uint8_t)((ch >> 8) & 0xFF);
    }
    for (int i = 0; i < 6; i++)
    {
        ch = (uname_off + 5 + i < FAT_MAX_UNI_CHARS) ? uname[uname_off + 5 + i] : 0xFFFF;
        p_slot->name5_10[i * 2] = (uint8_t)(ch & 0xFF);
        p_slot->name5_10[i * 2 + 1] = (uint8_t)((ch >> 8) & 0xFF);
    }
    for (int i = 0; i < 2; i++)
    {
        ch = (uname_off + 11 + i < FAT_MAX_UNI_CHARS) ? uname[uname_off + 11 + i] : 0xFFFF;
        p_slot->name11_12[i * 2] = (uint8_t)(ch & 0xFF);
        p_slot->name11_12[i * 2 + 1] = (uint8_t)((ch >> 8) & 0xFF);
    }
}

int pfs_fat_create_file(pfs_fat_sbi_t *p_sbi, const char *path, pfs_fat_dir_entry_t *p_info)
{
    pfs_fat_dir_entry_t parent;
    pfs_fat_dir_entry_t exist;
    char leaf[FAT_LFN_LEN + 1];
    char parent_path[512];
    const char *leaf_start;
    const char *p;
    uint16_t uname[FAT_MAX_UNI_CHARS];
    uint8_t short_name[FAT_MSDOS_NAME];
    uint8_t alias_chksum;
    fat_dir_entry_t short_ent;
    int64_t slot_cpos;
    int leaf_len;
    int parent_len;
    int uni_len;
    int lfn_slots;
    int need;
    uint32_t dir_clu;
    int is_fixed_root;

    if (NULL == p_sbi || NULL == path || NULL == p_info)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_FAT_RET_EINVAL;
    }

    leaf_start = p;
    while ('\0' != *p)
    {
        const char *slash = p;

        while ('\0' != *slash && '/' != *slash)
        {
            slash++;
        }
        if ('\0' == *slash)
        {
            leaf_start = p;
            break;
        }
        p = slash;
        while ('/' == *p)
        {
            p++;
        }
        if ('\0' == *p)
        {
            return PFS_FAT_RET_EINVAL;
        }
        leaf_start = p;
    }

    leaf_len = 0;
    while ('\0' != leaf_start[leaf_len] && '/' != leaf_start[leaf_len])
    {
        if (leaf_len >= FAT_LFN_LEN)
        {
            return PFS_FAT_RET_ENAMETOOLONG;
        }
        leaf[leaf_len] = leaf_start[leaf_len];
        leaf_len++;
    }
    leaf[leaf_len] = '\0';
    if (0 == leaf_len || (1 == leaf_len && '.' == leaf[0]) ||
            (2 == leaf_len && '.' == leaf[0] && '.' == leaf[1]))
    {
        return PFS_FAT_RET_EINVAL;
    }

    int rc;
    parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && '/' == path[parent_len - 1])
    {
        parent_len--;
    }
    if (parent_len <= 0)
    {
        pfs_fat_fill_root_info(p_sbi, &parent);
    }
    else
    {
        if (parent_len >= (int)sizeof(parent_path))
        {
            return PFS_FAT_RET_ENAMETOOLONG;
        }
        memcpy(parent_path, path, (size_t)parent_len);
        parent_path[parent_len] = '\0';
        rc = pfs_fat_lookup_path(p_sbi, parent_path, &parent);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (PFS_FAT_TYPE_DIR != parent.type)
        {
            return PFS_FAT_RET_ENOENT;
        }
    }

    dir_clu = parent.start_clu;
    is_fixed_root = parent.is_fixed_root ? 1 : 0;

    rc = pfs_fat_find_dir_entry(p_sbi, dir_clu, is_fixed_root, leaf, &exist);
    if (PFS_FAT_RET_OK == rc)
    {
        return PFS_FAT_RET_EEXIST;
    }
    if (PFS_FAT_RET_ENOENT != rc)
    {
        return rc;
    }

    uni_len = pfs_fat_utf8_to_utf16(leaf, leaf_len, uname, FAT_MAX_UNI_CHARS);
    if (uni_len < 0)
    {
        return uni_len;
    }

    rc = pfs_fat_make_unique_short_name(p_sbi, dir_clu, is_fixed_root, leaf, short_name);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    alias_chksum = pfs_fat_checksum(short_name);
    lfn_slots = (uni_len + 12) / 13;
    if (lfn_slots <= 0)
    {
        lfn_slots = 1;
    }
    need = lfn_slots + 1;

    rc = pfs_fat_search_empty_slots(p_sbi, dir_clu, is_fixed_root, need, -1, 0, &slot_cpos);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    // LFN 自高序号写到低序号; 名后首空为 0, 其余 0xFFFF (对齐 linux xlate_to_uni)
    uname[uni_len] = 0;
    for (int i = uni_len + 1; i < FAT_MAX_UNI_CHARS; i++)
    {
        uname[i] = 0xFFFF;
    }
    for (int i = lfn_slots; i >= 1; i--)
    {
        fat_dir_slot_t slot;
        int uname_off = (i - 1) * 13;

        pfs_fat_fill_lfn_slot(&slot, i, (i == lfn_slots) ? 1 : 0, alias_chksum, uname, uname_off);
        rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
                slot_cpos + (int64_t)(lfn_slots - i) * 32, &slot);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }

    memset(&short_ent, 0, sizeof(short_ent));
    memcpy(short_ent.name, short_name, FAT_MSDOS_NAME);
    short_ent.attr = FAT_ATTR_ARCH;
    short_ent.start = 0;
    short_ent.starthi = 0;
    short_ent.size = 0;
    rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
            slot_cpos + (int64_t)lfn_slots * 32, &short_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    memset(p_info, 0, sizeof(*p_info));
    p_info->type = PFS_FAT_TYPE_FILE;
    p_info->attr = FAT_ATTR_ARCH;
    p_info->start_clu = 0;
    p_info->size = 0;
    p_info->is_root = 0;
    p_info->is_fixed_root = 0;
    p_info->parent_clu = dir_clu;
    p_info->parent_fixed_root = is_fixed_root;
    p_info->sfn_cpos = slot_cpos + (int64_t)lfn_slots * 32;
    return PFS_FAT_RET_OK;
}

int pfs_fat_update_sfn_meta(pfs_fat_sbi_t *p_sbi, const pfs_fat_dir_entry_t *p_loc,
        uint32_t start_clu, uint32_t size)
{
    fat_dir_entry_t ent;

    if (NULL == p_sbi || NULL == p_loc || p_loc->sfn_cpos < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    int rc = pfs_fat_dir_read_raw(p_sbi, p_loc->parent_clu, p_loc->parent_fixed_root,
            p_loc->sfn_cpos, &ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    // host 序改 start/size 再写回 LE
    ent.ctime = pfs_le16_to_cpu(ent.ctime);
    ent.cdate = pfs_le16_to_cpu(ent.cdate);
    ent.adate = pfs_le16_to_cpu(ent.adate);
    ent.starthi = pfs_le16_to_cpu(ent.starthi);
    ent.time = pfs_le16_to_cpu(ent.time);
    ent.date = pfs_le16_to_cpu(ent.date);
    ent.start = pfs_le16_to_cpu(ent.start);
    ent.size = pfs_le32_to_cpu(ent.size);

    pfs_fat_set_start(&ent, start_clu);
    ent.size = size;

    ent.ctime = pfs_cpu_to_le16(ent.ctime);
    ent.cdate = pfs_cpu_to_le16(ent.cdate);
    ent.adate = pfs_cpu_to_le16(ent.adate);
    ent.starthi = pfs_cpu_to_le16(ent.starthi);
    ent.time = pfs_cpu_to_le16(ent.time);
    ent.date = pfs_cpu_to_le16(ent.date);
    ent.start = pfs_cpu_to_le16(ent.start);
    ent.size = pfs_cpu_to_le32(ent.size);

    return pfs_fat_dir_write_raw(p_sbi, p_loc->parent_clu, p_loc->parent_fixed_root,
            p_loc->sfn_cpos, &ent);
}

static int pfs_fat_mark_entry_deleted(pfs_fat_sbi_t *p_sbi, uint32_t dir_clu, int is_fixed_root,
        int64_t cpos)
{
    uint8_t raw[sizeof(fat_dir_entry_t)];

    int rc = pfs_fat_dir_read_raw(p_sbi, dir_clu, is_fixed_root, cpos, raw);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (0x00 == raw[0])
    {
        return PFS_FAT_RET_OK;
    }
    raw[0] = FAT_DELETED_FLAG;
    return pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root, cpos, raw);
}

static int pfs_fat_unlink_entry_at(pfs_fat_sbi_t *p_sbi, const pfs_fat_dir_entry_t *p_info)
{
    int64_t cpos;
    uint8_t raw[sizeof(fat_dir_entry_t)];

    if (NULL == p_info || p_info->sfn_cpos < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    // 向前删除连续 LFN 槽
    cpos = p_info->sfn_cpos - 32;
    while (cpos >= 0)
    {
        int rc = pfs_fat_dir_read_raw(p_sbi, p_info->parent_clu, p_info->parent_fixed_root, cpos, raw);
        if (PFS_FAT_RET_OK != rc)
        {
            break;
        }
        if (FAT_ATTR_EXT != raw[11])
        {
            break;
        }
        rc = pfs_fat_mark_entry_deleted(p_sbi, p_info->parent_clu, p_info->parent_fixed_root, cpos);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        cpos -= 32;
    }
    return pfs_fat_mark_entry_deleted(p_sbi, p_info->parent_clu, p_info->parent_fixed_root,
            p_info->sfn_cpos);
}

static int pfs_fat_path_parent_leaf(pfs_fat_sbi_t *p_sbi, const char *path,
        pfs_fat_dir_entry_t *p_parent, char *leaf, int leaf_cap, int *p_leaf_len)
{
    pfs_fat_dir_entry_t parent;
    char parent_path[512];
    const char *p;
    const char *leaf_start;
    int leaf_len;
    int parent_len;

    if (NULL == p_sbi || NULL == path || NULL == p_parent || NULL == leaf || leaf_cap <= 1 ||
            NULL == p_leaf_len)
    {
        return PFS_FAT_RET_EINVAL;
    }

    p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_FAT_RET_EINVAL;
    }

    leaf_start = p;
    while ('\0' != *p)
    {
        const char *slash = p;

        while ('\0' != *slash && '/' != *slash)
        {
            slash++;
        }
        if ('\0' == *slash)
        {
            leaf_start = p;
            break;
        }
        p = slash;
        while ('/' == *p)
        {
            p++;
        }
        if ('\0' == *p)
        {
            return PFS_FAT_RET_EINVAL;
        }
        leaf_start = p;
    }

    leaf_len = 0;
    while ('\0' != leaf_start[leaf_len] && '/' != leaf_start[leaf_len])
    {
        if (leaf_len + 1 >= leaf_cap)
        {
            return PFS_FAT_RET_ENAMETOOLONG;
        }
        leaf[leaf_len] = leaf_start[leaf_len];
        leaf_len++;
    }
    leaf[leaf_len] = '\0';
    if (0 == leaf_len || (1 == leaf_len && '.' == leaf[0]) ||
            (2 == leaf_len && '.' == leaf[0] && '.' == leaf[1]))
    {
        return PFS_FAT_RET_EINVAL;
    }

    parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && '/' == path[parent_len - 1])
    {
        parent_len--;
    }
    if (parent_len <= 0)
    {
        pfs_fat_fill_root_info(p_sbi, &parent);
    }
    else
    {
        if (parent_len >= (int)sizeof(parent_path))
        {
            return PFS_FAT_RET_ENAMETOOLONG;
        }
        memcpy(parent_path, path, (size_t)parent_len);
        parent_path[parent_len] = '\0';
        int rc = pfs_fat_lookup_path(p_sbi, parent_path, &parent);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (PFS_FAT_TYPE_DIR != parent.type)
        {
            return PFS_FAT_RET_ENOENT;
        }
    }

    *p_parent = parent;
    *p_leaf_len = leaf_len;
    return PFS_FAT_RET_OK;
}

int pfs_fat_mkdir(pfs_fat_sbi_t *p_sbi, const char *path)
{
    pfs_fat_dir_entry_t parent;
    pfs_fat_dir_entry_t exist;
    char leaf[FAT_LFN_LEN + 1];
    uint16_t uname[FAT_MAX_UNI_CHARS];
    uint8_t short_name[FAT_MSDOS_NAME];
    uint8_t alias_chksum;
    fat_dir_entry_t short_ent;
    fat_dir_entry_t dot_ent;
    int64_t slot_cpos;
    uint32_t new_clu;
    uint32_t dir_clu;
    uint32_t parent_clu_for_dotdot;
    int is_fixed_root;
    int leaf_len;
    int uni_len;
    int lfn_slots;
    int need;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    int rc = pfs_fat_path_parent_leaf(p_sbi, path, &parent, leaf, (int)sizeof(leaf), &leaf_len);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    dir_clu = parent.start_clu;
    is_fixed_root = parent.is_fixed_root ? 1 : 0;

    rc = pfs_fat_find_dir_entry(p_sbi, dir_clu, is_fixed_root, leaf, &exist);
    if (PFS_FAT_RET_OK == rc)
    {
        return PFS_FAT_RET_EEXIST;
    }
    if (PFS_FAT_RET_ENOENT != rc)
    {
        return rc;
    }

    uni_len = pfs_fat_utf8_to_utf16(leaf, leaf_len, uname, FAT_MAX_UNI_CHARS);
    if (uni_len < 0)
    {
        return uni_len;
    }
    rc = pfs_fat_make_unique_short_name(p_sbi, dir_clu, is_fixed_root, leaf, short_name);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    alias_chksum = pfs_fat_checksum(short_name);
    lfn_slots = (uni_len + 12) / 13;
    if (lfn_slots <= 0)
    {
        lfn_slots = 1;
    }
    need = lfn_slots + 1;

    rc = pfs_fat_search_empty_slots(p_sbi, dir_clu, is_fixed_root, need, -1, 0, &slot_cpos);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_fat_alloc_cluster(p_sbi, 0, &new_clu);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_fat_zero_cluster(p_sbi, new_clu);
    if (PFS_FAT_RET_OK != rc)
    {
        (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
        return rc;
    }

    // . / ..
    memset(&dot_ent, 0, sizeof(dot_ent));
    memset(dot_ent.name, ' ', FAT_MSDOS_NAME);
    dot_ent.name[0] = '.';
    dot_ent.attr = FAT_ATTR_DIR;
    pfs_fat_set_start(&dot_ent, new_clu);
    rc = pfs_fat_dir_write_raw(p_sbi, new_clu, 0, 0, &dot_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
        return rc;
    }

    memset(&dot_ent, 0, sizeof(dot_ent));
    memset(dot_ent.name, ' ', FAT_MSDOS_NAME);
    dot_ent.name[0] = '.';
    dot_ent.name[1] = '.';
    dot_ent.attr = FAT_ATTR_DIR;
    // .. 指向父目录: 卷根用魔数 0 (含 FAT32; 对齐 linux i_logstart / Win chkdsk)
    // 非根才写真实 start_clu; 勿对 FAT32 根写 root_cluster
    if (parent.is_root || parent.is_fixed_root)
    {
        parent_clu_for_dotdot = 0;
    }
    else
    {
        parent_clu_for_dotdot = parent.start_clu;
    }
    pfs_fat_set_start(&dot_ent, parent_clu_for_dotdot);
    rc = pfs_fat_dir_write_raw(p_sbi, new_clu, 0, 32, &dot_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
        return rc;
    }

    uname[uni_len] = 0;
    for (int i = uni_len + 1; i < FAT_MAX_UNI_CHARS; i++)
    {
        uname[i] = 0xFFFF;
    }
    for (int i = lfn_slots; i >= 1; i--)
    {
        fat_dir_slot_t slot;
        int uname_off = (i - 1) * 13;

        pfs_fat_fill_lfn_slot(&slot, i, (i == lfn_slots) ? 1 : 0, alias_chksum, uname, uname_off);
        rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
                slot_cpos + (int64_t)(lfn_slots - i) * 32, &slot);
        if (PFS_FAT_RET_OK != rc)
        {
            (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
            return rc;
        }
    }

    memset(&short_ent, 0, sizeof(short_ent));
    memcpy(short_ent.name, short_name, FAT_MSDOS_NAME);
    short_ent.attr = FAT_ATTR_DIR;
    pfs_fat_set_start(&short_ent, new_clu);
    short_ent.size = 0;
    rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
            slot_cpos + (int64_t)lfn_slots * 32, &short_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        (void)pfs_fat_free_cluster_chain(p_sbi, new_clu);
        return rc;
    }
    return PFS_FAT_RET_OK;
}

static int pfs_fat_dir_is_empty(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root)
{
    pfs_fat_dir_entry_t de;
    char utf8[FAT_LFN_LEN * 3 + 4];
    int64_t cpos = 0;

    for (;;)
    {
        utf8[0] = '\0';
        int rc = pfs_fat_readdir(p_sbi, start_clu, is_fixed_root, &cpos, &de, utf8, (int)sizeof(utf8));
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if ('\0' == utf8[0])
        {
            return 1;
        }
        // readdir 已跳过 . / ..; 出现任何项则非空
        return 0;
    }
}

int pfs_fat_unlink(pfs_fat_sbi_t *p_sbi, const char *path)
{
    pfs_fat_dir_entry_t info;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    int rc = pfs_fat_lookup_path(p_sbi, path, &info);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_FAT_TYPE_FILE != info.type)
    {
        return PFS_FAT_RET_EPERM;
    }
    if (0 != (info.attr & FAT_ATTR_RO))
    {
        return PFS_FAT_RET_EPERM;
    }

    if (0 != info.start_clu)
    {
        rc = pfs_fat_free_cluster_chain(p_sbi, info.start_clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return pfs_fat_unlink_entry_at(p_sbi, &info);
}

int pfs_fat_rmdir(pfs_fat_sbi_t *p_sbi, const char *path)
{
    pfs_fat_dir_entry_t info;
    int empty;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    int rc = pfs_fat_lookup_path(p_sbi, path, &info);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_FAT_TYPE_DIR != info.type || info.is_root)
    {
        return PFS_FAT_RET_EPERM;
    }

    empty = pfs_fat_dir_is_empty(p_sbi, info.start_clu, 0);
    if (empty < 0)
    {
        return empty;
    }
    if (0 == empty)
    {
        return PFS_FAT_RET_ENOTEMPTY;
    }

    if (0 != info.start_clu)
    {
        rc = pfs_fat_free_cluster_chain(p_sbi, info.start_clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return pfs_fat_unlink_entry_at(p_sbi, &info);
}

static int pfs_fat_count_lfn_slots(pfs_fat_sbi_t *p_sbi, const pfs_fat_dir_entry_t *p_info)
{
    int64_t cpos;
    uint8_t raw[32];
    int n;

    if (NULL == p_info || p_info->sfn_cpos < 0)
    {
        return 0;
    }
    n = 0;
    cpos = p_info->sfn_cpos - 32;
    while (cpos >= 0)
    {
        int rc = pfs_fat_dir_read_raw(p_sbi, p_info->parent_clu, p_info->parent_fixed_root, cpos, raw);
        if (PFS_FAT_RET_OK != rc)
        {
            break;
        }
        if (FAT_ATTR_EXT != raw[11])
        {
            break;
        }
        n++;
        cpos -= 32;
    }
    return n;
}

static int pfs_fat_update_dotdot(pfs_fat_sbi_t *p_sbi, uint32_t dir_clu, uint32_t parent_clu)
{
    fat_dir_entry_t ent;

    int rc = pfs_fat_dir_read_raw(p_sbi, dir_clu, 0, 32, &ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    ent.starthi = pfs_le16_to_cpu(ent.starthi);
    ent.start = pfs_le16_to_cpu(ent.start);
    pfs_fat_set_start(&ent, parent_clu);
    ent.starthi = pfs_cpu_to_le16(ent.starthi);
    ent.start = pfs_cpu_to_le16(ent.start);
    return pfs_fat_dir_write_raw(p_sbi, dir_clu, 0, 32, &ent);
}

static int pfs_fat_replace_target(pfs_fat_sbi_t *p_sbi, pfs_fat_dir_entry_t *p_exist)
{
    int empty;

    if (PFS_FAT_TYPE_DIR == p_exist->type)
    {
        empty = pfs_fat_dir_is_empty(p_sbi, p_exist->start_clu, 0);
        if (empty < 0)
        {
            return empty;
        }
        if (0 == empty)
        {
            return PFS_FAT_RET_ENOTEMPTY;
        }
        if (0 != p_exist->start_clu)
        {
            int rc = pfs_fat_free_cluster_chain(p_sbi, p_exist->start_clu);
            if (PFS_FAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else if (PFS_FAT_TYPE_FILE == p_exist->type)
    {
        if (0 != p_exist->start_clu)
        {
            int rc = pfs_fat_free_cluster_chain(p_sbi, p_exist->start_clu);
            if (PFS_FAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else
    {
        return PFS_FAT_RET_EPERM;
    }
    return pfs_fat_unlink_entry_at(p_sbi, p_exist);
}

int pfs_fat_rename(pfs_fat_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    pfs_fat_dir_entry_t old_info;
    pfs_fat_dir_entry_t new_parent;
    pfs_fat_dir_entry_t exist;
    fat_dir_entry_t short_ent;
    char leaf[FAT_LFN_LEN + 1];
    uint16_t uname[FAT_MAX_UNI_CHARS];
    uint8_t short_name[FAT_MSDOS_NAME];
    uint8_t alias_chksum;
    int64_t slot_cpos;
    int64_t skip_start;
    int skip_slots;
    uint32_t dir_clu;
    uint32_t old_parent_clu;
    uint32_t parent_for_dotdot;
    int is_fixed_root;
    int old_fixed;
    int leaf_len;
    int uni_len;
    int lfn_slots;
    int need;
    int same_parent;

    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }
    if (0 == strcmp(oldpath, newpath))
    {
        return PFS_FAT_RET_OK;
    }

    int rc = pfs_fat_lookup_path(p_sbi, oldpath, &old_info);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (old_info.is_root || old_info.sfn_cpos < 0)
    {
        return PFS_FAT_RET_EPERM;
    }

    rc = pfs_fat_path_parent_leaf(p_sbi, newpath, &new_parent, leaf, (int)sizeof(leaf), &leaf_len);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    dir_clu = new_parent.start_clu;
    is_fixed_root = new_parent.is_fixed_root ? 1 : 0;

    if (PFS_FAT_TYPE_DIR == old_info.type && !is_fixed_root && dir_clu == old_info.start_clu)
    {
        return PFS_FAT_RET_EINVAL;
    }

    rc = pfs_fat_find_dir_entry(p_sbi, dir_clu, is_fixed_root, leaf, &exist);
    if (PFS_FAT_RET_OK == rc)
    {
        if (exist.sfn_cpos == old_info.sfn_cpos && exist.parent_clu == old_info.parent_clu &&
                exist.parent_fixed_root == old_info.parent_fixed_root)
        {
            // 同项 (大小写), 继续重写名
        }
        else
        {
            if (old_info.type != exist.type)
            {
                return PFS_FAT_RET_EEXIST;
            }
            rc = pfs_fat_replace_target(p_sbi, &exist);
            if (PFS_FAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else if (PFS_FAT_RET_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_fat_dir_read_raw(p_sbi, old_info.parent_clu, old_info.parent_fixed_root,
            old_info.sfn_cpos, &short_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    uni_len = pfs_fat_utf8_to_utf16(leaf, leaf_len, uname, FAT_MAX_UNI_CHARS);
    if (uni_len < 0)
    {
        return uni_len;
    }
    rc = pfs_fat_make_unique_short_name(p_sbi, dir_clu, is_fixed_root, leaf, short_name);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    alias_chksum = pfs_fat_checksum(short_name);
    lfn_slots = (uni_len + 12) / 13;
    if (lfn_slots <= 0)
    {
        lfn_slots = 1;
    }
    need = lfn_slots + 1;

    same_parent = (old_info.parent_clu == dir_clu && old_info.parent_fixed_root == is_fixed_root) ? 1 : 0;
    skip_start = -1;
    skip_slots = 0;
    if (same_parent)
    {
        skip_slots = pfs_fat_count_lfn_slots(p_sbi, &old_info) + 1;
        skip_start = old_info.sfn_cpos - (int64_t)(skip_slots - 1) * 32;
        if (skip_start < 0)
        {
            skip_start = 0;
        }
    }

    rc = pfs_fat_search_empty_slots(p_sbi, dir_clu, is_fixed_root, need, skip_start, skip_slots, &slot_cpos);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    uname[uni_len] = 0;
    for (int i = uni_len + 1; i < FAT_MAX_UNI_CHARS; i++)
    {
        uname[i] = 0xFFFF;
    }
    for (int i = lfn_slots; i >= 1; i--)
    {
        fat_dir_slot_t slot;
        int uname_off = (i - 1) * 13;

        pfs_fat_fill_lfn_slot(&slot, i, (i == lfn_slots) ? 1 : 0, alias_chksum, uname, uname_off);
        rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
                slot_cpos + (int64_t)(lfn_slots - i) * 32, &slot);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }

    memcpy(short_ent.name, short_name, FAT_MSDOS_NAME);
    rc = pfs_fat_dir_write_raw(p_sbi, dir_clu, is_fixed_root,
            slot_cpos + (int64_t)lfn_slots * 32, &short_ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    old_parent_clu = old_info.parent_clu;
    old_fixed = old_info.parent_fixed_root;
    if (!(same_parent && slot_cpos == skip_start))
    {
        rc = pfs_fat_unlink_entry_at(p_sbi, &old_info);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }

    if (PFS_FAT_TYPE_DIR == old_info.type &&
            (old_parent_clu != dir_clu || old_fixed != is_fixed_root))
    {
        uint32_t child;

        // 迁入卷根时 .. 亦须魔数 0 (FAT32 同; 与 mkdir 一致)
        if (new_parent.is_root || is_fixed_root)
        {
            parent_for_dotdot = 0;
        }
        else
        {
            parent_for_dotdot = dir_clu;
        }
        child = (uint32_t)pfs_le16_to_cpu(short_ent.start);
        if (pfs_fat_is_fat32(p_sbi))
        {
            child |= ((uint32_t)pfs_le16_to_cpu(short_ent.starthi) << 16);
        }
        rc = pfs_fat_update_dotdot(p_sbi, child, parent_for_dotdot);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_FAT_RET_OK;
}

// end
