// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 块位图分配/释放 (对照 linux-7.1 fs/ext4/balloc.c 子集)
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

#define OFF_FREE_BLOCKS_LO      0x00Cu
#define OFF_FREE_BLOCKS_HI      0x158u

static uint32_t pfs_ext4_gd_free_blocks_get(const pfs_ext4_sbi_t *p_sbi,
        const pfs_ext4_group_desc_t *p_gd)
{
    uint32_t n = p_gd->bg_free_blocks_count_lo;
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        n |= ((uint32_t)p_gd->bg_free_blocks_count_hi << 16);
    }
    return n;
}

static void pfs_ext4_gd_free_blocks_set(const pfs_ext4_sbi_t *p_sbi,
        pfs_ext4_group_desc_t *p_gd, uint32_t n)
{
    p_gd->bg_free_blocks_count_lo = (uint16_t)(n & 0xFFFFu);
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        p_gd->bg_free_blocks_count_hi = (uint16_t)(n >> 16);
    }
}

static int pfs_ext4_sb_dec_free_blocks(pfs_ext4_sbi_t *p_sbi)
{
    uint32_t n;

    if (0u == p_sbi->free_blocks_count)
    {
        return PFS_EXT4_RET_ENOSPC;
    }
    n = p_sbi->free_blocks_count - 1u;
    p_sbi->free_blocks_count = n;
    pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_BLOCKS_LO, n);
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_64BIT))
    {
        pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_BLOCKS_HI, 0u);
    }
    return pfs_ext4_sync_sb(p_sbi);
}

static int pfs_ext4_sb_inc_free_blocks(pfs_ext4_sbi_t *p_sbi)
{
    uint32_t n;

    n = p_sbi->free_blocks_count + 1u;
    p_sbi->free_blocks_count = n;
    pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_BLOCKS_LO, n);
    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_64BIT))
    {
        pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_BLOCKS_HI, 0u);
    }
    return pfs_ext4_sync_sb(p_sbi);
}

static uint32_t pfs_ext4_group_num_blocks(const pfs_ext4_sbi_t *p_sbi, uint32_t group)
{
    uint64_t start;
    uint64_t end;

    start = (uint64_t)group * (uint64_t)p_sbi->blocks_per_group +
            (uint64_t)p_sbi->first_data_block;
    end = start + (uint64_t)p_sbi->blocks_per_group;
    if (end > p_sbi->blocks_count)
    {
        end = p_sbi->blocks_count;
    }
    if (end <= start)
    {
        return 0u;
    }
    return (uint32_t)(end - start);
}

static int pfs_ext4_bitmap_test(const uint8_t *p_bmp, uint32_t bit)
{
    return (0u != (p_bmp[bit >> 3] & (uint8_t)(1u << (bit & 7u)))) ? 1 : 0;
}

static void pfs_ext4_bitmap_set(uint8_t *p_bmp, uint32_t bit)
{
    p_bmp[bit >> 3] = (uint8_t)(p_bmp[bit >> 3] | (uint8_t)(1u << (bit & 7u)));
}

static void pfs_ext4_bitmap_clear(uint8_t *p_bmp, uint32_t bit)
{
    p_bmp[bit >> 3] = (uint8_t)(p_bmp[bit >> 3] & (uint8_t)(~(uint8_t)(1u << (bit & 7u))));
}

int pfs_ext4_balloc_new(pfs_ext4_sbi_t *p_sbi, uint64_t *p_block)
{
    pfs_ext4_group_desc_t gd;
    uint8_t *p_bmp;
    uint32_t bits;
    uint64_t bmp_blk;
    uint64_t block;

    if (NULL == p_sbi || NULL == p_block)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    p_bmp = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_bmp)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    for (uint32_t group = 0; group < p_sbi->groups_count; group++)
    {
        int rc = pfs_ext4_gd_get(p_sbi, group, &gd);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_bmp);
            return rc;
        }
        if (0u != (gd.bg_flags & PFS_EXT4_BG_BLOCK_UNINIT))
        {
            continue;
        }
        if (0u == pfs_ext4_gd_free_blocks_get(p_sbi, &gd))
        {
            continue;
        }

        bmp_blk = pfs_ext4_gd_block_bitmap(p_sbi, &gd);
        rc = pfs_ext4_read_block(p_sbi, bmp_blk, p_bmp);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_bmp);
            return rc;
        }

        bits = pfs_ext4_group_num_blocks(p_sbi, group);
        for (uint32_t bit = 0; bit < bits; bit++)
        {
            if (pfs_ext4_bitmap_test(p_bmp, bit))
            {
                continue;
            }
            block = (uint64_t)group * (uint64_t)p_sbi->blocks_per_group +
                    (uint64_t)p_sbi->first_data_block + (uint64_t)bit;
            if (block >= p_sbi->blocks_count)
            {
                break;
            }

            pfs_ext4_bitmap_set(p_bmp, bit);
            rc = pfs_ext4_write_block(p_sbi, bmp_blk, p_bmp);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }

            pfs_ext4_gd_free_blocks_set(p_sbi, &gd,
                    pfs_ext4_gd_free_blocks_get(p_sbi, &gd) - 1u);
            rc = pfs_ext4_gd_put(p_sbi, group, &gd);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }
            rc = pfs_ext4_sb_dec_free_blocks(p_sbi);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }

            *p_block = block;
            pfs_free(p_bmp);
            return PFS_EXT4_RET_OK;
        }
    }

    pfs_free(p_bmp);
    return PFS_EXT4_RET_ENOSPC;
}

int pfs_ext4_balloc_free(pfs_ext4_sbi_t *p_sbi, uint64_t block)
{
    pfs_ext4_group_desc_t gd;
    uint8_t *p_bmp;
    uint32_t group;
    uint32_t bit;
    uint64_t rel;
    uint64_t bmp_blk;

    if (NULL == p_sbi)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (block < (uint64_t)p_sbi->first_data_block || block >= p_sbi->blocks_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    rel = block - (uint64_t)p_sbi->first_data_block;
    group = (uint32_t)(rel / (uint64_t)p_sbi->blocks_per_group);
    bit = (uint32_t)(rel % (uint64_t)p_sbi->blocks_per_group);
    if (group >= p_sbi->groups_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    int rc = pfs_ext4_gd_get(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0u != (gd.bg_flags & PFS_EXT4_BG_BLOCK_UNINIT))
    {
        return PFS_EXT4_RET_EINVAL;
    }

    p_bmp = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_bmp)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    bmp_blk = pfs_ext4_gd_block_bitmap(p_sbi, &gd);
    rc = pfs_ext4_read_block(p_sbi, bmp_blk, p_bmp);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_bmp);
        return rc;
    }
    if (!pfs_ext4_bitmap_test(p_bmp, bit))
    {
        pfs_free(p_bmp);
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_bitmap_clear(p_bmp, bit);
    rc = pfs_ext4_write_block(p_sbi, bmp_blk, p_bmp);
    pfs_free(p_bmp);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext4_gd_free_blocks_set(p_sbi, &gd, pfs_ext4_gd_free_blocks_get(p_sbi, &gd) + 1u);
    rc = pfs_ext4_gd_put(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ext4_sb_inc_free_blocks(p_sbi);
}

// end
