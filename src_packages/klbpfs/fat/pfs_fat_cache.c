// Doc Encode : UTF-8 BOM, Unix(LF)
// FAT 缓存适配: dir_cache(簇) / root_sec_cache(固定根扇区) / fat_cache(FAT 表扇区)
// 约定: *_cache_init 仅分配槽内存; *_cache_get 命中/缺槽时才读盘 (对齐 exFAT)
#include "fat/pfs_fat_internal.h"
#include "pfs_compiler.h"
#include "pfs_blkio.h"

// ---------------------------------------------------------------------------
// dir_cache: FAT32 根 + 子目录 (按簇)
// ---------------------------------------------------------------------------

int pfs_fat_dir_cache_init(pfs_cache_cluster_t *p_dir_cache, pfs_fat_sbi_t *p_sbi,
        uint32_t cluster_size)
{
    if (NULL == p_dir_cache || NULL == p_sbi || 0 == cluster_size)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (0 != p_sbi->bs.cluster_size && cluster_size != p_sbi->bs.cluster_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    return pfs_cache_cluster_init(p_dir_cache, p_sbi->p_blkio, cluster_size);
}

int pfs_fat_dir_cache_deinit(pfs_cache_cluster_t *p_dir_cache)
{
    return pfs_cache_cluster_deinit(p_dir_cache);
}

pfs_cache_cluster_slot_t *pfs_fat_dir_cache_get(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id)
{
    if (NULL == p_dir_cache)
    {
        return NULL;
    }

    pfs_fat_sbi_t *p_sbi = pfs_container_of(p_dir_cache, pfs_fat_sbi_t, dir_cache);
    if (!pfs_fat_is_valid_cluster(p_sbi, clu_id))
    {
        return NULL;
    }
    int64_t off = pfs_fat_cluster_to_byte(p_sbi, clu_id);
    return pfs_cache_cluster_get(p_dir_cache, clu_id, off);
}

void pfs_fat_dir_cache_invalidate(pfs_cache_cluster_t *p_dir_cache, uint32_t clu_id)
{
    pfs_cache_cluster_invalidate(p_dir_cache, clu_id);
}

void pfs_fat_dir_cache_invalidate_all(pfs_cache_cluster_t *p_dir_cache)
{
    pfs_cache_cluster_invalidate_all(p_dir_cache);
}

// ---------------------------------------------------------------------------
// root_sec_cache: FAT12/16 固定根目录区 (按卷内相对扇区 LBA)
// ---------------------------------------------------------------------------

int pfs_fat_root_sec_cache_init(pfs_cache_sector_t *p_root_sec_cache, pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_root_sec_cache || NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t sector_size = p_sbi->bs.sector_size;
    if (0 == sector_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    return pfs_cache_sector_init(p_root_sec_cache, p_sbi->p_blkio, sector_size);
}

int pfs_fat_root_sec_cache_deinit(pfs_cache_sector_t *p_root_sec_cache)
{
    return pfs_cache_sector_deinit(p_root_sec_cache);
}

pfs_cache_sector_slot_t *pfs_fat_root_sec_cache_get(pfs_cache_sector_t *p_root_sec_cache,
        uint64_t sector_id)
{
    if (NULL == p_root_sec_cache)
    {
        return NULL;
    }

    pfs_fat_sbi_t *p_sbi = pfs_container_of(p_root_sec_cache, pfs_fat_sbi_t, root_sec_cache);
    if (0 == p_sbi->root_sec_cache_on)
    {
        return NULL;
    }
    const pfs_fat_bs_t *p_bs = PFS_FAT_SBI_BS(p_sbi);
    // 固定根: [dir_start, dir_start + rootdir_sectors)
    if (sector_id < (uint64_t)p_bs->dir_start ||
            sector_id >= (uint64_t)p_bs->dir_start + (uint64_t)p_bs->rootdir_sectors)
    {
        return NULL;
    }
    int64_t off = pfs_fat_sector_to_byte(p_sbi, (int64_t)sector_id);
    return pfs_cache_sector_get(p_root_sec_cache, sector_id, off);
}

void pfs_fat_root_sec_cache_invalidate(pfs_cache_sector_t *p_root_sec_cache, uint64_t sector_id)
{
    pfs_cache_sector_invalidate(p_root_sec_cache, sector_id);
}

void pfs_fat_root_sec_cache_invalidate_all(pfs_cache_sector_t *p_root_sec_cache)
{
    pfs_cache_sector_invalidate_all(p_root_sec_cache);
}

// ---------------------------------------------------------------------------
// fat_cache: FAT1 表扇区 (init=填几何+分配槽; get=按需加载; 嵌在 sbi->fat.fat_cache)
// ---------------------------------------------------------------------------

int pfs_fat_fat_cache_init(pfs_cache_sector_t *p_fat_cache, pfs_fat_sbi_t *p_sbi)
{
    if (NULL == p_fat_cache || NULL == p_sbi)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (NULL == p_sbi->p_blkio)
    {
        return PFS_FAT_RET_EINVAL;
    }
    if (0 == p_sbi->bs.fat_length || 0 == p_sbi->bs.sector_size)
    {
        return PFS_FAT_RET_EINVAL;
    }

    uint32_t sector_size = p_sbi->bs.sector_size;
    p_sbi->fat.start_sector = p_sbi->bs.fat_start;
    p_sbi->fat.num_sectors = p_sbi->bs.fat_length;
    p_sbi->fat.size = (uint64_t)p_sbi->bs.fat_length * (uint64_t)sector_size;
    p_sbi->fat.fat_bits = p_sbi->bs.fat_bits;
    p_sbi->fat.valid = false;

    int rc = pfs_cache_sector_init(p_fat_cache, p_sbi->p_blkio, sector_size);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    p_sbi->fat.valid = true;
    return PFS_FAT_RET_OK;
}

int pfs_fat_fat_cache_deinit(pfs_cache_sector_t *p_fat_cache)
{
    return pfs_cache_sector_deinit(p_fat_cache);
}

pfs_cache_sector_slot_t *pfs_fat_fat_cache_get(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id)
{
    if (NULL == p_fat_cache)
    {
        return NULL;
    }

    pfs_fat_sbi_t *p_sbi = pfs_container_of(p_fat_cache, pfs_fat_sbi_t, fat.fat_cache);
    if (!p_sbi->fat.valid || 0 == p_sbi->fat.num_sectors)
    {
        return NULL;
    }
    if (sector_id < p_sbi->fat.start_sector ||
            sector_id >= p_sbi->fat.start_sector + (uint64_t)p_sbi->fat.num_sectors)
    {
        return NULL;
    }
    int64_t off = pfs_fat_sector_to_byte(p_sbi, (int64_t)sector_id);
    return pfs_cache_sector_get(p_fat_cache, sector_id, off);
}

void pfs_fat_fat_cache_invalidate(pfs_cache_sector_t *p_fat_cache, uint64_t sector_id)
{
    pfs_cache_sector_invalidate(p_fat_cache, sector_id);
}

void pfs_fat_fat_cache_invalidate_all(pfs_cache_sector_t *p_fat_cache)
{
    pfs_cache_sector_invalidate_all(p_fat_cache);
}

// end
