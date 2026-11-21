// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 inode 位图分配/释放
#include "ext4/pfs_ext4_internal.h"

#include <stdbool.h>
#include <string.h>

#define OFF_FREE_INODES         0x010u

static uint32_t pfs_ext4_gd_free_inodes_get(const pfs_ext4_sbi_t *p_sbi,
        const pfs_ext4_group_desc_t *p_gd)
{
    uint32_t n = p_gd->bg_free_inodes_count_lo;
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        n |= ((uint32_t)p_gd->bg_free_inodes_count_hi << 16);
    }
    return n;
}

static void pfs_ext4_gd_free_inodes_set(const pfs_ext4_sbi_t *p_sbi,
        pfs_ext4_group_desc_t *p_gd, uint32_t n)
{
    p_gd->bg_free_inodes_count_lo = (uint16_t)(n & 0xFFFFu);
    if (p_sbi->desc_size >= PFS_EXT4_MIN_DESC_SIZE_64BIT)
    {
        p_gd->bg_free_inodes_count_hi = (uint16_t)(n >> 16);
    }
}

static int pfs_ext4_sb_dec_free_inodes(pfs_ext4_sbi_t *p_sbi)
{
    if (0u == p_sbi->free_inodes_count)
    {
        return PFS_EXT4_RET_ENOSPC;
    }
    uint32_t n = p_sbi->free_inodes_count - 1u;
    p_sbi->free_inodes_count = n;
    pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_INODES, n);
    return pfs_ext4_sync_sb(p_sbi);
}

static int pfs_ext4_sb_inc_free_inodes(pfs_ext4_sbi_t *p_sbi)
{
    uint32_t n = p_sbi->free_inodes_count + 1u;
    p_sbi->free_inodes_count = n;
    pfs_ext4_sb_set_u32(p_sbi->sb_raw, OFF_FREE_INODES, n);
    return pfs_ext4_sync_sb(p_sbi);
}

static uint32_t pfs_ext4_group_num_inodes(const pfs_ext4_sbi_t *p_sbi, uint32_t group)
{
    uint32_t start = group * p_sbi->inodes_per_group + 1u;
    uint32_t end = start + p_sbi->inodes_per_group;
    if (end > p_sbi->inodes_count + 1u)
    {
        end = p_sbi->inodes_count + 1u;
    }
    if (end <= start)
    {
        return 0u;
    }
    return end - start;
}

static bool pfs_ext4_ibitmap_test(const uint8_t *p_bmp, uint32_t bit)
{
    return (0u != (p_bmp[bit >> 3] & (uint8_t)(1u << (bit & 7u))));
}

static void pfs_ext4_ibitmap_set(uint8_t *p_bmp, uint32_t bit)
{
    p_bmp[bit >> 3] = (uint8_t)(p_bmp[bit >> 3] | (uint8_t)(1u << (bit & 7u)));
}

static void pfs_ext4_ibitmap_clear(uint8_t *p_bmp, uint32_t bit)
{
    p_bmp[bit >> 3] = (uint8_t)(p_bmp[bit >> 3] & (uint8_t)(~(uint8_t)(1u << (bit & 7u))));
}

int pfs_ext4_ialloc_new(pfs_ext4_sbi_t *p_sbi, uint32_t *p_ino)
{
    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    uint8_t *p_bmp = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_bmp)
    {
        return PFS_EXT4_RET_ENOMEM;
    }

    for (uint32_t group = 0; group < p_sbi->groups_count; group++)
    {
        pfs_ext4_group_desc_t gd;
        int rc = pfs_ext4_gd_get(p_sbi, group, &gd);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_bmp);
            return rc;
        }
        if (0u != (gd.bg_flags & PFS_EXT4_BG_INODE_UNINIT))
        {
            continue;
        }
        if (0u == pfs_ext4_gd_free_inodes_get(p_sbi, &gd))
        {
            continue;
        }

        uint64_t bmp_blk = pfs_ext4_gd_inode_bitmap(p_sbi, &gd);
        rc = pfs_ext4_read_block(p_sbi, bmp_blk, p_bmp);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_bmp);
            return rc;
        }

        uint32_t bits = pfs_ext4_group_num_inodes(p_sbi, group);
        for (uint32_t bit = 0; bit < bits; bit++)
        {
            uint32_t ino = group * p_sbi->inodes_per_group + bit + 1u;
            if (ino < p_sbi->first_ino)
            {
                continue;
            }
            if (ino > p_sbi->inodes_count)
            {
                break;
            }
            if (pfs_ext4_ibitmap_test(p_bmp, bit))
            {
                continue;
            }

            pfs_ext4_ibitmap_set(p_bmp, bit);
            rc = pfs_ext4_write_block(p_sbi, bmp_blk, p_bmp);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }

            pfs_ext4_gd_free_inodes_set(p_sbi, &gd,
                    pfs_ext4_gd_free_inodes_get(p_sbi, &gd) - 1u);
            rc = pfs_ext4_gd_put(p_sbi, group, &gd);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }
            rc = pfs_ext4_sb_dec_free_inodes(p_sbi);
            if (PFS_EXT4_RET_OK != rc)
            {
                pfs_free(p_bmp);
                return rc;
            }

            *p_ino = ino;
            pfs_free(p_bmp);
            return PFS_EXT4_RET_OK;
        }
    }

    pfs_free(p_bmp);
    return PFS_EXT4_RET_ENOSPC;
}

int pfs_ext4_ialloc_free(pfs_ext4_sbi_t *p_sbi, uint32_t ino)
{
    if (NULL == p_sbi || 0u == ino)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (PFS_EXT4_ROOT_INO == ino)
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    if (ino > p_sbi->inodes_count)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    ino--;
    uint32_t group = ino / p_sbi->inodes_per_group;
    uint32_t bit = ino % p_sbi->inodes_per_group;

    pfs_ext4_group_desc_t gd;
    int rc = pfs_ext4_gd_get(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0u != (gd.bg_flags & PFS_EXT4_BG_INODE_UNINIT))
    {
        return PFS_EXT4_RET_EINVAL;
    }

    uint8_t *p_bmp = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_bmp)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    uint64_t bmp_blk = pfs_ext4_gd_inode_bitmap(p_sbi, &gd);
    rc = pfs_ext4_read_block(p_sbi, bmp_blk, p_bmp);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_bmp);
        return rc;
    }
    if (!pfs_ext4_ibitmap_test(p_bmp, bit))
    {
        pfs_free(p_bmp);
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_ibitmap_clear(p_bmp, bit);
    rc = pfs_ext4_write_block(p_sbi, bmp_blk, p_bmp);
    pfs_free(p_bmp);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    pfs_ext4_gd_free_inodes_set(p_sbi, &gd, pfs_ext4_gd_free_inodes_get(p_sbi, &gd) + 1u);
    rc = pfs_ext4_gd_put(p_sbi, group, &gd);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    return pfs_ext4_sb_inc_free_inodes(p_sbi);
}

// end
