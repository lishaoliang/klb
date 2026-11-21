// Doc Encode : UTF-8 BOM, Unix(LF)
// FAT 表与簇链 (linux-7.1 fs/exfat/fatent.c)
#include "exfat/pfs_exfat_internal.h"

int pfs_exfat_ent_get(pfs_exfat_sbi_t *p_sbi, uint32_t loc, uint32_t *p_content)
{
    if (NULL == p_sbi || NULL == p_content || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, loc))
    {
        return PFS_EXFAT_RET_ERANGE;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    int64_t off = pfs_exfat_sector_to_byte(p_sbi, (int64_t)p_bs->FAT1_start_sector)
            + ((int64_t)loc << 2);
    uint8_t buf[4];
    // Win 卷须扇区对齐读; 经 pread_at 做扇区 RMW
    int rc = pfs_exfat_pread_at(p_sbi, buf, (int)sizeof(buf), off);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t content = pfs_read_le32(buf);
    // 对齐内核: 保留簇号映射为 EOF, 便于簇链遍历
    if (content > EXFAT_BAD_CLUSTER)
    {
        content = EXFAT_EOF_CLUSTER;
    }
    *p_content = content;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_ent_set(pfs_exfat_sbi_t *p_sbi, uint32_t loc, uint32_t content)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, loc))
    {
        return PFS_EXFAT_RET_ERANGE;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi) || !p_sbi->fat.valid)
    {
        return PFS_EXFAT_RET_EPERM;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    uint32_t sector_size = 1u << p_bs->boot.sect_size_bits;
    if (0 == sector_size)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    int64_t off = pfs_exfat_sector_to_byte(p_sbi, (int64_t)p_bs->FAT1_start_sector)
            + ((int64_t)loc << 2);
    uint8_t buf[4];
    pfs_write_le32(buf, content);
    // Win 卷须扇区对齐写; 经 pwrite_at 做扇区 RMW (勿直接写 4 字节)
    int rc = pfs_exfat_pwrite_at(p_sbi, buf, (int)sizeof(buf), off);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    uint64_t sector_id = p_bs->FAT1_start_sector + (((uint64_t)loc << 2) / (uint64_t)sector_size);
    pfs_exfat_fat_cache_invalidate(&p_sbi->fat.fat_cache, sector_id);
    return PFS_EXFAT_RET_OK;
}

// end
