// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF partition map: Type1 / Virtual+VAT / Metadata 2.50 block translation
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

static int pfs_udf_regid_prefix(const pfs_udf_regid_t *p_id, const char *prefix)
{
    if (NULL == p_id || NULL == prefix)
    {
        return 0;
    }
    size_t n = strlen(prefix);
    if (n > sizeof(p_id->ident))
    {
        return 0;
    }
    return (0 == strncmp((const char *)p_id->ident, prefix, n)) ? 1 : 0;
}

static const pfs_udf_pmap_t *pfs_udf_pmap_get(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref)
{
    if (NULL == p_sbi || part_ref >= (uint16_t)p_sbi->n_maps)
    {
        return NULL;
    }
    return &p_sbi->maps[part_ref];
}

int pfs_udf_find_type1_map_idx(const pfs_udf_sbi_t *p_sbi, uint16_t partition_num)
{

    if (NULL == p_sbi)
    {
        return -1;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (PFS_UDF_PMAP_TYPE1 == p_sbi->maps[i].type &&
                p_sbi->maps[i].partition_num == partition_num)
        {
            return i;
        }
    }
    return -1;
}

static int64_t pfs_udf_type1_lba_to_off(const pfs_udf_sbi_t *p_sbi,
        const pfs_udf_pmap_t *p_map, uint32_t part_lba)
{
    if (NULL == p_sbi || NULL == p_map)
    {
        return (int64_t)PFS_UDF_RET_EINVAL;
    }
    return p_sbi->part_offset
            + ((int64_t)p_map->phys_start * (int64_t)p_sbi->sector_size)
            + ((int64_t)part_lba * (int64_t)p_sbi->block_size);
}

static int pfs_udf_meta_lba_to_phys(const pfs_udf_pmap_t *p_map, uint32_t block_size,
        uint32_t meta_lba, uint32_t *p_phys_lba)
{
    if (NULL == p_map || 0 == block_size || NULL == p_phys_lba ||
            0 == p_map->have_meta_inode)
    {
        return PFS_UDF_RET_EINVAL;
    }

    const pfs_udf_inode_t *p_meta = &p_map->meta_inode;
    uint64_t byte_off = (uint64_t)meta_lba * (uint64_t)block_size;
    uint64_t file_off = 0;

    for (int ei = 0; ei < p_meta->n_extents; ei++)
    {
        uint64_t ext_len = (uint64_t)p_meta->sec[ei].len;

        if (byte_off >= file_off && byte_off < file_off + ext_len)
        {
            uint64_t rel = byte_off - file_off + (uint64_t)p_meta->sec[ei].skip;

            *p_phys_lba = p_meta->sec[ei].block +
                    (uint32_t)(rel / (uint64_t)block_size);
            return PFS_UDF_RET_OK;
        }
        file_off += ext_len;
    }
    return PFS_UDF_RET_EBADMSG;
}

static int pfs_udf_map_part_lba(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref,
        uint32_t part_lba, uint32_t *p_phys_lba, uint16_t *p_phys_ref)
{
    const pfs_udf_pmap_t *p_map = pfs_udf_pmap_get(p_sbi, part_ref);

    if (NULL == p_map)
    {
        return PFS_UDF_RET_EINVAL;
    }

    switch (p_map->type)
    {
    case PFS_UDF_PMAP_TYPE1:
        if (part_lba >= p_map->phys_len)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        *p_phys_lba = part_lba;
        *p_phys_ref = part_ref;
        return PFS_UDF_RET_OK;

    case PFS_UDF_PMAP_VIRT15:
    case PFS_UDF_PMAP_VIRT20:
    {
        uint32_t phys_lba;

        if (NULL == p_map->vat_entries_tbl || part_lba >= p_map->vat_entries)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        phys_lba = p_map->vat_entries_tbl[part_lba];
        if (0xFFFFFFFFu == phys_lba)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        *p_phys_lba = phys_lba;
        *p_phys_ref = p_map->vat_phys_part_ref;
        return PFS_UDF_RET_OK;
    }

    case PFS_UDF_PMAP_META25:
    {
        uint32_t phys_lba;

        if (PFS_UDF_RET_OK != pfs_udf_meta_lba_to_phys(p_map, p_sbi->block_size,
                part_lba, &phys_lba))
        {
            return PFS_UDF_RET_EBADMSG;
        }
        *p_phys_lba = phys_lba;
        *p_phys_ref = p_map->meta_phys_part_ref;
        return PFS_UDF_RET_OK;
    }

    default:
        return PFS_UDF_RET_EOPNOTSUPP;
    }
}

int64_t pfs_udf_map_to_off(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref, uint32_t lba)
{
    if (NULL == p_sbi)
    {
        return (int64_t)PFS_UDF_RET_EINVAL;
    }

    uint32_t phys_lba;
    uint16_t phys_ref;

    int rc = pfs_udf_map_part_lba(p_sbi, part_ref, lba, &phys_lba, &phys_ref);
    if (PFS_UDF_RET_OK != rc)
    {
        return (int64_t)rc;
    }

    const pfs_udf_pmap_t *p_phys = pfs_udf_pmap_get(p_sbi, phys_ref);
    if (NULL == p_phys || PFS_UDF_PMAP_TYPE1 != p_phys->type)
    {
        return (int64_t)PFS_UDF_RET_EBADMSG;
    }
    if (phys_lba >= p_phys->phys_len)
    {
        return (int64_t)PFS_UDF_RET_EBADMSG;
    }
    return pfs_udf_type1_lba_to_off(p_sbi, p_phys, phys_lba);
}

int pfs_udf_read_part_block_ref(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref,
        uint32_t part_lba, void *buf)
{
    if (NULL == p_sbi || NULL == buf || NULL == p_sbi->p_blkio || 0 == p_sbi->block_size)
    {
        return PFS_UDF_RET_EINVAL;
    }

    int64_t off = pfs_udf_map_to_off(p_sbi, part_ref, part_lba);
    if (off < 0)
    {
        return (int)off;
    }
    int nread = pfs_blkio_pread(p_sbi->p_blkio, buf, (int)p_sbi->block_size, off);
    if (nread != (int)p_sbi->block_size)
    {
        if (nread < 0)
        {
            return pfs_udf_map_blkio_rc(nread);
        }
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_read_phys_block(const pfs_udf_sbi_t *p_sbi, uint16_t type1_ref,
        uint32_t part_lba, void *buf)
{
    const pfs_udf_pmap_t *p_map = pfs_udf_pmap_get(p_sbi, type1_ref);

    if (NULL == p_map || PFS_UDF_PMAP_TYPE1 != p_map->type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (part_lba >= p_map->phys_len)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    int64_t off = pfs_udf_type1_lba_to_off(p_sbi, p_map, part_lba);
    if (off < 0)
    {
        return (int)off;
    }
    int nread = pfs_blkio_pread(p_sbi->p_blkio, buf, (int)p_sbi->block_size, off);
    if (nread != (int)p_sbi->block_size)
    {
        if (nread < 0)
        {
            return pfs_udf_map_blkio_rc(nread);
        }
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

uint32_t pfs_udf_part_capacity_blocks(const pfs_udf_sbi_t *p_sbi, uint16_t part_ref)
{
    const pfs_udf_pmap_t *p_map = pfs_udf_pmap_get(p_sbi, part_ref);

    if (NULL == p_map)
    {
        return 0;
    }
    switch (p_map->type)
    {
    case PFS_UDF_PMAP_TYPE1:
        return p_map->phys_len;
    case PFS_UDF_PMAP_VIRT15:
    case PFS_UDF_PMAP_VIRT20:
        return p_map->vat_entries;
    case PFS_UDF_PMAP_META25:
        if (0 == p_map->have_meta_inode || 0 == p_sbi->block_size)
        {
            return 0;
        }
        return (uint32_t)(p_map->meta_inode.size / (uint64_t)p_sbi->block_size);
    default:
        return 0;
    }
}

void pfs_udf_maps_free(pfs_udf_sbi_t *p_sbi)
{

    if (NULL == p_sbi)
    {
        return;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (NULL != p_sbi->maps[i].vat_entries_tbl)
        {
            pfs_free(p_sbi->maps[i].vat_entries_tbl);
            p_sbi->maps[i].vat_entries_tbl = NULL;
        }
        p_sbi->maps[i].vat_entries = 0;
        p_sbi->maps[i].have_meta_inode = 0;
    }
}

static int pfs_udf_fe_is_vat(const uint8_t *p_buf, int buf_len, int virt20)
{
    if (NULL == p_buf || buf_len < (int)sizeof(pfs_udf_file_entry_t))
    {
        return 0;
    }

    const pfs_udf_file_entry_t *p_fe = (const pfs_udf_file_entry_t *)p_buf;
    uint16_t tag_ident = pfs_udf_le16(&p_fe->descTag.tagIdent);

    if (PFS_UDF_TAG_IDENT_FE != tag_ident && PFS_UDF_TAG_IDENT_EFE != tag_ident)
    {
        return 0;
    }

    uint8_t file_type = p_fe->icbTag.fileType;
    if (virt20)
    {
        return (PFS_UDF_ICBTAG_FILE_VAT20 == file_type) ? 1 : 0;
    }
    return (PFS_UDF_ICBTAG_FILE_VAT20 == file_type ||
            0 == file_type) ? 1 : 0;
}

static int pfs_udf_read_inode_bytes_phys(const pfs_udf_sbi_t *p_sbi,
        const pfs_udf_inode_t *p_ino, uint16_t phys_ref, uint64_t pos,
        void *buf, int len)
{
    if (NULL == p_sbi || NULL == p_ino || NULL == buf || len < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if ((uint64_t)len > p_ino->size - pos)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint8_t *out = (uint8_t *)buf;
    int left = len;
    uint64_t cur = pos;

    for (int si = 0; si < p_ino->n_extents && left > 0; si++)
    {
        uint64_t ext_len = (uint64_t)p_ino->sec[si].len;

        if (cur >= ext_len)
        {
            cur -= ext_len;
            continue;
        }

        int chunk = (int)(ext_len - cur);

        if (chunk > left)
        {
            chunk = left;
        }

        int64_t disk_off = pfs_udf_type1_lba_to_off(p_sbi, &p_sbi->maps[phys_ref],
                p_ino->sec[si].block);
        if (disk_off < 0)
        {
            return (int)disk_off;
        }
        disk_off += (int64_t)p_ino->sec[si].skip + (int64_t)cur;
        int rc = pfs_blkio_pread(p_sbi->p_blkio, out, chunk, disk_off);
        if (rc < 0)
        {
            return pfs_udf_map_blkio_rc(rc);
        }
        if (rc != chunk)
        {
            return PFS_UDF_RET_EIO;
        }
        out += chunk;
        left -= chunk;
        cur = 0;
    }
    if (left > 0)
    {
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_icb_from_meta_fe_buf(const pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint32_t iblock, uint16_t part_ref, pfs_udf_inode_t *p_out)
{

    if (NULL == p_buf || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int rc = pfs_udf_icb_from_fe_buf_ex(p_sbi, p_buf, buf_len, iblock, part_ref, p_out, 1);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_out->alloc_type)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_load_meta_inode(pfs_udf_sbi_t *p_sbi, pfs_udf_pmap_t *p_map,
        uint16_t phys_ref, uint32_t file_loc)
{
    uint8_t *blk;

    blk = (uint8_t *)pfs_malloc((size_t)p_sbi->block_size);
    if (NULL == blk)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    int rc = pfs_udf_read_phys_block(p_sbi, phys_ref, file_loc, blk);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(blk);
        return rc;
    }
    rc = pfs_udf_icb_from_meta_fe_buf(p_sbi, blk, (int)p_sbi->block_size, file_loc,
            phys_ref, &p_map->meta_inode);
    pfs_free(blk);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    p_map->have_meta_inode = 1;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_load_vat_table(pfs_udf_sbi_t *p_sbi, pfs_udf_pmap_t *p_map,
        const pfs_udf_inode_t *p_vat)
{
    uint64_t vat_size = p_vat->size;
    uint32_t start_off = p_map->vat_start_offset;

    if (vat_size <= (uint64_t)start_off)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint64_t payload;

    // VAT1.5: 文件尾 36 字节为识别串, 不计入表项 (对照 linux udf_load_vat)
    if (PFS_UDF_PMAP_VIRT15 == p_map->type)
    {
        if (vat_size < (uint64_t)start_off + 36u)
        {
            return PFS_UDF_RET_EBADMSG;
        }
        payload = vat_size - (uint64_t)start_off - 36u;
    }
    else
    {
        payload = vat_size - (uint64_t)start_off;
    }

    uint32_t num_entries = (uint32_t)(payload >> 2);

    if (0 == num_entries || num_entries > PFS_UDF_MAX_VAT_ENTRIES)
    {
        return PFS_UDF_RET_ERANGE;
    }

    uint8_t *vat_buf = (uint8_t *)pfs_malloc((size_t)vat_size);
    if (NULL == vat_buf)
    {
        return PFS_UDF_RET_ENOMEM;
    }
    int rc = pfs_udf_read_inode_bytes_phys(p_sbi, p_vat, p_map->vat_phys_part_ref, 0,
            vat_buf, (int)vat_size);
    if (PFS_UDF_RET_OK != rc)
    {
        pfs_free(vat_buf);
        return rc;
    }

    p_map->vat_entries_tbl = (uint32_t *)pfs_malloc((size_t)num_entries * sizeof(uint32_t));
    if (NULL == p_map->vat_entries_tbl)
    {
        pfs_free(vat_buf);
        return PFS_UDF_RET_ENOMEM;
    }
    for (uint32_t i = 0; i < num_entries; i++)
    {
        p_map->vat_entries_tbl[i] = pfs_udf_le32(vat_buf + start_off + (size_t)i * 4u);
    }
    p_map->vat_entries = num_entries;
    pfs_free(vat_buf);
    return PFS_UDF_RET_OK;
}

static int pfs_udf_parse_vat_fe(pfs_udf_sbi_t *p_sbi, const uint8_t *p_buf,
        int buf_len, uint16_t phys_ref, uint32_t iblock, int virt20,
        pfs_udf_inode_t *p_vat, uint16_t *p_start_off)
{
    if (0 == pfs_udf_fe_is_vat(p_buf, buf_len, virt20))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    const pfs_udf_file_entry_t *p_fe = (const pfs_udf_file_entry_t *)p_buf;
    uint16_t flags = pfs_udf_le16(&p_fe->icbTag.flags);
    int alloc_type = (int)(flags & 0x0007u);
    int hdr_sz = (PFS_UDF_TAG_IDENT_EFE == pfs_udf_le16(&p_fe->descTag.tagIdent)) ?
            (int)sizeof(pfs_udf_extended_file_entry_t) :
            (int)sizeof(pfs_udf_file_entry_t);
    uint32_t len_eattr = pfs_udf_le32(&p_fe->lengthExtendedAttr);
    uint32_t len_alloc = pfs_udf_le32(&p_fe->lengthAllocDescs);

    memset(p_vat, 0, sizeof(*p_vat));
    p_vat->iblock = iblock;
    p_vat->part_ref = phys_ref;
    p_vat->alloc_type = alloc_type;
    p_vat->size = pfs_udf_le64(&p_fe->informationLength);
    p_vat->type = PFS_UDF_TYPE_FILE;

    if (virt20)
    {
        if (PFS_UDF_ICBTAG_AD_IN_ICB == alloc_type)
        {
            uint32_t data_off = (uint32_t)hdr_sz + len_eattr;

            *p_start_off = pfs_udf_le16(&((const pfs_udf_vat20_hdr_t *)
                    (p_buf + data_off))->lengthHeader);
        }
        else
        {
            int rc = pfs_udf_icb_from_fe_buf_ex(p_sbi, p_buf, buf_len, iblock, phys_ref,
                    p_vat, 1);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
            if (p_vat->size < sizeof(pfs_udf_vat20_hdr_t))
            {
                return PFS_UDF_RET_EBADMSG;
            }

            pfs_udf_vat20_hdr_t vat20;

            rc = pfs_udf_read_inode_bytes_phys(p_sbi, p_vat, phys_ref, 0,
                    (uint8_t *)&vat20, (int)sizeof(vat20));
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
            *p_start_off = pfs_udf_le16(&vat20.lengthHeader);
            return PFS_UDF_RET_OK;
        }
    }
    else
    {
        *p_start_off = 0;
    }

    if (PFS_UDF_ICBTAG_AD_IN_ICB == alloc_type)
    {
        uint32_t data_off = (uint32_t)hdr_sz + len_eattr;

        p_vat->n_extents = 0;
        return pfs_udf_add_extent(p_vat, iblock, data_off, (uint32_t)p_vat->size);
    }

    return pfs_udf_icb_from_fe_buf_ex(p_sbi, p_buf, buf_len, iblock, phys_ref, p_vat, 1);
}

static int pfs_udf_load_vat_map(pfs_udf_sbi_t *p_sbi, pfs_udf_pmap_t *p_map)
{
    if (PFS_UDF_PMAP_VIRT15 != p_map->type && PFS_UDF_PMAP_VIRT20 != p_map->type)
    {
        return PFS_UDF_RET_EINVAL;
    }

    int virt20 = (PFS_UDF_PMAP_VIRT20 == p_map->type) ? 1 : 0;

    p_map->vat_phys_part_ref = (uint16_t)pfs_udf_find_type1_map_idx(p_sbi, p_map->partition_num);
    if (p_map->vat_phys_part_ref >= (uint16_t)p_sbi->n_maps)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    const pfs_udf_pmap_t *p_phys = &p_sbi->maps[p_map->vat_phys_part_ref];

    if (0 == p_phys->phys_len)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    uint8_t *blk = (uint8_t *)pfs_malloc((size_t)p_sbi->block_size);
    if (NULL == blk)
    {
        return PFS_UDF_RET_ENOMEM;
    }

    int rc = PFS_UDF_RET_EBADMSG;
    uint32_t start = p_phys->phys_len - 1u;

    if (start > 3u)
    {
        start -= 3u;
    }
    else
    {
        start = 0;
    }

    for (uint32_t scan_lba = p_phys->phys_len - 1u; scan_lba >= start; scan_lba--)
    {
        pfs_udf_inode_t vat;
        uint16_t vat_start_off;

        if (PFS_UDF_RET_OK != pfs_udf_read_phys_block(p_sbi, p_map->vat_phys_part_ref,
                scan_lba, blk))
        {
            continue;
        }
        if (PFS_UDF_RET_OK != pfs_udf_parse_vat_fe(p_sbi, blk, (int)p_sbi->block_size,
                p_map->vat_phys_part_ref, scan_lba, virt20, &vat, &vat_start_off))
        {
            continue;
        }
        p_map->vat_start_offset = vat_start_off;
        rc = pfs_udf_load_vat_table(p_sbi, p_map, &vat);
        if (PFS_UDF_RET_OK == rc)
        {
            break;
        }
        if (NULL != p_map->vat_entries_tbl)
        {
            pfs_free(p_map->vat_entries_tbl);
            p_map->vat_entries_tbl = NULL;
            p_map->vat_entries = 0;
        }
        if (PFS_UDF_RET_ERANGE == rc)
        {
            break;
        }
        rc = PFS_UDF_RET_EBADMSG;
        if (0 == scan_lba)
        {
            break;
        }
    }

    pfs_free(blk);
    return rc;
}

static int pfs_udf_load_meta_map(pfs_udf_sbi_t *p_sbi, pfs_udf_pmap_t *p_map)
{
    uint16_t phys_ref = (uint16_t)pfs_udf_find_type1_map_idx(p_sbi, p_map->partition_num);

    if (phys_ref >= (uint16_t)p_sbi->n_maps)
    {
        return PFS_UDF_RET_EBADMSG;
    }
    p_map->meta_phys_part_ref = phys_ref;

    int rc = pfs_udf_load_meta_inode(p_sbi, p_map, phys_ref, p_map->meta_file_loc);
    if (PFS_UDF_RET_OK != rc && 0 != p_map->mirror_file_loc)
    {
        rc = pfs_udf_load_meta_inode(p_sbi, p_map, phys_ref, p_map->mirror_file_loc);
    }
    return rc;
}

int pfs_udf_maps_postload(pfs_udf_sbi_t *p_sbi)
{

    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        pfs_udf_pmap_t *p_map;

        p_map = &p_sbi->maps[i];
        if (PFS_UDF_PMAP_VIRT15 == p_map->type || PFS_UDF_PMAP_VIRT20 == p_map->type)
        {
            int rc = pfs_udf_load_vat_map(p_sbi, p_map);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
        else if (PFS_UDF_PMAP_META25 == p_map->type)
        {
            int rc = pfs_udf_load_meta_map(p_sbi, p_map);
            if (PFS_UDF_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_pmap_add_type1(pfs_udf_sbi_t *p_sbi, const pfs_udf_partmap1_t *p_m1)
{
    if (NULL == p_sbi || NULL == p_m1 || p_sbi->n_maps >= PFS_UDF_MAX_MAPS)
    {
        return PFS_UDF_RET_ERANGE;
    }

    pfs_udf_pmap_t *p_map = &p_sbi->maps[p_sbi->n_maps];

    memset(p_map, 0, sizeof(*p_map));
    p_map->type = PFS_UDF_PMAP_TYPE1;
    p_map->vol_seq = pfs_udf_le16(&p_m1->volSeqNum);
    p_map->partition_num = pfs_udf_le16(&p_m1->partitionNum);
    p_sbi->n_maps++;
    return PFS_UDF_RET_OK;
}

int pfs_udf_pmap_add_type2(pfs_udf_sbi_t *p_sbi, const uint8_t *p_map_raw, uint8_t map_len)
{
    if (NULL == p_sbi || NULL == p_map_raw || p_sbi->n_maps >= PFS_UDF_MAX_MAPS)
    {
        return PFS_UDF_RET_ERANGE;
    }
    if (map_len < sizeof(pfs_udf_partmap2_t))
    {
        return PFS_UDF_RET_EBADMSG;
    }

    const pfs_udf_partmap2_t *p_m2 = (const pfs_udf_partmap2_t *)p_map_raw;

    if (pfs_udf_regid_prefix(&p_m2->partIdent, PFS_UDF_ID_SPARABLE))
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    pfs_udf_pmap_t *p_map = &p_sbi->maps[p_sbi->n_maps];

    memset(p_map, 0, sizeof(*p_map));
    p_map->vol_seq = pfs_udf_le16(&p_m2->volSeqNum);
    p_map->partition_num = pfs_udf_le16(&p_m2->partitionNum);

    if (pfs_udf_regid_prefix(&p_m2->partIdent, PFS_UDF_ID_VIRTUAL))
    {
        uint16_t suffix = pfs_udf_le16(p_m2->partIdent.identSuffix);

        p_map->type = (suffix < 0x0200u) ? PFS_UDF_PMAP_VIRT15 : PFS_UDF_PMAP_VIRT20;
    }
    else if (pfs_udf_regid_prefix(&p_m2->partIdent, PFS_UDF_ID_METADATA))
    {
        if (map_len < sizeof(pfs_udf_metadata_partmap_t))
        {
            return PFS_UDF_RET_EBADMSG;
        }

        const pfs_udf_metadata_partmap_t *p_mdm = (const pfs_udf_metadata_partmap_t *)p_map_raw;

        p_map->type = PFS_UDF_PMAP_META25;
        p_map->meta_file_loc = pfs_udf_le32(&p_mdm->metadataFileLoc);
        p_map->mirror_file_loc = pfs_udf_le32(&p_mdm->metadataMirrorFileLoc);
    }
    else
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    p_sbi->n_maps++;
    return PFS_UDF_RET_OK;
}

void pfs_udf_pmap_apply_pd(pfs_udf_sbi_t *p_sbi, uint16_t partition_number,
        uint32_t partition_start, uint32_t partition_len)
{

    if (NULL == p_sbi)
    {
        return;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (p_sbi->maps[i].partition_num == partition_number)
        {
            p_sbi->maps[i].phys_start = partition_start;
            p_sbi->maps[i].phys_len = partition_len;
        }
    }
}

int pfs_udf_pmap_have_type1_pd(const pfs_udf_sbi_t *p_sbi)
{

    if (NULL == p_sbi)
    {
        return 0;
    }
    for (int i = 0; i < p_sbi->n_maps; i++)
    {
        if (PFS_UDF_PMAP_TYPE1 == p_sbi->maps[i].type && 0 != p_sbi->maps[i].phys_len)
        {
            return 1;
        }
    }
    return 0;
}

int pfs_udf_write_part_block_ref(pfs_udf_sbi_t *p_sbi, uint16_t part_ref,
        uint32_t part_lba, const void *buf)
{
    if (NULL == p_sbi || NULL == buf || NULL == p_sbi->p_blkio || 0 == p_sbi->block_size)
    {
        return PFS_UDF_RET_EINVAL;
    }

    int64_t off = pfs_udf_map_to_off(p_sbi, part_ref, part_lba);
    if (off < 0)
    {
        return (int)off;
    }
    int nwrite = pfs_blkio_pwrite(p_sbi->p_blkio, buf, (int)p_sbi->block_size, off);
    if (nwrite != (int)p_sbi->block_size)
    {
        if (nwrite < 0)
        {
            return pfs_udf_map_blkio_rc(nwrite);
        }
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_write_part_block(pfs_udf_sbi_t *p_sbi, uint32_t part_lba, const void *buf)
{
    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == p_sbi->n_maps)
    {
        return PFS_UDF_RET_EINVAL;
    }

    uint16_t part_ref = p_sbi->root.part_ref;

    return pfs_udf_write_part_block_ref(p_sbi, part_ref, part_lba, buf);
}

// end
