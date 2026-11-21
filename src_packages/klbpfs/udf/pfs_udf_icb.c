// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF FE/EFE parse and extent list build
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

static int pfs_udf_fe_header_size(uint16_t tag_ident)
{
    if (PFS_UDF_TAG_IDENT_EFE == tag_ident)
    {
        return (int)sizeof(pfs_udf_extended_file_entry_t);
    }
    return (int)sizeof(pfs_udf_file_entry_t);
}

int pfs_udf_add_extent(pfs_udf_inode_t *p_out, uint32_t block, uint32_t skip,
        uint32_t len)
{
    if (NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (p_out->n_extents >= PFS_UDF_MAX_EXTENTS)
    {
        return PFS_UDF_RET_ERANGE;
    }
    p_out->sec[p_out->n_extents].block = block;
    p_out->sec[p_out->n_extents].skip = skip;
    p_out->sec[p_out->n_extents].len = len;
    p_out->n_extents++;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_parse_short_ads(const pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_out,
        const uint8_t *p_ad, int ad_len, uint16_t part_ref)
{
    int off = 0;

    while (off + (int)sizeof(pfs_udf_short_ad_t) <= ad_len)
    {
        const pfs_udf_short_ad_t *p_sa = (const pfs_udf_short_ad_t *)(p_ad + off);
        uint32_t elen_raw = pfs_udf_le32(&p_sa->extLength);
        if (0 == elen_raw)
        {
            break;
        }
        uint32_t etype = elen_raw & PFS_UDF_EXT_TYPE_MASK;
        uint32_t elen = elen_raw & PFS_UDF_EXT_LENGTH_MASK;
        uint32_t pos = pfs_udf_le32(&p_sa->extPosition);
        off += (int)sizeof(pfs_udf_short_ad_t);

        if (PFS_UDF_EXT_NEXT_ALLOCDESCS == etype)
        {
            return PFS_UDF_RET_EOPNOTSUPP;
        }
        if (PFS_UDF_EXT_RECORDED_ALLOC != etype)
        {
            continue;
        }
        if (0 == elen)
        {
            continue;
        }
        int rc = pfs_udf_add_extent(p_out, pos, 0, elen);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
        (void)p_sbi;
        (void)part_ref;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_parse_long_ads(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_out,
        const uint8_t *p_ad, int ad_len)
{
    int off = 0;

    while (off + (int)sizeof(pfs_udf_long_ad_t) <= ad_len)
    {
        const pfs_udf_long_ad_t *p_la = (const pfs_udf_long_ad_t *)(p_ad + off);
        uint32_t elen_raw = pfs_udf_le32(&p_la->extLength);

        if (0 == elen_raw)
        {
            break;
        }

        uint32_t etype = elen_raw & PFS_UDF_EXT_TYPE_MASK;
        uint32_t elen = elen_raw & PFS_UDF_EXT_LENGTH_MASK;
        uint32_t pos = pfs_udf_le32(&p_la->extLocation.logicalBlockNum);
        uint16_t pref = pfs_udf_le16(&p_la->extLocation.partitionReferenceNum);

        off += (int)sizeof(pfs_udf_long_ad_t);

        if (PFS_UDF_EXT_NEXT_ALLOCDESCS == etype)
        {
            uint8_t *blk;
            const pfs_udf_alloc_ext_desc_t *p_aed;
            int body_len;

            if (NULL == p_sbi)
            {
                return PFS_UDF_RET_EINVAL;
            }

            blk = (uint8_t *)pfs_malloc((size_t)p_sbi->block_size);
            if (NULL == blk)
            {
                return PFS_UDF_RET_ENOMEM;
            }

            int rc = pfs_udf_read_part_block_ref(p_sbi, pref, pos, blk);
            if (PFS_UDF_RET_OK != rc)
            {
                pfs_free(blk);
                return rc;
            }
            p_aed = (const pfs_udf_alloc_ext_desc_t *)blk;
            if (PFS_UDF_TAG_IDENT_AED != pfs_udf_le16(&p_aed->descTag.tagIdent))
            {
                pfs_free(blk);
                return PFS_UDF_RET_EBADMSG;
            }
            body_len = (int)pfs_udf_le32(&p_aed->lengthAllocDescs);
            if (body_len < 0 ||
                    (int)sizeof(pfs_udf_alloc_ext_desc_t) + body_len > (int)p_sbi->block_size)
            {
                pfs_free(blk);
                return PFS_UDF_RET_EBADMSG;
            }
            rc = pfs_udf_parse_long_ads(p_sbi, p_out,
                    blk + sizeof(pfs_udf_alloc_ext_desc_t), body_len);
            pfs_free(blk);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
            continue;
        }
        if (PFS_UDF_EXT_RECORDED_ALLOC != etype)
        {
            continue;
        }
        if (pref != p_out->part_ref)
        {
            return PFS_UDF_RET_EOPNOTSUPP;
        }
        if (0 == elen)
        {
            continue;
        }

        int rc = pfs_udf_add_extent(p_out, pos, 0, elen);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_icb_from_fe_buf(const pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint32_t iblock, uint16_t part_ref, pfs_udf_inode_t *p_out)
{
    return pfs_udf_icb_from_fe_buf_ex(p_sbi, p_buf, buf_len, iblock, part_ref, p_out, 0);
}

int pfs_udf_icb_from_fe_buf_ex(const pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint32_t iblock, uint16_t part_ref, pfs_udf_inode_t *p_out,
        int allow_special)
{
    if (NULL == p_buf || NULL == p_out || buf_len < (int)sizeof(pfs_udf_file_entry_t))
    {
        return PFS_UDF_RET_EINVAL;
    }

    const pfs_udf_file_entry_t *p_fe = (const pfs_udf_file_entry_t *)p_buf;
    uint16_t tag_ident = pfs_udf_le16(&p_fe->descTag.tagIdent);

    if (PFS_UDF_TAG_IDENT_FE != tag_ident && PFS_UDF_TAG_IDENT_EFE != tag_ident)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    const pfs_udf_icbtag_t *p_icb = &p_fe->icbTag;
    uint16_t flags = pfs_udf_le16(&p_icb->flags);
    int alloc_type = (int)(flags & 0x0007u);
    uint8_t file_type = p_icb->fileType;
    uint16_t link_count = pfs_udf_le16(&p_fe->fileLinkCount);

    memset(p_out, 0, sizeof(*p_out));
    p_out->iblock = iblock;
    p_out->part_ref = part_ref;
    p_out->alloc_type = alloc_type;
    p_out->size = pfs_udf_le64(&p_fe->informationLength);

    if (PFS_UDF_ICBTAG_FILE_DIRECTORY == file_type)
    {
        p_out->type = PFS_UDF_TYPE_DIR;
    }
    else if (PFS_UDF_ICBTAG_FILE_REGULAR == file_type)
    {
        p_out->type = PFS_UDF_TYPE_FILE;
    }
    else if (PFS_UDF_ICBTAG_FILE_SYMLINK == file_type)
    {
        return PFS_UDF_RET_ENOENT;
    } else if (allow_special &&
            (PFS_UDF_ICBTAG_FILE_VAT20 == file_type || 0 == file_type ||
             PFS_UDF_ICBTAG_FILE_MAIN == file_type ||
             PFS_UDF_ICBTAG_FILE_MIRROR == file_type))
    {
        p_out->type = PFS_UDF_TYPE_FILE;
    }
    else
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    if (0 == link_count && 0 == allow_special)
    {
        return PFS_UDF_RET_ENOENT;
    }

    int hdr_sz = pfs_udf_fe_header_size(tag_ident);
    uint32_t len_eattr;
    uint32_t len_alloc;

    if (PFS_UDF_TAG_IDENT_EFE == tag_ident)
    {
        const pfs_udf_extended_file_entry_t *p_efe = (const pfs_udf_extended_file_entry_t *)p_buf;

        len_eattr = pfs_udf_le32(&p_efe->lengthExtendedAttr);
        len_alloc = pfs_udf_le32(&p_efe->lengthAllocDescs);
    }
    else
    {
        len_eattr = pfs_udf_le32(&p_fe->lengthExtendedAttr);
        len_alloc = pfs_udf_le32(&p_fe->lengthAllocDescs);
    }

    if (hdr_sz + (int)len_eattr + (int)len_alloc > buf_len)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    if (PFS_UDF_ICBTAG_AD_EXTENDED == alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    if (PFS_UDF_ICBTAG_AD_IN_ICB == alloc_type)
    {
        uint32_t data_off = (uint32_t)hdr_sz + len_eattr;

        return pfs_udf_add_extent(p_out, iblock, data_off, (uint32_t)p_out->size);
    }

    if (PFS_UDF_ICBTAG_AD_SHORT == alloc_type)
    {
        return pfs_udf_parse_short_ads(p_sbi, p_out, p_buf + hdr_sz + len_eattr,
                (int)len_alloc, part_ref);
    }
    if (PFS_UDF_ICBTAG_AD_LONG == alloc_type)
    {
        return pfs_udf_parse_long_ads((pfs_udf_sbi_t *)p_sbi, p_out,
                p_buf + hdr_sz + len_eattr, (int)len_alloc);
    }
    return PFS_UDF_RET_EBADMSG;
}

int pfs_udf_icb_read(pfs_udf_sbi_t *p_sbi, const pfs_udf_lb_addr_t *p_loc,
        pfs_udf_inode_t *p_out)
{
    if (NULL == p_sbi || NULL == p_loc || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint32_t iblock = pfs_udf_le32(&p_loc->logicalBlockNum);
    uint16_t part_ref = pfs_udf_le16(&p_loc->partitionReferenceNum);

    if (iblock >= pfs_udf_part_capacity_blocks(p_sbi, part_ref))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)p_sbi->block_size);
    if (NULL == blk)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    int rc = pfs_udf_read_part_block_ref(p_sbi, part_ref, iblock, blk);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(blk);
        return rc;
    }
    rc = pfs_udf_icb_from_fe_buf(p_sbi, blk, (int)p_sbi->block_size, iblock, part_ref, p_out);
    pfs_free(blk);
    return rc;
}

// end
