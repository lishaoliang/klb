// Doc Encode : UTF-8 BOM, Unix(LF)
// 分配位图 (linux-7.1 fs/exfat/balloc.c + fatent alloc/free)
// 位图数据走 sbi->bitmap.bmp_cache: mount 只分配槽, 读写时 bmp_cache_get 按需加载
// load_bitmap 不预载整张位图 (与 linux vol_amap 全量 pin 不同)
#include "exfat/pfs_exfat_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

int pfs_exfat_load_bitmap(pfs_exfat_sbi_t *p_sbi)
{
    (void)p_sbi;
    // 元数据已在 parse_root; 缓存池已在 mount init; 无需再全量加载
    return PFS_EXFAT_RET_OK;
}

void pfs_exfat_free_bitmap(pfs_exfat_sbi_t *p_sbi)
{
    (void)p_sbi;
    // bmp_cache 由 mount_release / umount 释放
}

// 簇号 -> 位图内 bit 下标 (对齐 CLUSTER_TO_BITMAP_ENT)
static uint32_t pfs_exfat_clu_to_bitmap_ent(uint32_t clu)
{
    return clu - EXFAT_RESERVED_CLUSTERS;
}

// 位图字节偏移对应的位图簇号 (假定位图为连续簇; 与常见 mkfs 一致)
static int pfs_exfat_bitmap_clu_for_ent(pfs_exfat_sbi_t *p_sbi, uint32_t ent_idx,
        uint32_t *p_bmp_clu, uint32_t *p_byte_in_clu)
{
    if (NULL == p_sbi || NULL == p_bmp_clu || NULL == p_byte_in_clu || !p_sbi->bitmap.valid)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    uint64_t byte_off = (uint64_t)ent_idx >> 3;
    if (byte_off >= p_sbi->bitmap.size)
    {
        return PFS_EXFAT_RET_ERANGE;
    }
    uint32_t clu_off = (uint32_t)(byte_off / (uint64_t)p_bs->cluster_size);
    *p_byte_in_clu = (uint32_t)(byte_off % (uint64_t)p_bs->cluster_size);

    uint32_t bmp_clu = p_sbi->bitmap.start_clu;
    for (uint32_t i = 0; i < clu_off; i++)
    {
        int rc = pfs_exfat_ent_get(p_sbi, bmp_clu, &bmp_clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (EXFAT_EOF_CLUSTER == bmp_clu || !pfs_exfat_is_valid_cluster(p_sbi, bmp_clu))
        {
            return PFS_EXFAT_RET_EIO;
        }
    }
    *p_bmp_clu = bmp_clu;
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_bitmap_flush_clu(pfs_exfat_sbi_t *p_sbi, uint32_t bmp_clu,
        const uint8_t *p_data)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    int64_t off = pfs_exfat_cluster_to_byte(p_sbi, bmp_clu);
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_data, (int)p_bs->cluster_size, off);
    if (0 > rc)
    {
        return PFS_EXFAT_RET_EIO;
    }
    else if (rc != (int)p_bs->cluster_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_set_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    if (NULL == p_sbi || !pfs_exfat_is_valid_cluster(p_sbi, clu))
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    uint32_t ent_idx = pfs_exfat_clu_to_bitmap_ent(clu);
    uint32_t bmp_clu = 0;
    uint32_t byte_in_clu = 0;
    int rc = pfs_exfat_bitmap_clu_for_ent(p_sbi, ent_idx, &bmp_clu, &byte_in_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    uint32_t bit = ent_idx & 7u;
    pfs_cache_cluster_slot_t *p_slot = pfs_exfat_bmp_cache_get(&p_sbi->bitmap.bmp_cache, bmp_clu);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_EXFAT_RET_EIO;
    }
    p_slot->p_data[byte_in_clu] = (uint8_t)(p_slot->p_data[byte_in_clu] | (uint8_t)(1u << bit));
    return pfs_exfat_bitmap_flush_clu(p_sbi, bmp_clu, p_slot->p_data);
}

int pfs_exfat_clear_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    if (NULL == p_sbi || !pfs_exfat_is_valid_cluster(p_sbi, clu))
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    uint32_t ent_idx = pfs_exfat_clu_to_bitmap_ent(clu);
    uint32_t bmp_clu = 0;
    uint32_t byte_in_clu = 0;
    int rc = pfs_exfat_bitmap_clu_for_ent(p_sbi, ent_idx, &bmp_clu, &byte_in_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    uint32_t bit = ent_idx & 7u;
    pfs_cache_cluster_slot_t *p_slot = pfs_exfat_bmp_cache_get(&p_sbi->bitmap.bmp_cache, bmp_clu);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_EXFAT_RET_EIO;
    }
    if (0 == (p_slot->p_data[byte_in_clu] & (uint8_t)(1u << bit)))
    {
        return PFS_EXFAT_RET_EIO;
    }
    p_slot->p_data[byte_in_clu] = (uint8_t)(p_slot->p_data[byte_in_clu] & (uint8_t)~(1u << bit));
    return pfs_exfat_bitmap_flush_clu(p_sbi, bmp_clu, p_slot->p_data);
}

int pfs_exfat_test_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t clu, int *p_used)
{
    if (NULL == p_sbi || NULL == p_used || !pfs_exfat_is_valid_cluster(p_sbi, clu))
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!p_sbi->bitmap.valid)
    {
        return PFS_EXFAT_RET_EBUSY;
    }

    uint32_t ent_idx = pfs_exfat_clu_to_bitmap_ent(clu);
    uint32_t bmp_clu = 0;
    uint32_t byte_in_clu = 0;
    int rc = pfs_exfat_bitmap_clu_for_ent(p_sbi, ent_idx, &bmp_clu, &byte_in_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    uint32_t bit = ent_idx & 7u;
    pfs_cache_cluster_slot_t *p_slot = pfs_exfat_bmp_cache_get(&p_sbi->bitmap.bmp_cache, bmp_clu);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_EXFAT_RET_EIO;
    }
    *p_used = (0 != (p_slot->p_data[byte_in_clu] & (uint8_t)(1u << bit))) ? 1 : 0;
    return PFS_EXFAT_RET_OK;
}

uint32_t pfs_exfat_find_free_bitmap(pfs_exfat_sbi_t *p_sbi, uint32_t hint)
{
    if (NULL == p_sbi || !p_sbi->bitmap.valid)
    {
        return EXFAT_EOF_CLUSTER;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    uint32_t clu = hint;
    if (!pfs_exfat_is_valid_cluster(p_sbi, clu))
    {
        clu = EXFAT_FIRST_CLUSTER;
    }

    uint32_t scanned = 0;
    while (scanned < (p_bs->num_clusters - EXFAT_RESERVED_CLUSTERS))
    {
        int used = 0;
        int rc = pfs_exfat_test_bitmap(p_sbi, clu, &used);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return EXFAT_EOF_CLUSTER;
        }
        if (0 == used)
        {
            return clu;
        }
        scanned++;
        clu++;
        if (clu >= p_bs->num_clusters)
        {
            clu = EXFAT_FIRST_CLUSTER;
        }
    }
    return EXFAT_EOF_CLUSTER;
}

int pfs_exfat_alloc_cluster(pfs_exfat_sbi_t *p_sbi, uint32_t *p_clu)
{
    if (NULL == p_sbi || NULL == p_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    uint32_t new_clu = pfs_exfat_find_free_bitmap(p_sbi, EXFAT_FIRST_CLUSTER);
    if (EXFAT_EOF_CLUSTER == new_clu)
    {
        return PFS_EXFAT_RET_ENOSPC;
    }
    int rc = pfs_exfat_set_bitmap(p_sbi, new_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_exfat_ent_set(p_sbi, new_clu, EXFAT_EOF_CLUSTER);
    if (PFS_EXFAT_RET_OK != rc)
    {
        (void)pfs_exfat_clear_bitmap(p_sbi, new_clu);
        return rc;
    }
    *p_clu = new_clu;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_free_cluster_chain(pfs_exfat_sbi_t *p_sbi, uint32_t start, uint32_t num_clu,
        uint8_t flags)
{
    if (NULL == p_sbi)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }
    if (EXFAT_EOF_CLUSTER == start || EXFAT_FREE_CLUSTER == start || 0 == num_clu)
    {
        return PFS_EXFAT_RET_OK;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, start))
    {
        return PFS_EXFAT_RET_ERANGE;
    }

    if (EXFAT_ALLOC_NO_FAT_CHAIN == flags)
    {
        for (uint32_t i = 0; i < num_clu; i++)
        {
            uint32_t clu = start + i;
            if (!pfs_exfat_is_valid_cluster(p_sbi, clu))
            {
                return PFS_EXFAT_RET_EIO;
            }
            int rc = pfs_exfat_clear_bitmap(p_sbi, clu);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
            rc = pfs_exfat_ent_set(p_sbi, clu, EXFAT_FREE_CLUSTER);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
        return PFS_EXFAT_RET_OK;
    }

    uint32_t clu = start;
    uint32_t i = 0;
    while (pfs_exfat_is_valid_cluster(p_sbi, clu) && i < num_clu)
    {
        uint32_t next;
        int rc = pfs_exfat_ent_get(p_sbi, clu, &next);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_exfat_clear_bitmap(p_sbi, clu);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_exfat_ent_set(p_sbi, clu, EXFAT_FREE_CLUSTER);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        i++;
        if (EXFAT_EOF_CLUSTER == next || EXFAT_FREE_CLUSTER == next)
        {
            break;
        }
        clu = next;
    }
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_zero_cluster(pfs_exfat_sbi_t *p_sbi, uint32_t clu)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_is_valid_cluster(p_sbi, clu) || !pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (0 == p_bs->cluster_size)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }
    uint8_t *p_z = (uint8_t *)pfs_mallocz(p_bs->cluster_size);
    if (NULL == p_z)
    {
        return PFS_EXFAT_RET_ENOMEM;
    }
    int64_t off = pfs_exfat_cluster_to_byte(p_sbi, clu);
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_z, (int)p_bs->cluster_size, off);
    pfs_free(p_z);
    if (0 > rc)
    {
        return PFS_EXFAT_RET_EIO;
    }
    else if (rc != (int)p_bs->cluster_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    pfs_exfat_dir_cache_invalidate(&p_sbi->dir_cache, clu);
    return PFS_EXFAT_RET_OK;
}

// end
