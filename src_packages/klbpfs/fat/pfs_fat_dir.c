// Doc Encode : UTF-8 BOM, Unix(LF)
// 目录项 (linux-7.1 fs/fat/dir.c; 只读 readdir / 短名+LFN)
#include "fat/pfs_fat_internal.h"

#include <string.h>

static int pfs_fat_utf8_encode(uint32_t cp, char *out, int out_cap)
{
    if (cp <= 0x7Fu)
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

int pfs_fat_utf16_to_utf8(const uint16_t *uniname, int max_uni, char *out, int out_cap)
{
    int out_len = 0;

    if (NULL == uniname || NULL == out || out_cap <= 0)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (max_uni <= 0)
    {
        max_uni = FAT_LFN_LEN;
    }

    for (int i = 0; i < max_uni; i++)
    {
        uint16_t ch = uniname[i];
        char seq[4];
        int n;

        if (0x0 == ch || 0xFFFFu == ch)
        {
            break;
        }
        n = pfs_fat_utf8_encode(ch, seq, (int)sizeof(seq));
        if (n < 0 || out_len + n >= out_cap)
        {
            return PFS_FAT_RET_ERANGE;
        }
        memcpy(out + out_len, seq, (size_t)n);
        out_len += n;
    }
    out[out_len] = '\0';
    return out_len;
}

static void pfs_fat_slot_copy_uni(uint16_t *dst, const uint8_t *src, int nchar)
{

    for (int i = 0; i < nchar; i++)
    {
        dst[i] = (uint16_t)src[i * 2] | ((uint16_t)src[i * 2 + 1] << 8);
    }
}

static int pfs_fat_dir_byte_eof(const pfs_fat_sbi_t *p_sbi, int is_fixed_root, int64_t cpos)
{
    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    if (is_fixed_root)
    {
        int64_t limit = (int64_t)p_bs->rootdir_sectors * (int64_t)p_bs->sector_size;

        return (cpos >= limit) ? 1 : 0;
    }
    return (cpos < 0 || cpos >= (int64_t)FAT_MAX_DIR_SIZE) ? 1 : 0;
}

// 读目录绝对字节偏移处的一项 (32B); EOF 返回 PFS_FAT_RET_ENOENT
static int pfs_fat_dir_read_at(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t cpos, fat_dir_entry_t *p_out)
{
    if (NULL == p_sbi || NULL == p_out || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (0 != (cpos & ((int64_t)sizeof(fat_dir_entry_t) - 1)))
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (pfs_fat_dir_byte_eof(p_sbi, is_fixed_root, cpos))
    {
        return PFS_FAT_RET_ENOENT;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);

    if (is_fixed_root)
    {
        pfs_cache_sector_slot_t *p_slot;
        uint64_t sector_id;
        uint32_t byte_in_sec;

        // FAT12/16 固定根: root_sec_cache 按扇区
        if (0 == p_sbi->root_sec_cache_on || 0 == p_bs->sector_size)
        {
            return PFS_FAT_RET_EBUSY;
        }
        sector_id = (uint64_t)p_bs->dir_start + (uint64_t)(cpos / (int64_t)p_bs->sector_size);
        byte_in_sec = (uint32_t)(cpos % (int64_t)p_bs->sector_size);
        p_slot = pfs_fat_root_sec_cache_get(&p_sbi->root_sec_cache, sector_id);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_FAT_RET_EIO;
        }
        if (byte_in_sec + sizeof(*p_out) > p_bs->sector_size)
        {
            return PFS_FAT_RET_EBADMSG;
        }
        memcpy(p_out, p_slot->p_data + byte_in_sec, sizeof(*p_out));
        return PFS_FAT_RET_OK;
    }

    {
        pfs_cache_cluster_slot_t *p_slot;

        // FAT32 根 / 子目录: dir_cache 按簇
        if (!pfs_fat_is_valid_cluster(p_sbi, start_clu))
        {
            return PFS_FAT_RET_EBADMSG;
        }
        uint32_t clu_off = (uint32_t)(cpos >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)cpos & (p_bs->cluster_size - 1U);
        uint32_t clu = start_clu;
        int rc = pfs_fat_cluster_walk(p_sbi, &clu, clu_off);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_EOF == clu || !pfs_fat_is_valid_cluster(p_sbi, clu))
        {
            return PFS_FAT_RET_ENOENT;
        }
        p_slot = pfs_fat_dir_cache_get(&p_sbi->dir_cache, clu);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_FAT_RET_EIO;
        }
        if (in_clu + sizeof(*p_out) > p_bs->cluster_size)
        {
            return PFS_FAT_RET_EBADMSG;
        }
        memcpy(p_out, p_slot->p_data + in_clu, sizeof(*p_out));
        return PFS_FAT_RET_OK;
    }
}

int pfs_fat_dir_write_raw(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t cpos, const void *p_raw32)
{
    if (NULL == p_sbi || NULL == p_raw32 || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }
    if (0 != (cpos & ((int64_t)sizeof(fat_dir_entry_t) - 1)))
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (pfs_fat_dir_byte_eof(p_sbi, is_fixed_root, cpos))
    {
        return PFS_FAT_RET_ENOSPC;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);

    if (is_fixed_root)
    {
        pfs_cache_sector_slot_t *p_slot;
        uint64_t sector_id;
        uint32_t byte_in_sec;

        if (0 == p_sbi->root_sec_cache_on || 0 == p_bs->sector_size)
        {
            return PFS_FAT_RET_EBUSY;
        }
        sector_id = (uint64_t)p_bs->dir_start + (uint64_t)(cpos / (int64_t)p_bs->sector_size);
        byte_in_sec = (uint32_t)(cpos % (int64_t)p_bs->sector_size);
        p_slot = pfs_fat_root_sec_cache_get(&p_sbi->root_sec_cache, sector_id);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_FAT_RET_EIO;
        }
        if (byte_in_sec + sizeof(fat_dir_entry_t) > p_bs->sector_size)
        {
            return PFS_FAT_RET_EBADMSG;
        }
        memcpy(p_slot->p_data + byte_in_sec, p_raw32, sizeof(fat_dir_entry_t));
        int64_t off = pfs_fat_sector_to_byte(p_sbi, (int64_t)sector_id);
        int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_slot->p_data, (int)p_bs->sector_size, off);
        if (0 > rc || rc != (int)p_bs->sector_size)
        {
            return PFS_FAT_RET_EIO;
        }
        pfs_fat_root_sec_cache_invalidate(&p_sbi->root_sec_cache, sector_id);
        return PFS_FAT_RET_OK;
    }

    {
        pfs_cache_cluster_slot_t *p_slot;

        if (!pfs_fat_is_valid_cluster(p_sbi, start_clu))
        {
            return PFS_FAT_RET_EBADMSG;
        }
        uint32_t clu_off = (uint32_t)(cpos >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)cpos & (p_bs->cluster_size - 1U);
        uint32_t clu = start_clu;
        int rc = pfs_fat_cluster_walk(p_sbi, &clu, clu_off);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_EOF == clu || !pfs_fat_is_valid_cluster(p_sbi, clu))
        {
            return PFS_FAT_RET_ENOSPC;
        }
        p_slot = pfs_fat_dir_cache_get(&p_sbi->dir_cache, clu);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_FAT_RET_EIO;
        }
        if (in_clu + sizeof(fat_dir_entry_t) > p_bs->cluster_size)
        {
            return PFS_FAT_RET_EBADMSG;
        }
        memcpy(p_slot->p_data + in_clu, p_raw32, sizeof(fat_dir_entry_t));
        int64_t off = pfs_fat_cluster_to_byte(p_sbi, clu);
        rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_slot->p_data, (int)p_bs->cluster_size, off);
        if (0 > rc || rc != (int)p_bs->cluster_size)
        {
            return PFS_FAT_RET_EIO;
        }
        pfs_fat_dir_cache_invalidate(&p_sbi->dir_cache, clu);
        return PFS_FAT_RET_OK;
    }
}

int pfs_fat_dir_read_raw(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t cpos, void *p_raw32)
{
    if (NULL == p_raw32)
    {
        return PFS_FAT_RET_EINVAL;
    }

    fat_dir_entry_t ent;
    int rc = pfs_fat_dir_read_at(p_sbi, start_clu, is_fixed_root, cpos, &ent);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    // read_at 返回盘上小端原样 (未 de_host)
    memcpy(p_raw32, &ent, sizeof(ent));
    return PFS_FAT_RET_OK;
}

static void pfs_fat_de_host(fat_dir_entry_t *p_de)
{
    // on-disk LE -> 主机序 (多字节字段)
    p_de->ctime = pfs_le16_to_cpu(p_de->ctime);
    p_de->cdate = pfs_le16_to_cpu(p_de->cdate);
    p_de->adate = pfs_le16_to_cpu(p_de->adate);
    p_de->starthi = pfs_le16_to_cpu(p_de->starthi);
    p_de->time = pfs_le16_to_cpu(p_de->time);
    p_de->date = pfs_le16_to_cpu(p_de->date);
    p_de->start = pfs_le16_to_cpu(p_de->start);
    p_de->size = pfs_le32_to_cpu(p_de->size);
}

static int pfs_fat_parse_short_name(const fat_dir_entry_t *p_de, char *out, int out_cap)
{
    uint8_t work[FAT_MSDOS_NAME];
    int base_len = 0;
    int ext_len = 0;
    int o = 0;

    if (NULL == p_de || NULL == out || out_cap <= 1)
    {
        return PFS_FAT_RET_EINVAL;
    }

    memcpy(work, p_de->name, FAT_MSDOS_NAME);
    if (0x05 == work[0])
    {
        work[0] = 0xE5;
    }

    for (int i = 0; i < 8 && ' ' != work[i]; i++)
    {
        base_len++;
    }
    for (int i = 8; i < 11 && ' ' != work[i]; i++)
    {
        ext_len++;
    }
    if (0 == base_len && 0 == ext_len)
    {
        out[0] = '\0';
        return 0;
    }

    for (int i = 0; i < base_len; i++)
    {
        char c = (char)work[i];

        if (0 != (p_de->lcase & FAT_CASE_LOWER_BASE) && c >= 'A' && c <= 'Z')
        {
            c = (char)(c - 'A' + 'a');
        }
        if (o + 1 >= out_cap)
        {
            return PFS_FAT_RET_ERANGE;
        }
        out[o++] = c;
    }
    if (ext_len > 0)
    {
        if (o + 1 >= out_cap)
        {
            return PFS_FAT_RET_ERANGE;
        }
        out[o++] = '.';
        for (int i = 0; i < ext_len; i++)
        {
            char c = (char)work[8 + i];

            if (0 != (p_de->lcase & FAT_CASE_LOWER_EXT) && c >= 'A' && c <= 'Z')
            {
                c = (char)(c - 'A' + 'a');
            }
            if (o + 1 >= out_cap)
            {
                return PFS_FAT_RET_ERANGE;
            }
            out[o++] = c;
        }
    }
    out[o] = '\0';
    return o;
}

static int pfs_fat_name_is_dot(const char *name)
{
    return (NULL != name && '.' == name[0] && '\0' == name[1]) ? 1 : 0;
}

static int pfs_fat_name_is_dotdot(const char *name)
{
    return (NULL != name && '.' == name[0] && '.' == name[1] && '\0' == name[2]) ? 1 : 0;
}

static void pfs_fat_fill_info_from_de(pfs_fat_sbi_t *p_sbi, const fat_dir_entry_t *p_de,
        pfs_fat_dir_entry_t *p_info)
{
    memset(p_info, 0, sizeof(*p_info));
    p_info->attr = p_de->attr;
    p_info->size = p_de->size;
    p_info->start_clu = pfs_fat_get_start(p_sbi, p_de);
    p_info->sfn_cpos = -1;
    if (0 != (p_de->attr & FAT_ATTR_DIR))
    {
        p_info->type = PFS_FAT_TYPE_DIR;
    }
    else
    {
        p_info->type = PFS_FAT_TYPE_FILE;
    }
}

// 从 cpos 起解析下一条可见文件/目录; 成功时推进 *p_cpos, 填 name+info
// 空名且 OK = 目录结束
int pfs_fat_readdir(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        int64_t *p_cpos, pfs_fat_dir_entry_t *p_de, char *utf8_name, int utf8_cap)
{
    if (NULL == p_sbi || NULL == p_cpos || NULL == p_de || NULL == utf8_name || utf8_cap <= 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    int64_t cpos = *p_cpos;
    utf8_name[0] = '\0';

    for (;;)
    {
        fat_dir_entry_t ent;
        int rc = pfs_fat_dir_read_at(p_sbi, start_clu, is_fixed_root, cpos, &ent);
        if (PFS_FAT_RET_ENOENT == rc)
        {
            utf8_name[0] = '\0';
            *p_cpos = cpos;
            return PFS_FAT_RET_OK;
        }
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }

        // name[0]==0: 目录结束
        if (0x00 == ent.name[0])
        {
            utf8_name[0] = '\0';
            *p_cpos = cpos;
            return PFS_FAT_RET_OK;
        }

        if (FAT_DELETED_FLAG == ent.name[0])
        {
            cpos += (int64_t)sizeof(fat_dir_entry_t);
            continue;
        }

        if (FAT_ATTR_EXT == ent.attr)
        {
            // LFN 序列 (对齐 fat_parse_long)
            fat_dir_slot_t *p_slot = (fat_dir_slot_t *)&ent;
            uint16_t unicode[FAT_MAX_UNI_CHARS];
            unsigned char nr_slots;
            unsigned char alias_checksum;
            unsigned char id = p_slot->id;
            unsigned char slots;
            unsigned char slot;
            int n;

            if (0 == (id & 0x40))
            {
                cpos += (int64_t)sizeof(fat_dir_entry_t);
                continue;
            }
            slots = (unsigned char)(id & ~0x40);
            if (0 == slots || slots > 20)
            {
                cpos += (int64_t)sizeof(fat_dir_entry_t);
                continue;
            }
            nr_slots = slots;
            alias_checksum = p_slot->alias_checksum;
            memset(unicode, 0, sizeof(unicode));
            slot = slots;

            for (;;)
            {
                int offset;

                slot--;
                offset = (int)slot * 13;
                pfs_fat_slot_copy_uni(unicode + offset, p_slot->name0_4, 5);
                pfs_fat_slot_copy_uni(unicode + offset + 5, p_slot->name5_10, 6);
                pfs_fat_slot_copy_uni(unicode + offset + 11, p_slot->name11_12, 2);
                if (0 != (p_slot->id & 0x40))
                {
                    unicode[offset + 13] = 0;
                }

                cpos += (int64_t)sizeof(fat_dir_entry_t);
                rc = pfs_fat_dir_read_at(p_sbi, start_clu, is_fixed_root, cpos, &ent);
                if (PFS_FAT_RET_ENOENT == rc || (PFS_FAT_RET_OK == rc && 0x00 == ent.name[0]))
                {
                    utf8_name[0] = '\0';
                    *p_cpos = cpos;
                    return PFS_FAT_RET_OK;
                }
                if (PFS_FAT_RET_OK != rc)
                {
                    return rc;
                }
                if (0 == slot)
                {
                    break;
                }
                p_slot = (fat_dir_slot_t *)&ent;
                if (FAT_ATTR_EXT != p_slot->attr)
                {
                    // 短项重解析
                    break;
                }
                if (((unsigned char)(p_slot->id & ~0x40) != slot) ||
                        p_slot->alias_checksum != alias_checksum)
                {
                    // 序列损坏, 跳过当前项继续扫
                    cpos += (int64_t)sizeof(fat_dir_entry_t);
                    continue;
                }
            }

            if (FAT_ATTR_EXT == ent.attr || FAT_DELETED_FLAG == ent.name[0] ||
                    0 != (ent.attr & FAT_ATTR_VOLUME))
            {
                cpos += (int64_t)sizeof(fat_dir_entry_t);
                continue;
            }
            pfs_fat_de_host(&ent);
            if (pfs_fat_checksum(ent.name) != alias_checksum)
            {
                nr_slots = 0;
            }

            if (nr_slots > 0)
            {
                n = pfs_fat_utf16_to_utf8(unicode, FAT_MAX_UNI_CHARS, utf8_name, utf8_cap);
                if (n < 0)
                {
                    cpos += (int64_t)sizeof(fat_dir_entry_t);
                    continue;
                }
            }
            else
            {
                n = pfs_fat_parse_short_name(&ent, utf8_name, utf8_cap);
                if (n < 0)
                {
                    cpos += (int64_t)sizeof(fat_dir_entry_t);
                    continue;
                }
            }
        }
        else
        {
            int n;

            if (0 != (ent.attr & FAT_ATTR_VOLUME) || FAT_IS_FREE(ent.name))
            {
                cpos += (int64_t)sizeof(fat_dir_entry_t);
                continue;
            }
            pfs_fat_de_host(&ent);
            n = pfs_fat_parse_short_name(&ent, utf8_name, utf8_cap);
            if (n < 0)
            {
                cpos += (int64_t)sizeof(fat_dir_entry_t);
                continue;
            }
        }

        // 跳过磁盘上的 . / .. (vfs 已合成)
        if (pfs_fat_name_is_dot(utf8_name) || pfs_fat_name_is_dotdot(utf8_name))
        {
            cpos += (int64_t)sizeof(fat_dir_entry_t);
            continue;
        }

        pfs_fat_fill_info_from_de(p_sbi, &ent, p_de);
        p_de->parent_clu = start_clu;
        p_de->parent_fixed_root = is_fixed_root ? 1 : 0;
        p_de->sfn_cpos = cpos;
        cpos += (int64_t)sizeof(fat_dir_entry_t);
        *p_cpos = cpos;
        return PFS_FAT_RET_OK;
    }
}

int pfs_fat_find_dir_entry(pfs_fat_sbi_t *p_sbi, uint32_t start_clu, int is_fixed_root,
        const char *name, pfs_fat_dir_entry_t *p_info)
{
    pfs_fat_dir_entry_t de;
    char utf8[FAT_LFN_LEN * 3 + 4];
    int64_t cpos = 0;

    if (NULL == p_sbi || NULL == name || '\0' == name[0] || NULL == p_info)
    {
        return PFS_FAT_RET_EINVAL;
    }

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
            return PFS_FAT_RET_ENOENT;
        }
        // 大小写不敏感 ASCII 比较 (LFN 亦按 UTF-8 字节 tolower A-Z)
        {
            const char *a = utf8;
            const char *b = name;
            int match = 1;

            while ('\0' != *a && '\0' != *b)
            {
                char ca = *a;
                char cb = *b;

                if (ca >= 'A' && ca <= 'Z')
                {
                    ca = (char)(ca - 'A' + 'a');
                }
                if (cb >= 'A' && cb <= 'Z')
                {
                    cb = (char)(cb - 'A' + 'a');
                }
                if (ca != cb)
                {
                    match = 0;
                    break;
                }
                a++;
                b++;
            }
            if (match && '\0' == *a && '\0' == *b)
            {
                *p_info = de;
                return PFS_FAT_RET_OK;
            }
        }
    }
}

// end
