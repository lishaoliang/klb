// Doc Encode : UTF-8 BOM, Unix(LF)
// FAT 表与簇链 (linux-7.1 fs/fat/fatent.c; 读路径走 sbi->fat.fat_cache)
#include "fat/pfs_fat_internal.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>

static bool pfs_fat_ent_is_eof_val(const pfs_fat_sbi_t *p_sbi, uint32_t content)
{
    if (pfs_fat_is_fat32(p_sbi))
    {
        return (content >= BAD_FAT32);
    }
    if (pfs_fat_is_fat16(p_sbi))
    {
        return (content >= BAD_FAT16);
    }
    return (content >= BAD_FAT12);
}

// 从 fat_cache 取 FAT 区内绝对字节 off 起的 1~4 字节 (可跨扇区, 供 FAT12)
static int pfs_fat_ent_read_bytes(pfs_fat_sbi_t *p_sbi, uint32_t fat_byte_off, uint8_t *dst, int len)
{
    if (NULL == p_sbi || NULL == dst || len <= 0 || len > 4)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!p_sbi->fat.valid)
    {
        return PFS_FAT_RET_EBUSY;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    uint32_t sector_size = p_bs->sector_size;
    if (0 == sector_size)
    {
        return PFS_FAT_RET_EBADMSG;
    }
    if ((uint64_t)fat_byte_off + (uint64_t)len > p_sbi->fat.size)
    {
        return PFS_FAT_RET_ERANGE;
    }

    int got = 0;
    while (got < len)
    {
        uint64_t sector_id = p_sbi->fat.start_sector +
                ((uint64_t)fat_byte_off + (uint64_t)got) / (uint64_t)sector_size;
        uint32_t byte_in_sec = (uint32_t)(((uint64_t)fat_byte_off + (uint64_t)got) % (uint64_t)sector_size);
        pfs_cache_sector_slot_t *p_slot = pfs_fat_fat_cache_get(&p_sbi->fat.fat_cache, sector_id);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_FAT_RET_EIO;
        }
        int n = (int)(sector_size - byte_in_sec);
        if (n > (len - got))
        {
            n = len - got;
        }
        memcpy(dst + got, p_slot->p_data + byte_in_sec, (size_t)n);
        got += n;
    }
    return PFS_FAT_RET_OK;
}

int pfs_fat_ent_get(pfs_fat_sbi_t *p_sbi, uint32_t loc, uint32_t *p_content)
{
    if (NULL == p_sbi || NULL == p_content || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_is_valid_cluster(p_sbi, loc))
    {
        return PFS_FAT_RET_ERANGE;
    }
    if (!p_sbi->fat.valid)
    {
        return PFS_FAT_RET_EBUSY;
    }

    uint8_t buf[4];
    uint32_t fat_byte_off;
    int need;
    if (pfs_fat_is_fat32(p_sbi))
    {
        fat_byte_off = loc << 2;
        need = 4;
    }
    else if (pfs_fat_is_fat16(p_sbi))
    {
        fat_byte_off = loc << 1;
        need = 2;
    }
    else
    {
        // FAT12: 1.5 字节/项; 可能跨扇区
        fat_byte_off = loc + (loc >> 1);
        need = 2;
    }

    memset(buf, 0, sizeof(buf));
    int rc = pfs_fat_ent_read_bytes(p_sbi, fat_byte_off, buf, need);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t content;
    if (pfs_fat_is_fat32(p_sbi))
    {
        content = pfs_read_le32(buf) & 0x0FFFFFFFu;
    }
    else if (pfs_fat_is_fat16(p_sbi))
    {
        content = pfs_read_le16(buf);
    }
    else
    {
        content = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
        if (0 != (loc & 1U))
        {
            content >>= 4;
        }
        else
        {
            content &= 0x0FFFu;
        }
    }

    if (pfs_fat_ent_is_eof_val(p_sbi, content))
    {
        content = FAT_ENT_EOF;
    }
    *p_content = content;
    return PFS_FAT_RET_OK;
}

// 写 FAT 区相对 FAT1 的字节 off; 同步写到全部 FAT 副本; 写后 invalidate fat_cache
static int pfs_fat_ent_write_bytes(pfs_fat_sbi_t *p_sbi, uint32_t fat_byte_off,
        const uint8_t *src, int len)
{
    if (NULL == p_sbi || NULL == src || len <= 0 || len > 4 || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!p_sbi->fat.valid || !pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    uint32_t sector_size = p_bs->sector_size;
    uint32_t fats = p_bs->fats;
    if (0 == sector_size || sector_size > PFS_FAT_SEC_BUF_MAX || 0 == fats)
    {
        return PFS_FAT_RET_EBADMSG;
    }
    if ((uint64_t)fat_byte_off + (uint64_t)len > p_sbi->fat.size)
    {
        return PFS_FAT_RET_ERANGE;
    }

    uint8_t sec[PFS_FAT_SEC_BUF_MAX];
    int got = 0;
    while (got < len)
    {
        uint64_t sector_in_fat = ((uint64_t)fat_byte_off + (uint64_t)got) / (uint64_t)sector_size;
        uint32_t byte_in_sec = (uint32_t)(((uint64_t)fat_byte_off + (uint64_t)got) % (uint64_t)sector_size);
        int n = (int)(sector_size - byte_in_sec);
        if (n > (len - got))
        {
            n = len - got;
        }

        for (uint32_t fi = 0; fi < fats; fi++)
        {
            uint64_t sector_id = (uint64_t)p_bs->fat_start + (uint64_t)fi * (uint64_t)p_bs->fat_length +
                    sector_in_fat;
            int64_t off = pfs_fat_sector_to_byte(p_sbi, (int64_t)sector_id);
            int rc = pfs_blkio_pread(p_sbi->p_blkio, sec, (int)sector_size, off);
            if (0 > rc)
            {
                return PFS_FAT_RET_EIO;
            }
            else if (rc != (int)sector_size)
            {
                return PFS_FAT_RET_EIO;
            }
            memcpy(sec + byte_in_sec, src + got, (size_t)n);
            rc = pfs_blkio_pwrite(p_sbi->p_blkio, sec, (int)sector_size, off);
            if (0 > rc)
            {
                return PFS_FAT_RET_EIO;
            }
            else if (rc != (int)sector_size)
            {
                return PFS_FAT_RET_EIO;
            }
            pfs_fat_fat_cache_invalidate(&p_sbi->fat.fat_cache, sector_id);
        }
        got += n;
    }
    return PFS_FAT_RET_OK;
}

int pfs_fat_ent_set(pfs_fat_sbi_t *p_sbi, uint32_t loc, uint32_t content)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_is_valid_cluster(p_sbi, loc))
    {
        return PFS_FAT_RET_ERANGE;
    }
    if (!p_sbi->fat.valid || !pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    uint8_t buf[4];
    uint32_t fat_byte_off;
    uint32_t write_val = content;
    int need;
    if (FAT_ENT_EOF == content)
    {
        if (pfs_fat_is_fat32(p_sbi))
        {
            write_val = EOF_FAT32;
        }
        else if (pfs_fat_is_fat16(p_sbi))
        {
            write_val = EOF_FAT16;
        }
        else
        {
            write_val = EOF_FAT12;
        }
    }

    if (pfs_fat_is_fat32(p_sbi))
    {
        fat_byte_off = loc << 2;
        need = 4;
        // 保留高 4 位 (对齐 linux fat_ent_write)
        int rc = pfs_fat_ent_read_bytes(p_sbi, fat_byte_off, buf, 4);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        write_val = (pfs_read_le32(buf) & 0xF0000000u) | (write_val & 0x0FFFFFFFu);
        pfs_write_le32(buf, write_val);
    }
    else if (pfs_fat_is_fat16(p_sbi))
    {
        fat_byte_off = loc << 1;
        need = 2;
        pfs_write_le16(buf, (uint16_t)(write_val & 0xFFFFu));
    }
    else
    {
        uint32_t old;

        fat_byte_off = loc + (loc >> 1);
        need = 2;
        memset(buf, 0, sizeof(buf));
        int rc = pfs_fat_ent_read_bytes(p_sbi, fat_byte_off, buf, 2);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        old = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8);
        write_val &= 0x0FFFu;
        if (0 != (loc & 1U))
        {
            old = (old & 0x000Fu) | (write_val << 4);
        }
        else
        {
            old = (old & 0xF000u) | write_val;
        }
        buf[0] = (uint8_t)(old & 0xFFu);
        buf[1] = (uint8_t)((old >> 8) & 0xFFu);
    }

    return pfs_fat_ent_write_bytes(p_sbi, fat_byte_off, buf, need);
}

// 从 hint 起找空闲簇; 成功写入 *p_clu; 无空间 ENOSPC
int pfs_fat_alloc_cluster(pfs_fat_sbi_t *p_sbi, uint32_t hint, uint32_t *p_clu)
{
    if (NULL == p_sbi || NULL == p_clu)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    uint32_t start = hint;
    if (!pfs_fat_is_valid_cluster(p_sbi, start))
    {
        start = (0 != p_sbi->prev_free && pfs_fat_is_valid_cluster(p_sbi, p_sbi->prev_free))
                ? p_sbi->prev_free : FAT_START_ENT;
    }

    uint32_t scanned = 0;
    uint32_t clu = start;
    while (scanned < (p_bs->max_cluster - FAT_START_ENT))
    {
        uint32_t content;
        int rc = pfs_fat_ent_get(p_sbi, clu, &content);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_FREE == content)
        {
            rc = pfs_fat_ent_set(p_sbi, clu, FAT_ENT_EOF);
            if (PFS_FAT_RET_OK != rc)
            {
                return rc;
            }
            p_sbi->prev_free = clu;
            if (p_sbi->free_clus_valid && p_sbi->free_clusters > 0)
            {
                p_sbi->free_clusters--;
            }
            *p_clu = clu;
            return PFS_FAT_RET_OK;
        }
        scanned++;
        clu++;
        if (clu >= p_bs->max_cluster)
        {
            clu = FAT_START_ENT;
        }
    }
    return PFS_FAT_RET_ENOSPC;
}

// 释放从 start 起的整条簇链 (含 start); start 为 0/EOF 则空操作
int pfs_fat_free_cluster_chain(pfs_fat_sbi_t *p_sbi, uint32_t start)
{
    if (NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EPERM;
    }
    if (0 == start || FAT_ENT_FREE == start || FAT_ENT_EOF == start)
    {
        return PFS_FAT_RET_OK;
    }
    if (!pfs_fat_is_valid_cluster(p_sbi, start))
    {
        return PFS_FAT_RET_ERANGE;
    }

    uint32_t clu = start;
    while (pfs_fat_is_valid_cluster(p_sbi, clu))
    {
        uint32_t next;
        int rc = pfs_fat_ent_get(p_sbi, clu, &next);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_fat_ent_set(p_sbi, clu, FAT_ENT_FREE);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (p_sbi->free_clus_valid)
        {
            p_sbi->free_clusters++;
        }
        if (FAT_ENT_EOF == next || FAT_ENT_FREE == next)
        {
            break;
        }
        clu = next;
    }
    return PFS_FAT_RET_OK;
}

// 将簇内容清零 (整簇写 0)
int pfs_fat_zero_cluster(pfs_fat_sbi_t *p_sbi, uint32_t clu)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (!pfs_fat_is_valid_cluster(p_sbi, clu) || !pfs_fat_sbi_can_write(p_sbi))
    {
        return PFS_FAT_RET_EINVAL;
    }

    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    if (0 == p_bs->cluster_size)
    {
        return PFS_FAT_RET_EBADMSG;
    }
    uint8_t *p_z = (uint8_t *)pfs_mallocz(p_bs->cluster_size);
    if (NULL == p_z)
    {
        return PFS_FAT_RET_ENOMEM;
    }
    int64_t off = pfs_fat_cluster_to_byte(p_sbi, clu);
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_z, (int)p_bs->cluster_size, off);
    pfs_free(p_z);
    if (0 > rc)
    {
        return PFS_FAT_RET_EIO;
    }
    else if (rc != (int)p_bs->cluster_size)
    {
        return PFS_FAT_RET_EIO;
    }
    pfs_fat_dir_cache_invalidate(&p_sbi->dir_cache, clu);
    return PFS_FAT_RET_OK;
}

int pfs_fat_get_next_cluster(pfs_fat_sbi_t *p_sbi, uint32_t *p_clu)
{
    if (NULL == p_sbi || NULL == p_clu)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t next;
    int rc = pfs_fat_ent_get(p_sbi, *p_clu, &next);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    *p_clu = next;
    return PFS_FAT_RET_OK;
}

int pfs_fat_cluster_walk(pfs_fat_sbi_t *p_sbi, uint32_t *p_clu, uint32_t step)
{
    if (NULL == p_sbi || NULL == p_clu)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t clu = *p_clu;
    for (uint32_t i = 0; i < step; i++)
    {
        if (FAT_ENT_EOF == clu || FAT_ENT_FREE == clu || !pfs_fat_is_valid_cluster(p_sbi, clu))
        {
            return PFS_FAT_RET_EIO;
        }
        int rc = pfs_fat_get_next_cluster(p_sbi, &clu);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        if (FAT_ENT_EOF == clu)
        {
            *p_clu = FAT_ENT_EOF;
            return PFS_FAT_RET_OK;
        }
    }
    *p_clu = clu;
    return PFS_FAT_RET_OK;
}

// end
