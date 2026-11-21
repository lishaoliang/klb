// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF Space Bitmap (SBD) load/flush and block alloc/free (Type1 rewritable)
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

#define PFS_UDF_DESC_VERSION            0x0003u

static int pfs_udf_bitmap_bit_idx(uint32_t part_lba)
{
    return (int)part_lba + (int)((int)sizeof(pfs_udf_space_bitmap_desc_t) << 3);
}

static int pfs_udf_bitmap_get_bit(const uint8_t *bitmap, uint32_t part_lba)
{
    int bit = pfs_udf_bitmap_bit_idx(part_lba);
    int byte = bit >> 3;
    return (bitmap[byte] >> (bit & 7)) & 1;
}

static void pfs_udf_bitmap_set_bit(uint8_t *bitmap, uint32_t part_lba, int free_bit)
{
    int bit = pfs_udf_bitmap_bit_idx(part_lba);
    int byte = bit >> 3;
    uint8_t mask = (uint8_t)(1u << (bit & 7));
    if (free_bit)
    {
        bitmap[byte] = (uint8_t)(bitmap[byte] | mask);
    }
    else
    {
        bitmap[byte] = (uint8_t)(bitmap[byte] & (uint8_t)~mask);
    }
}

static int pfs_udf_bitmap_lba_reserved(const pfs_udf_sbi_t *p_sbi, uint32_t lba)
{
    if (NULL == p_sbi || 0 == p_sbi->bitmap_blocks)
    {
        return 0;
    }
    if (lba >= p_sbi->bitmap_lba &&
            lba < p_sbi->bitmap_lba + p_sbi->bitmap_blocks)
    {
        return 1;
    }
    return 0;
}

int pfs_udf_volume_can_rw(const pfs_udf_sbi_t *p_sbi)
{

    if (NULL == p_sbi || 0 == p_sbi->n_maps)
    {
        return 0;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (PFS_UDF_PMAP_TYPE1 != p_sbi->maps[i].type)
        {
            return 0;
        }
    }
    if (PFS_UDF_PD_ACCESS_REWRITABLE != p_sbi->pd_access_type &&
            PFS_UDF_PD_ACCESS_OVERWRITABLE != p_sbi->pd_access_type)
    {
        return 0;
    }
    if (0 == p_sbi->have_unalloc_bitmap || 0 == p_sbi->bitmap_blocks)
    {
        return 0;
    }
    return 1;
}

int pfs_udf_bitmap_load(pfs_udf_sbi_t *p_sbi)
{
    pfs_udf_space_bitmap_desc_t *p_sbd;
    uint32_t buf_size;
    uint8_t *buf;

    if (NULL == p_sbi || 0 == p_sbi->bitmap_blocks || 0 == p_sbi->block_size)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (NULL != p_sbi->bitmap)
    {
        return PFS_UDF_RET_OK;
    }

    buf_size = p_sbi->bitmap_blocks * p_sbi->block_size;
    buf = (uint8_t *)pfs_malloc((size_t)buf_size);
    if (NULL == buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    memset(buf, 0, (size_t)buf_size);

    int rc;
    {

        for (uint32_t b = 0; b < p_sbi->bitmap_blocks; b++)
        {
            rc = pfs_udf_read_part_block_ref(p_sbi, p_sbi->bitmap_part_ref,
                    p_sbi->bitmap_lba + b,
                    buf + (size_t)b * (size_t)p_sbi->block_size);
            if (PFS_UDF_RET_OK != rc)
            {
                pfs_free(buf);
                return rc;
            }
        }
    }

    p_sbd = (pfs_udf_space_bitmap_desc_t *)buf;
    if (PFS_UDF_TAG_IDENT_SBD != pfs_udf_le16(&p_sbd->descTag.tagIdent))
    {
        pfs_free(buf);
        return PFS_UDF_RET_EBADMSG;
    }
    p_sbi->bitmap_num_bits = pfs_udf_le32(&p_sbd->numOfBits);
    p_sbi->bitmap_num_bytes = pfs_udf_le32(&p_sbd->numOfBytes);
    rc = pfs_udf_verify_tag(&p_sbd->descTag, p_sbi->bitmap_lba,
            buf + sizeof(pfs_udf_tag_t),
            (int)sizeof(*p_sbd) - (int)sizeof(pfs_udf_tag_t) + (int)p_sbi->bitmap_num_bytes);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(buf);
        return rc;
    }

    p_sbi->bitmap = buf;
    p_sbi->bitmap_dirty = 0;
    p_sbi->have_unalloc_bitmap = 1;
    return PFS_UDF_RET_OK;
}

int pfs_udf_bitmap_flush(pfs_udf_sbi_t *p_sbi)
{
    pfs_udf_space_bitmap_desc_t *p_sbd;

    if (NULL == p_sbi || NULL == p_sbi->bitmap || 0 == p_sbi->bitmap_blocks)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == p_sbi->bitmap_dirty)
    {
        return PFS_UDF_RET_OK;
    }

    p_sbd = (pfs_udf_space_bitmap_desc_t *)p_sbi->bitmap;
    pfs_udf_wle32(&p_sbd->numOfBits, p_sbi->bitmap_num_bits);
    pfs_udf_wle32(&p_sbd->numOfBytes, p_sbi->bitmap_num_bytes);
    pfs_udf_set_tag(&p_sbd->descTag, PFS_UDF_TAG_IDENT_SBD, p_sbi->bitmap_lba, p_sbi->bitmap,
            (int)sizeof(pfs_udf_tag_t),
            (int)sizeof(*p_sbd) - (int)sizeof(pfs_udf_tag_t) + (int)p_sbi->bitmap_num_bytes,
            PFS_UDF_DESC_VERSION);

    for (uint32_t b = 0; b < p_sbi->bitmap_blocks; b++)
    {
        int rc = pfs_udf_write_part_block_ref(p_sbi, p_sbi->bitmap_part_ref,
                p_sbi->bitmap_lba + b,
                p_sbi->bitmap + (size_t)b * (size_t)p_sbi->block_size);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    p_sbi->bitmap_dirty = 0;
    return PFS_UDF_RET_OK;
}

void pfs_udf_bitmap_free(pfs_udf_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return;
    }
    if (NULL != p_sbi->bitmap)
    {
        pfs_free(p_sbi->bitmap);
        p_sbi->bitmap = NULL;
    }
    p_sbi->bitmap_dirty = 0;
    p_sbi->bitmap_num_bits = 0;
    p_sbi->bitmap_num_bytes = 0;
}

int pfs_udf_alloc_blocks(pfs_udf_sbi_t *p_sbi, uint32_t count, uint32_t *p_start)
{
    uint32_t cap;
    uint32_t run;
    uint32_t start;

    if (NULL == p_sbi || NULL == p_start || 0 == count)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (NULL == p_sbi->bitmap)
    {
        return PFS_UDF_RET_EPERM;
    }

    cap = p_sbi->partition_len;
    if (0 == cap && p_sbi->bitmap_num_bits > 0)
    {
        cap = p_sbi->bitmap_num_bits;
    }
    start = 0xFFFFFFFFu;
    run = 0;
    for (uint32_t lba = 0; lba < cap; lba++)
    {
        if (pfs_udf_bitmap_lba_reserved(p_sbi, lba))
        {
            run = 0;
            continue;
        }
        if (0 == pfs_udf_bitmap_get_bit(p_sbi->bitmap, lba))
        {
            run = 0;
            continue;
        }
        if (0 == run)
        {
            start = lba;
        }
        run++;
        if (run >= count)
        {
            for (uint32_t i = 0; i < count; i++)
            {
                pfs_udf_bitmap_set_bit(p_sbi->bitmap, start + i, 0);
            }
            p_sbi->bitmap_dirty = 1;
            *p_start = start;
            return PFS_UDF_RET_OK;
        }
    }
    return PFS_UDF_RET_ENOSPC;
}

int pfs_udf_free_blocks(pfs_udf_sbi_t *p_sbi, uint32_t start, uint32_t count)
{

    if (NULL == p_sbi || 0 == count)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (NULL == p_sbi->bitmap)
    {
        return PFS_UDF_RET_EPERM;
    }
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t lba;

        lba = start + i;
        if (pfs_udf_bitmap_lba_reserved(p_sbi, lba))
        {
            continue;
        }
        pfs_udf_bitmap_set_bit(p_sbi->bitmap, lba, 1);
    }
    p_sbi->bitmap_dirty = 1;
    return PFS_UDF_RET_OK;
}

// end
