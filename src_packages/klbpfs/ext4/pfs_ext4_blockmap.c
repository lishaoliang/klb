// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 block map: 传统 indirect 映射与 map_block 分派
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

static uint32_t pfs_ext4_ptrs_per_block(const pfs_ext4_sbi_t *p_sbi)
{
    return p_sbi->block_size / 4u;
}

static uint32_t pfs_ext4_blocks512_delta(const pfs_ext4_sbi_t *p_sbi)
{
    return p_sbi->block_size / 512u;
}

static void pfs_ext4_bump_blocks512(const pfs_ext4_sbi_t *p_sbi, uint32_t *p_blocks_512, uint32_t n_blks)
{
    if (NULL != p_blocks_512 && 0u != n_blks)
    {
        *p_blocks_512 += n_blks * pfs_ext4_blocks512_delta(p_sbi);
    }
}

static int pfs_ext4_iblock_all_zero(const uint32_t i_block[PFS_EXT4_N_BLOCKS])
{

    for (unsigned i = 0; i < PFS_EXT4_N_BLOCKS; i++)
    {
        if (0u != i_block[i])
        {
            return 0;
        }
    }
    return 1;
}

static int pfs_ext4_read_u32_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, uint32_t idx,
        uint32_t *p_val)
{
    if (NULL == p_val)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    uint8_t *p_buf = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_buf)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    int rc = pfs_ext4_read_block(p_sbi, block, p_buf);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_buf);
        return rc;
    }
    *p_val = pfs_read_le32(p_buf + (size_t)idx * 4u);
    pfs_free(p_buf);
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_write_u32_block(pfs_ext4_sbi_t *p_sbi, uint64_t block, uint32_t idx, uint32_t val)
{
    uint8_t *p_buf = (uint8_t *)pfs_malloc(p_sbi->block_size);
    if (NULL == p_buf)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    int rc = pfs_ext4_read_block(p_sbi, block, p_buf);
    if (PFS_EXT4_RET_OK != rc)
    {
        pfs_free(p_buf);
        return rc;
    }
    pfs_write_le32(p_buf + (size_t)idx * 4u, val);
    rc = pfs_ext4_write_block(p_sbi, block, p_buf);
    pfs_free(p_buf);
    return rc;
}

static int pfs_ext4_alloc_zero_block(pfs_ext4_sbi_t *p_sbi, uint64_t *p_block, uint32_t *p_blocks_512)
{
    if (NULL == p_block)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    int rc = pfs_ext4_balloc_new(p_sbi, p_block);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    uint8_t *p_buf = (uint8_t *)pfs_mallocz(p_sbi->block_size);
    if (NULL == p_buf)
    {
        return PFS_EXT4_RET_ENOMEM;
    }
    rc = pfs_ext4_write_block(p_sbi, *p_block, p_buf);
    pfs_free(p_buf);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
    return PFS_EXT4_RET_OK;
}

int pfs_ext4_blockmap_get(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t logical, uint64_t *p_pblk)
{
    if (NULL == p_sbi || NULL == i_block || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    *p_pblk = 0ull;
    uint32_t ptrs = pfs_ext4_ptrs_per_block(p_sbi);
    if (logical < PFS_EXT4_NDIR_BLOCKS)
    {
        *p_pblk = i_block[logical];
        return PFS_EXT4_RET_OK;
    }

    uint32_t l = logical - PFS_EXT4_NDIR_BLOCKS;
    if (l < ptrs)
    {
        uint64_t ib = i_block[PFS_EXT4_IND_BLOCK];
        if (0u == ib)
        {
            return PFS_EXT4_RET_OK;
        }
        uint32_t u32;
        int rc = pfs_ext4_read_u32_block(p_sbi, ib, l, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        *p_pblk = u32;
        return PFS_EXT4_RET_OK;
    }
    l -= ptrs;

    if (l < ptrs * ptrs)
    {
        uint32_t idx1 = l / ptrs;
        uint32_t idx2 = l % ptrs;
        uint64_t ib = i_block[PFS_EXT4_DIND_BLOCK];
        if (0u == ib)
        {
            return PFS_EXT4_RET_OK;
        }
        uint32_t u32;
        int rc = pfs_ext4_read_u32_block(p_sbi, ib, idx1, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            return PFS_EXT4_RET_OK;
        }
        rc = pfs_ext4_read_u32_block(p_sbi, u32, idx2, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        *p_pblk = u32;
        return PFS_EXT4_RET_OK;
    }
    l -= ptrs * ptrs;

    if (l < ptrs * ptrs * ptrs)
    {
        uint32_t idx1 = l / (ptrs * ptrs);
        uint32_t idx2 = (l / ptrs) % ptrs;
        uint32_t idx3 = l % ptrs;
        uint64_t ib = i_block[PFS_EXT4_TIND_BLOCK];
        if (0u == ib)
        {
            return PFS_EXT4_RET_OK;
        }
        uint32_t u32;
        int rc = pfs_ext4_read_u32_block(p_sbi, ib, idx1, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            return PFS_EXT4_RET_OK;
        }
        uint64_t ib2 = u32;
        rc = pfs_ext4_read_u32_block(p_sbi, ib2, idx2, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            return PFS_EXT4_RET_OK;
        }
        rc = pfs_ext4_read_u32_block(p_sbi, u32, idx3, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        *p_pblk = u32;
        return PFS_EXT4_RET_OK;
    }

    return PFS_EXT4_RET_ERANGE;
}

int pfs_ext4_blockmap_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk)
{
    if (NULL == p_sbi || NULL == i_block || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    int rc = pfs_ext4_blockmap_get(p_sbi, i_block, logical, p_pblk);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0ull != *p_pblk)
    {
        return PFS_EXT4_RET_OK;
    }

    uint32_t ptrs = pfs_ext4_ptrs_per_block(p_sbi);

    if (logical < PFS_EXT4_NDIR_BLOCKS)
    {
        uint64_t data;
        rc = pfs_ext4_balloc_new(p_sbi, &data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        i_block[logical] = (uint32_t)data;
        pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
        *p_pblk = data;
        return PFS_EXT4_RET_OK;
    }

    uint32_t l = logical - PFS_EXT4_NDIR_BLOCKS;
    if (l < ptrs)
    {
        uint64_t ib;
        uint64_t data;

        if (0u == i_block[PFS_EXT4_IND_BLOCK])
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &ib, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            i_block[PFS_EXT4_IND_BLOCK] = (uint32_t)ib;
        }
        else
        {
            ib = i_block[PFS_EXT4_IND_BLOCK];
        }
        rc = pfs_ext4_balloc_new(p_sbi, &data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_ext4_write_u32_block(p_sbi, ib, l, (uint32_t)data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
        *p_pblk = data;
        return PFS_EXT4_RET_OK;
    }
    l -= ptrs;

    if (l < ptrs * ptrs)
    {
        uint32_t idx1 = l / ptrs;
        uint32_t idx2 = l % ptrs;
        uint64_t ib;
        uint64_t data;
        uint32_t u32;

        if (0u == i_block[PFS_EXT4_DIND_BLOCK])
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &ib, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            i_block[PFS_EXT4_DIND_BLOCK] = (uint32_t)ib;
        }
        else
        {
            ib = i_block[PFS_EXT4_DIND_BLOCK];
        }
        rc = pfs_ext4_read_u32_block(p_sbi, ib, idx1, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &data, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            u32 = (uint32_t)data;
            rc = pfs_ext4_write_u32_block(p_sbi, ib, idx1, u32);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
        }
        rc = pfs_ext4_balloc_new(p_sbi, &data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_ext4_write_u32_block(p_sbi, u32, idx2, (uint32_t)data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
        *p_pblk = data;
        return PFS_EXT4_RET_OK;
    }
    l -= ptrs * ptrs;

    if (l < ptrs * ptrs * ptrs)
    {
        uint32_t idx1 = l / (ptrs * ptrs);
        uint32_t idx2 = (l / ptrs) % ptrs;
        uint32_t idx3 = l % ptrs;
        uint64_t ib;
        uint64_t ib2;
        uint64_t ib3;
        uint64_t data;
        uint32_t u32;

        if (0u == i_block[PFS_EXT4_TIND_BLOCK])
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &ib, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            i_block[PFS_EXT4_TIND_BLOCK] = (uint32_t)ib;
        }
        else
        {
            ib = i_block[PFS_EXT4_TIND_BLOCK];
        }
        rc = pfs_ext4_read_u32_block(p_sbi, ib, idx1, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &ib2, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            u32 = (uint32_t)ib2;
            rc = pfs_ext4_write_u32_block(p_sbi, ib, idx1, u32);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            ib2 = u32;
        }
        rc = pfs_ext4_read_u32_block(p_sbi, ib2, idx2, &u32);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        if (0u == u32)
        {
            rc = pfs_ext4_alloc_zero_block(p_sbi, &ib3, p_blocks_512);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            u32 = (uint32_t)ib3;
            rc = pfs_ext4_write_u32_block(p_sbi, ib2, idx2, u32);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
        }
        rc = pfs_ext4_balloc_new(p_sbi, &data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_ext4_write_u32_block(p_sbi, u32, idx3, (uint32_t)data);
        if (PFS_EXT4_RET_OK != rc)
        {
            return rc;
        }
        pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
        *p_pblk = data;
        return PFS_EXT4_RET_OK;
    }

    return PFS_EXT4_RET_ERANGE;
}

int pfs_ext4_map_block(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t flags, uint32_t logical, uint64_t *p_pblk)
{
    if (NULL == p_sbi || NULL == i_block || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (0u != (flags & PFS_EXT4_EXTENTS_FL))
    {
        return pfs_ext4_extents_get(p_sbi, i_block, logical, p_pblk);
    }
    return pfs_ext4_blockmap_get(p_sbi, i_block, logical, p_pblk);
}

int pfs_ext4_map_block_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_flags, uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk)
{
    if (NULL == p_sbi || NULL == i_block || NULL == p_flags || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    if (0u != (*p_flags & PFS_EXT4_EXTENTS_FL))
    {
        return pfs_ext4_extents_alloc(p_sbi, i_block, p_blocks_512, logical, p_pblk);
    }

    if (0u != (p_sbi->feature_incompat & PFS_EXT4_FEATURE_INCOMPAT_EXTENTS) &&
            pfs_ext4_iblock_all_zero(i_block))
    {
        *p_flags |= PFS_EXT4_EXTENTS_FL;
        return pfs_ext4_extents_alloc(p_sbi, i_block, p_blocks_512, logical, p_pblk);
    }

    return pfs_ext4_blockmap_alloc(p_sbi, i_block, p_blocks_512, logical, p_pblk);
}

// end
