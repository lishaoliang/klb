// Doc Encode : UTF-8 BOM, Unix(LF)
// ext4 extents: inline/header 树查找与 depth==0 分配
#include "ext4/pfs_ext4_internal.h"

#include <string.h>

#define PFS_EXT4_EXT_HDR_SIZE           ((unsigned)sizeof(pfs_ext4_extent_header_t))
#define PFS_EXT4_EXT_ENT_SIZE           ((unsigned)sizeof(pfs_ext4_extent_t))
#define PFS_EXT4_EXT_IDX_SIZE           ((unsigned)sizeof(pfs_ext4_extent_idx_t))
#define PFS_EXT4_EXT_UNWRITTEN_MASK     0x8000u
#define PFS_EXT4_EXT_INLINE_MAX         4u

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

static void pfs_ext4_ext_header_from_bytes(const uint8_t *p, pfs_ext4_extent_header_t *p_hdr)
{
    p_hdr->eh_magic = pfs_read_le16(p + 0);
    p_hdr->eh_entries = pfs_read_le16(p + 2);
    p_hdr->eh_max = pfs_read_le16(p + 4);
    p_hdr->eh_depth = pfs_read_le16(p + 6);
    p_hdr->eh_generation = pfs_read_le32(p + 8);
}

static void pfs_ext4_ext_header_to_bytes(const pfs_ext4_extent_header_t *p_hdr, uint8_t *p)
{
    pfs_write_le16(p + 0, p_hdr->eh_magic);
    pfs_write_le16(p + 2, p_hdr->eh_entries);
    pfs_write_le16(p + 4, p_hdr->eh_max);
    pfs_write_le16(p + 6, p_hdr->eh_depth);
    pfs_write_le32(p + 8, p_hdr->eh_generation);
}

static void pfs_ext4_extent_from_bytes(const uint8_t *p, pfs_ext4_extent_t *p_ext)
{
    p_ext->ee_block = pfs_read_le32(p + 0);
    p_ext->ee_len = pfs_read_le16(p + 4);
    p_ext->ee_start_hi = pfs_read_le16(p + 6);
    p_ext->ee_start_lo = pfs_read_le32(p + 8);
}

static void pfs_ext4_extent_to_bytes(const pfs_ext4_extent_t *p_ext, uint8_t *p)
{
    pfs_write_le32(p + 0, p_ext->ee_block);
    pfs_write_le16(p + 4, p_ext->ee_len);
    pfs_write_le16(p + 6, p_ext->ee_start_hi);
    pfs_write_le32(p + 8, p_ext->ee_start_lo);
}

static void pfs_ext4_extent_idx_from_bytes(const uint8_t *p, pfs_ext4_extent_idx_t *p_idx)
{
    p_idx->ei_block = pfs_read_le32(p + 0);
    p_idx->ei_leaf_lo = pfs_read_le32(p + 4);
    p_idx->ei_leaf_hi = pfs_read_le16(p + 8);
    p_idx->ei_unused = pfs_read_le16(p + 10);
}

static uint64_t pfs_ext4_extent_phys(const pfs_ext4_extent_t *p_ext)
{
    return (uint64_t)p_ext->ee_start_lo | ((uint64_t)p_ext->ee_start_hi << 32);
}

static uint64_t pfs_ext4_extent_idx_leaf(const pfs_ext4_extent_idx_t *p_idx)
{
    return (uint64_t)p_idx->ei_leaf_lo | ((uint64_t)p_idx->ei_leaf_hi << 32);
}

static uint16_t pfs_ext4_extent_len_val(uint16_t ee_len)
{
    if (0u != (ee_len & PFS_EXT4_EXT_UNWRITTEN_MASK))
    {
        return (uint16_t)(ee_len & ~PFS_EXT4_EXT_UNWRITTEN_MASK);
    }
    return ee_len;
}

static int pfs_ext4_extents_find_leaf(pfs_ext4_sbi_t *p_sbi, const uint8_t *p_node,
        uint32_t logical, uint64_t *p_pblk, const uint8_t **p_leaf_node, uint8_t **p_owned)
{
    const uint8_t *p_cur = p_node;
    uint8_t *p_buf = NULL;
    pfs_ext4_extent_header_t hdr;

    pfs_ext4_ext_header_from_bytes(p_cur, &hdr);
    if (PFS_EXT4_EXT_MAGIC != hdr.eh_magic)
    {
        return PFS_EXT4_RET_EBADMSG;
    }

    uint16_t depth = hdr.eh_depth;
    while (depth > 0u)
    {
        uint16_t best;
        pfs_ext4_extent_idx_t idx;
        uint64_t leaf;

        uint16_t entries = hdr.eh_entries;
        best = 0xFFFFu;
        for (uint16_t i = 0; i < entries; i++)
        {
            pfs_ext4_extent_idx_from_bytes(p_cur + PFS_EXT4_EXT_HDR_SIZE + (size_t)i * PFS_EXT4_EXT_IDX_SIZE,
                    &idx);
            if (idx.ei_block <= logical)
            {
                best = i;
            }
            else
            {
                break;
            }
        }
        if (best == 0xFFFFu)
        {
            *p_pblk = 0ull;
            if (NULL != p_leaf_node)
            {
                *p_leaf_node = NULL;
            }
            if (NULL != p_owned)
            {
                *p_owned = p_buf;
            }
            return PFS_EXT4_RET_OK;
        }

        pfs_ext4_extent_idx_from_bytes(p_cur + PFS_EXT4_EXT_HDR_SIZE + (size_t)best * PFS_EXT4_EXT_IDX_SIZE,
                &idx);
        leaf = pfs_ext4_extent_idx_leaf(&idx);
        if (0ull == leaf)
        {
            *p_pblk = 0ull;
            if (NULL != p_leaf_node)
            {
                *p_leaf_node = NULL;
            }
            if (NULL != p_owned)
            {
                *p_owned = p_buf;
            }
            return PFS_EXT4_RET_OK;
        }

        if (NULL != p_buf)
        {
            pfs_free(p_buf);
            p_buf = NULL;
        }
        p_buf = (uint8_t *)pfs_malloc(p_sbi->block_size);
        if (NULL == p_buf)
        {
            return PFS_EXT4_RET_ENOMEM;
        }
        int rc = pfs_ext4_read_block(p_sbi, leaf, p_buf);
        if (PFS_EXT4_RET_OK != rc)
        {
            pfs_free(p_buf);
            return rc;
        }
        p_cur = p_buf;
        pfs_ext4_ext_header_from_bytes(p_cur, &hdr);
        if (PFS_EXT4_EXT_MAGIC != hdr.eh_magic)
        {
            pfs_free(p_buf);
            return PFS_EXT4_RET_EBADMSG;
        }
        depth = hdr.eh_depth;
    }

    if (NULL != p_leaf_node)
    {
        *p_leaf_node = p_cur;
    }
    if (NULL != p_owned)
    {
        *p_owned = p_buf;
        p_buf = NULL;
    }
    else if (NULL != p_buf)
    {
        pfs_free(p_buf);
    }
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_extents_scan_leaf(const uint8_t *p_leaf, uint32_t logical, uint64_t *p_pblk)
{
    pfs_ext4_extent_header_t hdr;
    pfs_ext4_extent_t ext;
    uint16_t len;

    pfs_ext4_ext_header_from_bytes(p_leaf, &hdr);
    for (uint16_t i = 0; i < hdr.eh_entries; i++)
    {
        pfs_ext4_extent_from_bytes(p_leaf + PFS_EXT4_EXT_HDR_SIZE + (size_t)i * PFS_EXT4_EXT_ENT_SIZE, &ext);
        len = pfs_ext4_extent_len_val(ext.ee_len);
        if (0u == len)
        {
            continue;
        }
        if (logical >= ext.ee_block && logical < ext.ee_block + (uint32_t)len)
        {
            if (0u != (ext.ee_len & PFS_EXT4_EXT_UNWRITTEN_MASK))
            {
                *p_pblk = 0ull;
            }
            else
            {
                *p_pblk = pfs_ext4_extent_phys(&ext) + (uint64_t)(logical - ext.ee_block);
            }
            return PFS_EXT4_RET_OK;
        }
    }
    *p_pblk = 0ull;
    return PFS_EXT4_RET_OK;
}

static int pfs_ext4_extents_iblock_all_zero(const uint32_t i_block[PFS_EXT4_N_BLOCKS])
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

static void pfs_ext4_extents_init_header(uint32_t i_block[PFS_EXT4_N_BLOCKS])
{
    pfs_ext4_extent_header_t hdr;

    memset(i_block, 0, PFS_EXT4_N_BLOCKS * sizeof(i_block[0]));
    hdr.eh_magic = PFS_EXT4_EXT_MAGIC;
    hdr.eh_entries = 0u;
    hdr.eh_max = PFS_EXT4_EXT_INLINE_MAX;
    hdr.eh_depth = 0u;
    hdr.eh_generation = 0u;
    pfs_ext4_ext_header_to_bytes(&hdr, (uint8_t *)i_block);
}

static int pfs_ext4_extents_need_init(const uint32_t i_block[PFS_EXT4_N_BLOCKS])
{
    pfs_ext4_extent_header_t hdr;

    if (pfs_ext4_extents_iblock_all_zero(i_block))
    {
        return 1;
    }
    pfs_ext4_ext_header_from_bytes((const uint8_t *)i_block, &hdr);
    if (PFS_EXT4_EXT_MAGIC != hdr.eh_magic)
    {
        return 1;
    }
    return 0;
}

int pfs_ext4_extents_get(pfs_ext4_sbi_t *p_sbi, const uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t logical, uint64_t *p_pblk)
{
    const uint8_t *p_leaf;
    uint8_t *p_owned;

    if (NULL == p_sbi || NULL == i_block || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }

    *p_pblk = 0ull;
    int rc = pfs_ext4_extents_find_leaf(p_sbi, (const uint8_t *)i_block, logical, p_pblk, &p_leaf, &p_owned);
    if (PFS_EXT4_RET_OK != rc || NULL == p_leaf)
    {
        if (NULL != p_owned)
        {
            pfs_free(p_owned);
        }
        return rc;
    }
    rc = pfs_ext4_extents_scan_leaf(p_leaf, logical, p_pblk);
    if (NULL != p_owned)
    {
        pfs_free(p_owned);
    }
    return rc;
}

int pfs_ext4_extents_alloc(pfs_ext4_sbi_t *p_sbi, uint32_t i_block[PFS_EXT4_N_BLOCKS],
        uint32_t *p_blocks_512, uint32_t logical, uint64_t *p_pblk)
{
    pfs_ext4_extent_header_t hdr;
    pfs_ext4_extent_t ext;
    pfs_ext4_extent_t last;
    uint64_t data;
    uint64_t phys;
    uint16_t len;
    uint16_t slot;

    if (NULL == p_sbi || NULL == i_block || NULL == p_pblk)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write(p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }

    int rc = pfs_ext4_extents_get(p_sbi, i_block, logical, p_pblk);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (0ull != *p_pblk)
    {
        return PFS_EXT4_RET_OK;
    }

    if (pfs_ext4_extents_need_init(i_block))
    {
        pfs_ext4_extents_init_header(i_block);
    }

    pfs_ext4_ext_header_from_bytes((uint8_t *)i_block, &hdr);
    if (PFS_EXT4_EXT_MAGIC != hdr.eh_magic)
    {
        return PFS_EXT4_RET_EBADMSG;
    }
    if (0u != hdr.eh_depth)
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }

    if (hdr.eh_entries > 0u)
    {
        slot = (uint16_t)(hdr.eh_entries - 1u);
        pfs_ext4_extent_from_bytes((uint8_t *)i_block + PFS_EXT4_EXT_HDR_SIZE +
                (size_t)slot * PFS_EXT4_EXT_ENT_SIZE, &last);
        len = pfs_ext4_extent_len_val(last.ee_len);
        phys = pfs_ext4_extent_phys(&last);
        if (0u == (last.ee_len & PFS_EXT4_EXT_UNWRITTEN_MASK) &&
                last.ee_block + (uint32_t)len == logical &&
                phys + (uint64_t)len < p_sbi->blocks_count)
        {
            rc = pfs_ext4_balloc_new(p_sbi, &data);
            if (PFS_EXT4_RET_OK != rc)
            {
                return rc;
            }
            if (data == phys + (uint64_t)len)
            {
                last.ee_len = (uint16_t)(len + 1u);
                pfs_ext4_extent_to_bytes(&last, (uint8_t *)i_block + PFS_EXT4_EXT_HDR_SIZE +
                        (size_t)slot * PFS_EXT4_EXT_ENT_SIZE);
                pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
                *p_pblk = data;
                return PFS_EXT4_RET_OK;
            }
            (void)pfs_ext4_balloc_free(p_sbi, data);
        }
    }

    if (hdr.eh_entries >= hdr.eh_max)
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }

    rc = pfs_ext4_balloc_new(p_sbi, &data);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }

    memset(&ext, 0, sizeof(ext));
    ext.ee_block = logical;
    ext.ee_len = 1u;
    ext.ee_start_lo = (uint32_t)data;
    ext.ee_start_hi = (uint16_t)(data >> 32);
    slot = hdr.eh_entries;
    pfs_ext4_extent_to_bytes(&ext, (uint8_t *)i_block + PFS_EXT4_EXT_HDR_SIZE +
            (size_t)slot * PFS_EXT4_EXT_ENT_SIZE);
    hdr.eh_entries = (uint16_t)(hdr.eh_entries + 1u);
    pfs_ext4_ext_header_to_bytes(&hdr, (uint8_t *)i_block);
    pfs_ext4_bump_blocks512(p_sbi, p_blocks_512, 1u);
    *p_pblk = data;
    return PFS_EXT4_RET_OK;
}

// end
