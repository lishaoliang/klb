// Doc Encode : UTF-8 BOM, Unix(LF)
// exFAT 三池缓存适配: dir / bitmap / FAT (委托 base/pfs_cache_*)
// 约定: *_cache_init 仅分配槽内存 (slot.valid=false); *_cache_get 命中/缺槽时才读盘
// upcase 不在此文件: mount 时 create_upcase_table 分配+展开加载, 此后只读至 umount
#include "exfat/pfs_exfat_internal.h"
#include "pfs_compiler.h"
#include "pfs_blkio.h"

// ---------------------------------------------------------------------------
// dir_cache: 目录项等元数据簇 (init=分配; get=按需加载)
// ---------------------------------------------------------------------------

int pfs_exfat_dir_cache_init(pfs_cache_cluster_t *p_dir_cache, pfs_exfat_sbi_t *p_sbi,
        uint32_t cluster_size)
{
    if (NULL == p_dir_cache || NULL == p_sbi || 0 == cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    // 与 boot 几何一致 (mount 路径传入 bs.cluster_size)
    if (0 != p_sbi->bs.cluster_size && cluster_size != p_sbi->bs.cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    return pfs_cache_cluster_init(p_dir_cache, p_sbi->p_blkio, cluster_size);
}

int pfs_exfat_dir_cache_deinit(pfs_cache_cluster_t *p_dir_cache)
{
    return pfs_cache_cluster_deinit(p_dir_cache);
}

pfs_cache_cluster_slot_t *pfs_exfat_dir_cache_get(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id)
{
    if (NULL == p_dir_cache)
    {
        return NULL;
    }

    pfs_exfat_sbi_t *p_sbi = pfs_container_of(p_dir_cache, pfs_exfat_sbi_t, dir_cache);
    if (!pfs_exfat_is_valid_cluster(p_sbi, clu_id))
    {
        return NULL;
    }
    int64_t off = pfs_exfat_cluster_to_byte(p_sbi, clu_id);
    return pfs_cache_cluster_get(p_dir_cache, clu_id, off);
}

void pfs_exfat_dir_cache_invalidate(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id)
{
    pfs_cache_cluster_invalidate(p_dir_cache, clu_id);
}

void pfs_exfat_dir_cache_invalidate_all(pfs_cache_cluster_t *p_dir_cache)
{
    pfs_cache_cluster_invalidate_all(p_dir_cache);
}

// ---------------------------------------------------------------------------
// bmp_cache: 分配位图高热簇 (init=分配; get=按需加载; 嵌在 sbi->bitmap.bmp_cache)
// ---------------------------------------------------------------------------

int pfs_exfat_bmp_cache_init(pfs_cache_cluster_t *p_bmp_cache, pfs_exfat_sbi_t *p_sbi,
        uint32_t cluster_size)
{
    if (NULL == p_bmp_cache || NULL == p_sbi || 0 == cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 != p_sbi->bs.cluster_size && cluster_size != p_sbi->bs.cluster_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    return pfs_cache_cluster_init(p_bmp_cache, p_sbi->p_blkio, cluster_size);
}

int pfs_exfat_bmp_cache_deinit(pfs_cache_cluster_t *p_bmp_cache)
{
    return pfs_cache_cluster_deinit(p_bmp_cache);
}

pfs_cache_cluster_slot_t *pfs_exfat_bmp_cache_get(pfs_cache_cluster_t *p_bmp_cache, uint32_t clu_id)
{
    if (NULL == p_bmp_cache)
    {
        return NULL;
    }

    pfs_exfat_sbi_t *p_sbi = pfs_container_of(p_bmp_cache, pfs_exfat_sbi_t, bitmap.bmp_cache);
    if (!pfs_exfat_is_valid_cluster(p_sbi, clu_id))
    {
        return NULL;
    }
    int64_t off = pfs_exfat_cluster_to_byte(p_sbi, clu_id);
    return pfs_cache_cluster_get(p_bmp_cache, clu_id, off);
}

void pfs_exfat_bmp_cache_invalidate(pfs_cache_cluster_t *p_bmp_cache, uint32_t clu_id)
{
    pfs_cache_cluster_invalidate(p_bmp_cache, clu_id);
}

void pfs_exfat_bmp_cache_invalidate_all(pfs_cache_cluster_t *p_bmp_cache)
{
    pfs_cache_cluster_invalidate_all(p_bmp_cache);
}

// ---------------------------------------------------------------------------
// fat_cache: FAT 扇区 (init=填几何+分配槽; get=按需加载; 嵌在 sbi->fat.fat_cache)
// ---------------------------------------------------------------------------

int pfs_exfat_fat_cache_init(pfs_cache_sector_t *p_fat_cache, pfs_exfat_sbi_t *p_sbi)
{
    if (NULL == p_fat_cache || NULL == p_sbi)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 == p_sbi->bs.num_FAT_sectors)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    uint32_t sector_size = 1U << p_sbi->bs.boot.sect_size_bits;
    if (0 == sector_size)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    p_sbi->fat.start_sector = p_sbi->bs.FAT1_start_sector;
    p_sbi->fat.num_sectors = p_sbi->bs.num_FAT_sectors;
    p_sbi->fat.size = (uint64_t)p_sbi->bs.num_FAT_sectors * (uint64_t)sector_size;
    p_sbi->fat.valid = false;

    int rc = pfs_cache_sector_init(p_fat_cache, p_sbi->p_blkio, sector_size);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    p_sbi->fat.valid = true;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_fat_cache_deinit(pfs_cache_sector_t *p_fat_cache)
{
    return pfs_cache_sector_deinit(p_fat_cache);
}

pfs_cache_sector_slot_t *pfs_exfat_fat_cache_get(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id)
{
    if (NULL == p_fat_cache)
    {
        return NULL;
    }

    pfs_exfat_sbi_t *p_sbi = pfs_container_of(p_fat_cache, pfs_exfat_sbi_t, fat.fat_cache);
    if (!p_sbi->fat.valid || 0 == p_sbi->fat.num_sectors)
    {
        return NULL;
    }
    // sector_id: 卷内相对扇区 LBA, 须落在 FAT1 范围内
    if (sector_id < p_sbi->fat.start_sector ||
            sector_id >= p_sbi->fat.start_sector + (uint64_t)p_sbi->fat.num_sectors)
    {
        return NULL;
    }
    int64_t off = pfs_exfat_sector_to_byte(p_sbi, (int64_t)sector_id);
    return pfs_cache_sector_get(p_fat_cache, sector_id, off);
}

void pfs_exfat_fat_cache_invalidate(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id)
{
    pfs_cache_sector_invalidate(p_fat_cache, sector_id);
}

void pfs_exfat_fat_cache_invalidate_all(pfs_cache_sector_t *p_fat_cache)
{
    pfs_cache_sector_invalidate_all(p_fat_cache);
}

// end
