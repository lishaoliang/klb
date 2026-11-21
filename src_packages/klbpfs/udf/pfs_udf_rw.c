// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF Type1 limited RW: create/write/truncate/unlink/mkdir/rmdir (AD_SHORT only)
#include "udf/pfs_udf_internal.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <time.h>

#define PFS_UDF_DESC_VERSION            0x0003u
#define PFS_UDF_CS0_MAX                 254
#define PFS_UDF_PATH_PARENT_MAX         512

static int pfs_udf_rw_fid_round_up(int len)
{
    return (len + 3) & ~3;
}

static void pfs_udf_rw_fill_timestamp(uint8_t ts[12])
{
    if (NULL == ts)
    {
        return;
    }
    memset(ts, 0, 12);
    time_t now = time(NULL);
    struct tm tmb;
    struct tm *p_tm;
#ifdef _WIN32
    if (0 != localtime_s(&tmb, &now))
    {
        return;
    }
    p_tm = &tmb;
#else
    p_tm = localtime_r(&now, &tmb);
    if (NULL == p_tm)
    {
        return;
    }
#endif
    int year = p_tm->tm_year + 1900;
    int cent = year / 100;
    year = year % 100;
    ts[0] = (uint8_t)cent;
    ts[1] = (uint8_t)year;
    ts[2] = (uint8_t)(p_tm->tm_mon + 1);
    ts[3] = (uint8_t)p_tm->tm_mday;
    ts[4] = (uint8_t)p_tm->tm_hour;
    ts[5] = (uint8_t)p_tm->tm_min;
    ts[6] = (uint8_t)p_tm->tm_sec;
}

static uint32_t pfs_udf_rw_blocks_for_bytes(uint32_t bytes, uint32_t block_size)
{
    if (0 == bytes)
    {
        return 0;
    }
    return (bytes + block_size - 1u) / block_size;
}

static uint32_t pfs_udf_rw_dir_data_lba(const pfs_udf_inode_t *p_dir)
{
    if (NULL == p_dir || 0 == p_dir->n_extents)
    {
        return 0;
    }
    return p_dir->sec[0].block;
}

static int pfs_udf_rw_write_inode_bytes(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_ino,
        uint64_t pos, const void *buf, int len)
{
    int64_t file_off;
    const uint8_t *in;
    int left;

    if (NULL == p_sbi || NULL == p_ino || NULL == buf || len < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if ((uint64_t)len > p_ino->size - pos)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    in = (const uint8_t *)buf;
    left = len;
    file_off = (int64_t)pos;
    for (int si = 0; si < p_ino->n_extents && left > 0; si++)
    {
        uint32_t sec_sz;
        int64_t off_in_sec;
        int chunk;
        int64_t disk_off;

        sec_sz = p_ino->sec[si].len;
        if (file_off >= (int64_t)sec_sz)
        {
            file_off -= (int64_t)sec_sz;
            continue;
        }
        off_in_sec = file_off;
        chunk = (int)((int64_t)sec_sz - off_in_sec);
        if (chunk > left)
        {
            chunk = left;
        }
        disk_off = pfs_udf_map_to_off(p_sbi, p_ino->part_ref, p_ino->sec[si].block);
        if (disk_off < 0)
        {
            return (int)disk_off;
        }
        disk_off += (int64_t)p_ino->sec[si].skip + off_in_sec;
        int rc = pfs_blkio_pwrite(p_sbi->p_blkio, in, chunk, disk_off);
        if (rc < 0)
        {
            return pfs_udf_map_blkio_rc(rc);
        }
        if (rc != chunk)
        {
            return PFS_UDF_RET_EIO;
        }
        in += chunk;
        left -= chunk;
        file_off = 0;
    }
    if (left > 0)
    {
        return PFS_UDF_RET_EIO;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_rw_path_parent_leaf(pfs_udf_sbi_t *p_sbi, const char *path,
        pfs_udf_inode_t *p_parent, char *leaf, int leaf_cap)
{
    const char *leaf_start;
    int parent_len;
    char parent_path[PFS_UDF_PATH_PARENT_MAX];

    if (NULL == p_sbi || NULL == path || NULL == p_parent || NULL == leaf || leaf_cap <= 0)
    {
        return PFS_UDF_RET_EINVAL;
    }

    leaf_start = path + strlen(path);
    while (leaf_start > path && ('/' == leaf_start[-1] || '\\' == leaf_start[-1]))
    {
        leaf_start--;
    }
    while (leaf_start > path && '/' != leaf_start[-1] && '\\' != leaf_start[-1])
    {
        leaf_start--;
    }
    if (leaf_start == path && ('/' == *path || '\\' == *path))
    {
        return PFS_UDF_RET_EINVAL;
    }
    if ((int)strlen(leaf_start) >= leaf_cap)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    memcpy(leaf, leaf_start, strlen(leaf_start) + 1U);

    parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && ('/' == path[parent_len - 1] || '\\' == path[parent_len - 1]))
    {
        parent_len--;
    }
    if (0 == parent_len)
    {
        *p_parent = p_sbi->root;
        return PFS_UDF_RET_OK;
    }
    if (parent_len >= (int)sizeof(parent_path))
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    memcpy(parent_path, path, (size_t)parent_len);
    parent_path[parent_len] = '\0';
    int rc = pfs_udf_lookup_path(p_sbi, parent_path, p_parent);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_UDF_TYPE_DIR != p_parent->type)
    {
        return PFS_UDF_RET_ENOENT;
    }
    return PFS_UDF_RET_OK;
}

static void pfs_udf_rw_set_long_ad(pfs_udf_long_ad_t *p_la, uint16_t part_ref,
        uint32_t part_lba, uint32_t len_bytes)
{
    memset(p_la, 0, sizeof(*p_la));
    pfs_udf_wle32(&p_la->extLength, len_bytes | PFS_UDF_EXT_RECORDED_ALLOC);
    pfs_udf_wle32(&p_la->extLocation.logicalBlockNum, part_lba);
    pfs_udf_wle16(&p_la->extLocation.partitionReferenceNum, part_ref);
}

static void pfs_udf_rw_set_short_ad(pfs_udf_short_ad_t *p_sa, uint32_t part_lba,
        uint32_t len_bytes)
{
    memset(p_sa, 0, sizeof(*p_sa));
    pfs_udf_wle32(&p_sa->extLength, len_bytes | PFS_UDF_EXT_RECORDED_ALLOC);
    pfs_udf_wle32(&p_sa->extPosition, part_lba);
}

static int pfs_udf_rw_rewrite_fe(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_ino)
{
    uint8_t blk[4096];
    pfs_udf_file_entry_t *p_fe;
    pfs_udf_short_ad_t *p_sa;
    int hdr_sz;
    int crc_len;
    int ad_bytes;

    if (NULL == p_sbi || NULL == p_ino || 0 == p_sbi->block_size ||
            p_sbi->block_size > sizeof(blk))
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_ino->alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    int rc = pfs_udf_read_part_block_ref(p_sbi, p_ino->part_ref, p_ino->iblock, blk);
    if (PFS_UDF_RET_OK != rc)
    {
        memset(blk, 0, (size_t)p_sbi->block_size);
    }

    p_fe = (pfs_udf_file_entry_t *)blk;
    hdr_sz = (int)sizeof(pfs_udf_file_entry_t);
    ad_bytes = p_ino->n_extents * (int)sizeof(pfs_udf_short_ad_t);
    crc_len = hdr_sz - (int)sizeof(pfs_udf_tag_t) + ad_bytes;

    pfs_udf_wle16(&p_fe->icbTag.flags, PFS_UDF_ICBTAG_AD_SHORT);
    if (PFS_UDF_TYPE_DIR == p_ino->type)
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_DIRECTORY;
    }
    else
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_REGULAR;
    }
    pfs_udf_wle64(&p_fe->informationLength, p_ino->size);
    pfs_udf_rw_fill_timestamp(p_fe->modificationTime);
    pfs_udf_wle32(&p_fe->lengthAllocDescs, (uint32_t)ad_bytes);

    p_sa = (pfs_udf_short_ad_t *)(blk + hdr_sz);
    for (int i = 0; i < p_ino->n_extents; i++)
    {
        pfs_udf_rw_set_short_ad(&p_sa[i], p_ino->sec[i].block, p_ino->sec[i].len);
    }

    pfs_udf_set_tag(&p_fe->descTag, PFS_UDF_TAG_IDENT_FE, p_ino->iblock, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len, PFS_UDF_DESC_VERSION);
    return pfs_udf_write_part_block_ref(p_sbi, p_ino->part_ref, p_ino->iblock, blk);
}

static int pfs_udf_rw_dir_grow(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_dir, uint32_t need_bytes)
{
    uint32_t alloc_bytes;
    uint32_t have_bytes;
    uint32_t add_blocks;
    uint32_t start_lba;

    if (NULL == p_sbi || NULL == p_dir)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_dir->alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    have_bytes = 0;
    for (int i = 0; i < p_dir->n_extents; i++)
    {
        have_bytes += p_dir->sec[i].len;
    }
    if (need_bytes <= have_bytes)
    {
        return PFS_UDF_RET_OK;
    }

    alloc_bytes = need_bytes - have_bytes;
    add_blocks = pfs_udf_rw_blocks_for_bytes(alloc_bytes, p_sbi->block_size);
    if (0 == add_blocks)
    {
        add_blocks = 1;
    }

    int rc = pfs_udf_alloc_blocks(p_sbi, add_blocks, &start_lba);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    if (p_dir->n_extents > 0)
    {
        pfs_udf_extent_t *p_last;
        uint32_t last_end;

        p_last = &p_dir->sec[p_dir->n_extents - 1];
        last_end = p_last->block + pfs_udf_rw_blocks_for_bytes(p_last->len, p_sbi->block_size);
        if (start_lba == last_end)
        {
            p_last->len += add_blocks * p_sbi->block_size;
        }
        else
        {
            rc = pfs_udf_add_extent(p_dir, start_lba, 0, add_blocks * p_sbi->block_size);
            if (PFS_UDF_RET_OK != rc)
            {
                (void)pfs_udf_free_blocks(p_sbi, start_lba, add_blocks);
                return rc;
            }
        }
    }
    else
    {
        rc = pfs_udf_add_extent(p_dir, start_lba, 0, add_blocks * p_sbi->block_size);
        if (PFS_UDF_RET_OK != rc)
        {
            (void)pfs_udf_free_blocks(p_sbi, start_lba, add_blocks);
            return rc;
        }
    }

    if (p_dir->size < need_bytes)
    {
        p_dir->size = need_bytes;
    }
    return pfs_udf_rw_rewrite_fe(p_sbi, p_dir);
}

static int pfs_udf_rw_write_fid(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_dir,
        uint64_t pos, const char *name_utf8, int is_dir, int is_parent,
        uint32_t icb_lba, int mark_deleted)
{
    uint8_t cs0[PFS_UDF_CS0_MAX];
    uint8_t fid_buf[512];
    pfs_udf_file_ident_desc_t *p_fid;
    uint32_t data_lba;
    int cs0_len;
    int imp_len;
    int total;

    if (NULL == p_sbi || NULL == p_dir)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int rc;
    cs0_len = 0;
    if (is_parent)
    {
        cs0[0] = 8;
        cs0[1] = '.';
        cs0_len = 2;
    }
    else
    {
        rc = pfs_udf_utf8_to_cs0(name_utf8, cs0, (int)sizeof(cs0), &cs0_len);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }

    imp_len = 0;
    total = (int)sizeof(pfs_udf_file_ident_desc_t) + imp_len + cs0_len;
    total = pfs_udf_rw_fid_round_up(total);
    if (total > (int)sizeof(fid_buf))
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    if ((uint64_t)pos + (uint64_t)total > p_dir->size)
    {
        rc = pfs_udf_rw_dir_grow(p_sbi, p_dir, (uint32_t)(pos + (uint64_t)total));
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }

    memset(fid_buf, 0, (size_t)total);
    p_fid = (pfs_udf_file_ident_desc_t *)fid_buf;
    data_lba = pfs_udf_rw_dir_data_lba(p_dir);
    pfs_udf_wle16(&p_fid->fileVersionNum, 1);
    p_fid->fileCharacteristics = 0;
    if (is_dir)
    {
        p_fid->fileCharacteristics = (uint8_t)(p_fid->fileCharacteristics | PFS_UDF_FID_CHAR_DIRECTORY);
    }
    if (is_parent)
    {
        p_fid->fileCharacteristics = (uint8_t)(p_fid->fileCharacteristics | PFS_UDF_FID_CHAR_PARENT);
    }
    if (mark_deleted)
    {
        p_fid->fileCharacteristics = (uint8_t)(p_fid->fileCharacteristics | PFS_UDF_FID_CHAR_DELETED);
    }
    p_fid->lengthFileIdent = (uint8_t)cs0_len;
    pfs_udf_rw_set_long_ad(&p_fid->icb, p_dir->part_ref, icb_lba, p_sbi->block_size);
    pfs_udf_wle16(&p_fid->lengthOfImpUse, 0);
    if (cs0_len > 0)
    {
        memcpy(fid_buf + sizeof(pfs_udf_file_ident_desc_t), cs0, (size_t)cs0_len);
    }
    {
        uint32_t tag_lba;

        tag_lba = data_lba + (uint32_t)((uint32_t)pos / p_sbi->block_size);
        pfs_udf_set_tag(&p_fid->descTag, PFS_UDF_TAG_IDENT_FID, tag_lba, fid_buf,
                (int)sizeof(pfs_udf_tag_t), total - (int)sizeof(pfs_udf_tag_t),
                PFS_UDF_DESC_VERSION);
    }

    rc = pfs_udf_rw_write_inode_bytes(p_sbi, p_dir, pos, fid_buf, total);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (pos + (uint64_t)total > p_dir->size)
    {
        p_dir->size = pos + (uint64_t)total;
        rc = pfs_udf_rw_rewrite_fe(p_sbi, p_dir);
    }
    return rc;
}

static int pfs_udf_rw_append_fid(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_dir,
        const char *name, int is_dir, uint32_t icb_lba)
{
    uint64_t pos;

    if (NULL == p_dir)
    {
        return PFS_UDF_RET_EINVAL;
    }
    pos = p_dir->size;
    return pfs_udf_rw_write_fid(p_sbi, p_dir, pos, name, is_dir, 0, icb_lba, 0);
}

static int pfs_udf_rw_free_inode_data(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_ino)
{

    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_UDF_RET_EINVAL;
    }
    for (int i = 0; i < p_ino->n_extents; i++)
    {
        uint32_t blocks;
        blocks = pfs_udf_rw_blocks_for_bytes(p_ino->sec[i].len, p_sbi->block_size);
        if (blocks > 0)
        {
            (void)pfs_udf_free_blocks(p_sbi, p_ino->sec[i].block, blocks);
        }
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_rw_new_fe(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_parent,
        int is_dir, pfs_udf_inode_t *p_out)
{
    uint8_t blk[4096];
    pfs_udf_file_entry_t *p_fe;
    uint32_t fe_lba;
    uint64_t uid;
    int hdr_sz;
    int crc_len;

    if (NULL == p_sbi || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 == p_sbi->block_size || p_sbi->block_size > sizeof(blk))
    {
        return PFS_UDF_RET_EINVAL;
    }

    int rc = pfs_udf_alloc_blocks(p_sbi, 1, &fe_lba);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    memset(blk, 0, (size_t)p_sbi->block_size);
    p_fe = (pfs_udf_file_entry_t *)blk;
    hdr_sz = (int)sizeof(pfs_udf_file_entry_t);
    crc_len = hdr_sz - (int)sizeof(pfs_udf_tag_t);

    pfs_udf_wle16(&p_fe->icbTag.strategyType, 4);
    pfs_udf_wle16(&p_fe->icbTag.numEntries, 1);
    if (is_dir)
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_DIRECTORY;
    }
    else
    {
        p_fe->icbTag.fileType = PFS_UDF_ICBTAG_FILE_REGULAR;
    }
    if (NULL != p_parent)
    {
        pfs_udf_wle32(&p_fe->icbTag.parentICBLocation.logicalBlockNum, p_parent->iblock);
        pfs_udf_wle16(&p_fe->icbTag.parentICBLocation.partitionReferenceNum, p_parent->part_ref);
    }
    pfs_udf_wle16(&p_fe->icbTag.flags, PFS_UDF_ICBTAG_AD_SHORT);
    pfs_udf_wle32(&p_fe->permissions, 0x1edu);
    pfs_udf_wle16(&p_fe->fileLinkCount, 1);
    pfs_udf_wle64(&p_fe->informationLength, 0);
    pfs_udf_rw_fill_timestamp(p_fe->accessTime);
    pfs_udf_rw_fill_timestamp(p_fe->modificationTime);
    pfs_udf_rw_fill_timestamp(p_fe->attrTime);
    if (0 == p_sbi->next_unique_id)
    {
        p_sbi->next_unique_id = 16;
    }
    uid = p_sbi->next_unique_id++;
    pfs_udf_wle64(&p_fe->uniqueID, uid);
    pfs_udf_wle32(&p_fe->lengthAllocDescs, 0);
    pfs_udf_set_tag(&p_fe->descTag, PFS_UDF_TAG_IDENT_FE, fe_lba, blk,
            (int)sizeof(pfs_udf_tag_t), crc_len, PFS_UDF_DESC_VERSION);

    rc = pfs_udf_write_part_block_ref(p_sbi, p_sbi->root.part_ref, fe_lba, blk);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_free_blocks(p_sbi, fe_lba, 1);
        return rc;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->type = is_dir ? PFS_UDF_TYPE_DIR : PFS_UDF_TYPE_FILE;
    p_out->size = 0;
    p_out->iblock = fe_lba;
    p_out->part_ref = p_sbi->root.part_ref;
    p_out->alloc_type = PFS_UDF_ICBTAG_AD_SHORT;
    p_out->n_extents = 0;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_rw_dir_init_data(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_dir,
        const pfs_udf_inode_t *p_parent)
{
    uint32_t data_lba;
    uint32_t parent_fe;

    int rc = pfs_udf_alloc_blocks(p_sbi, 1, &data_lba);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_udf_add_extent(p_dir, data_lba, 0, p_sbi->block_size);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_free_blocks(p_sbi, data_lba, 1);
        return rc;
    }
    p_dir->size = 0;

    if (NULL != p_parent)
    {
        parent_fe = p_parent->iblock;
    }
    else
    {
        parent_fe = p_dir->iblock;
    }
    rc = pfs_udf_rw_write_fid(p_sbi, p_dir, 0, "", 1, 1, parent_fe, 0);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_rw_free_inode_data(p_sbi, p_dir);
        p_dir->n_extents = 0;
        return rc;
    }
    p_dir->size = p_sbi->block_size;
    return pfs_udf_rw_rewrite_fe(p_sbi, p_dir);
}

static int pfs_udf_rw_dir_is_empty(pfs_udf_sbi_t *p_sbi, const pfs_udf_inode_t *p_dir)
{
    uint64_t pos;

    pos = 0;
    while (pos < p_dir->size)
    {
        uint8_t hdr[sizeof(pfs_udf_file_ident_desc_t)];
        const pfs_udf_file_ident_desc_t *p_fid;
        pfs_udf_file_meta_t fm;
        int got;
        int imp_len;
        int ident_len;
        int total;

        fm.inode = *p_dir;
        int rc = pfs_udf_file_read(p_sbi, &fm, (int64_t)pos, hdr, (int)sizeof(hdr), &got);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
        if (got != (int)sizeof(hdr))
        {
            return PFS_UDF_RET_EIO;
        }
        p_fid = (const pfs_udf_file_ident_desc_t *)hdr;
        if (PFS_UDF_TAG_IDENT_FID != pfs_udf_le16(&p_fid->descTag.tagIdent))
        {
            return PFS_UDF_RET_EBADMSG;
        }
        imp_len = (int)pfs_udf_le16(&p_fid->lengthOfImpUse);
        ident_len = (int)p_fid->lengthFileIdent;
        total = pfs_udf_rw_fid_round_up((int)sizeof(*p_fid) + imp_len + ident_len);
        if (0 != (p_fid->fileCharacteristics & PFS_UDF_FID_CHAR_DELETED) ||
                0 != (p_fid->fileCharacteristics & PFS_UDF_FID_CHAR_METADATA) ||
                0 != (p_fid->fileCharacteristics & PFS_UDF_FID_CHAR_PARENT))
        {
            pos += (uint64_t)total;
            continue;
        }
        return PFS_UDF_RET_ENOTEMPTY;
    }
    return PFS_UDF_RET_OK;
}

static int pfs_udf_rw_extend_file(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_ino, uint64_t new_size)
{
    uint32_t need_bytes;
    uint32_t have_bytes;
    uint32_t add_blocks;
    uint32_t start_lba;

    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_ino->alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    need_bytes = (uint32_t)new_size;
    have_bytes = 0;
    for (int i = 0; i < p_ino->n_extents; i++)
    {
        have_bytes += p_ino->sec[i].len;
    }
    if (need_bytes <= have_bytes)
    {
        p_ino->size = new_size;
        return pfs_udf_rw_rewrite_fe(p_sbi, p_ino);
    }

    add_blocks = pfs_udf_rw_blocks_for_bytes(need_bytes - have_bytes, p_sbi->block_size);
    if (0 == add_blocks)
    {
        add_blocks = 1;
    }
    int rc = pfs_udf_alloc_blocks(p_sbi, add_blocks, &start_lba);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    if (p_ino->n_extents > 0)
    {
        pfs_udf_extent_t *p_last;
        uint32_t last_end;

        p_last = &p_ino->sec[p_ino->n_extents - 1];
        last_end = p_last->block + pfs_udf_rw_blocks_for_bytes(p_last->len, p_sbi->block_size);
        if (start_lba == last_end)
        {
            p_last->len += add_blocks * p_sbi->block_size;
        }
        else
        {
            rc = pfs_udf_add_extent(p_ino, start_lba, 0, add_blocks * p_sbi->block_size);
            if (PFS_UDF_RET_OK != rc)
            {
                (void)pfs_udf_free_blocks(p_sbi, start_lba, add_blocks);
                return rc;
            }
        }
    }
    else
    {
        rc = pfs_udf_add_extent(p_ino, start_lba, 0, add_blocks * p_sbi->block_size);
        if (PFS_UDF_RET_OK != rc)
        {
            (void)pfs_udf_free_blocks(p_sbi, start_lba, add_blocks);
            return rc;
        }
    }

    p_ino->size = new_size;
    return pfs_udf_rw_rewrite_fe(p_sbi, p_ino);
}

static int pfs_udf_rw_shrink_file(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_ino, uint64_t new_size)
{
    uint64_t cur;
    int new_n;

    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_ino->alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    cur = 0;
    new_n = 0;
    for (int i = 0; i < p_ino->n_extents; i++)
    {
        uint32_t sec_len;
        sec_len = p_ino->sec[i].len;
        if (cur >= new_size)
        {
            uint32_t blocks;
            blocks = pfs_udf_rw_blocks_for_bytes(sec_len, p_sbi->block_size);
            if (blocks > 0)
            {
                (void)pfs_udf_free_blocks(p_sbi, p_ino->sec[i].block, blocks);
            }
            continue;
        }
        if (cur + sec_len <= new_size)
        {
            p_ino->sec[new_n++] = p_ino->sec[i];
            cur += sec_len;
            continue;
        }
        p_ino->sec[new_n] = p_ino->sec[i];
        p_ino->sec[new_n].len = (uint32_t)(new_size - cur);
        {
            uint32_t tail_len;
            uint32_t tail_blocks;
            tail_len = sec_len - p_ino->sec[new_n].len;
            tail_blocks = pfs_udf_rw_blocks_for_bytes(tail_len, p_sbi->block_size);
            if (tail_blocks > 0)
            {
                uint32_t free_start;
                free_start = p_ino->sec[i].block +
                        pfs_udf_rw_blocks_for_bytes(p_ino->sec[new_n].len, p_sbi->block_size);
                (void)pfs_udf_free_blocks(p_sbi, free_start, tail_blocks);
            }
        }
        new_n++;
        cur = new_size;
    }
    p_ino->n_extents = new_n;
    p_ino->size = new_size;
    return pfs_udf_rw_rewrite_fe(p_sbi, p_ino);
}

int pfs_udf_create_file(pfs_udf_sbi_t *p_sbi, const char *path, pfs_udf_inode_t *p_out)
{
    pfs_udf_inode_t parent;
    pfs_udf_inode_t child;
    char leaf[PFS_NAME_MAX + 1];

    if (NULL == p_sbi || NULL == path || NULL == p_out)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }

    int rc = pfs_udf_rw_path_parent_leaf(p_sbi, path, &parent, leaf, (int)sizeof(leaf));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if ('\0' == leaf[0])
    {
        return PFS_UDF_RET_EINVAL;
    }
    rc = pfs_udf_find_in_dir(p_sbi, &parent, leaf, &child);
    if (PFS_UDF_RET_OK == rc)
    {
        return PFS_UDF_RET_EEXIST;
    }
    if (PFS_UDF_RET_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_udf_rw_new_fe(p_sbi, &parent, 0, &child);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_rw_append_fid(p_sbi, &parent, leaf, 0, child.iblock);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_free_blocks(p_sbi, child.iblock, 1);
        return rc;
    }
    *p_out = child;
    return PFS_UDF_RET_OK;
}

int pfs_udf_mkdir(pfs_udf_sbi_t *p_sbi, const char *path)
{
    pfs_udf_inode_t parent;
    pfs_udf_inode_t dir;
    char leaf[PFS_NAME_MAX + 1];

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }

    int rc = pfs_udf_rw_path_parent_leaf(p_sbi, path, &parent, leaf, (int)sizeof(leaf));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if ('\0' == leaf[0])
    {
        return PFS_UDF_RET_EINVAL;
    }
    rc = pfs_udf_find_in_dir(p_sbi, &parent, leaf, &dir);
    if (PFS_UDF_RET_OK == rc)
    {
        return PFS_UDF_RET_EEXIST;
    }
    if (PFS_UDF_RET_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_udf_rw_new_fe(p_sbi, &parent, 1, &dir);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_udf_rw_dir_init_data(p_sbi, &dir, &parent);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_free_blocks(p_sbi, dir.iblock, 1);
        return rc;
    }
    rc = pfs_udf_rw_append_fid(p_sbi, &parent, leaf, 1, dir.iblock);
    if (PFS_UDF_RET_OK != rc)
    {
        (void)pfs_udf_rw_free_inode_data(p_sbi, &dir);
        (void)pfs_udf_free_blocks(p_sbi, dir.iblock, 1);
        return rc;
    }
    return PFS_UDF_RET_OK;
}

int pfs_udf_file_write(pfs_udf_sbi_t *p_sbi, pfs_udf_file_meta_t *p_fm,
        int64_t pos, const void *buf, int size, int *p_put)
{
    uint64_t end;

    if (NULL == p_sbi || NULL == p_fm || NULL == buf || NULL == p_put || size < 0 || pos < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }
    if (PFS_UDF_TYPE_FILE != p_fm->inode.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_fm->inode.alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    *p_put = 0;
    if (0 == size)
    {
        return PFS_UDF_RET_OK;
    }

    int rc;
    end = (uint64_t)pos + (uint64_t)size;
    if (end > p_fm->inode.size)
    {
        rc = pfs_udf_rw_extend_file(p_sbi, &p_fm->inode, end);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }

    rc = pfs_udf_rw_write_inode_bytes(p_sbi, &p_fm->inode, (uint64_t)pos, buf, size);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    *p_put = size;
    if (end > p_fm->inode.size)
    {
        p_fm->inode.size = end;
    }
    return pfs_udf_rw_rewrite_fe(p_sbi, &p_fm->inode);
}

int pfs_udf_file_truncate(pfs_udf_sbi_t *p_sbi, pfs_udf_file_meta_t *p_fm, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fm || size < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }
    if (PFS_UDF_TYPE_FILE != p_fm->inode.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_ICBTAG_AD_SHORT != p_fm->inode.alloc_type)
    {
        return PFS_UDF_RET_EOPNOTSUPP;
    }

    if ((uint64_t)size >= p_fm->inode.size)
    {
        if ((uint64_t)size == p_fm->inode.size)
        {
            return PFS_UDF_RET_OK;
        }
        return pfs_udf_rw_extend_file(p_sbi, &p_fm->inode, (uint64_t)size);
    }
    return pfs_udf_rw_shrink_file(p_sbi, &p_fm->inode, (uint64_t)size);
}

static int pfs_udf_rw_unlink_entry(pfs_udf_sbi_t *p_sbi, pfs_udf_inode_t *p_parent,
        const char *name, int dir_only)
{
    pfs_udf_inode_t child;
    uint64_t fid_pos;
    int fid_len;

    int rc = pfs_udf_find_fid_in_dir(p_sbi, p_parent, name, &child, &fid_pos, &fid_len);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (dir_only && PFS_UDF_TYPE_DIR != child.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!dir_only && PFS_UDF_TYPE_DIR == child.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_TYPE_DIR == child.type)
    {
        rc = pfs_udf_rw_dir_is_empty(p_sbi, &child);
        if (PFS_UDF_RET_OK != rc)
        {
            return rc;
        }
    }

    rc = pfs_udf_rw_write_fid(p_sbi, p_parent, fid_pos, name,
            (PFS_UDF_TYPE_DIR == child.type) ? 1 : 0, 0, child.iblock, 1);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }

    (void)pfs_udf_rw_free_inode_data(p_sbi, &child);
    (void)pfs_udf_free_blocks(p_sbi, child.iblock, 1);
    return PFS_UDF_RET_OK;
}

int pfs_udf_unlink(pfs_udf_sbi_t *p_sbi, const char *path)
{
    pfs_udf_inode_t parent;
    char leaf[PFS_NAME_MAX + 1];

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }

    int rc = pfs_udf_rw_path_parent_leaf(p_sbi, path, &parent, leaf, (int)sizeof(leaf));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    return pfs_udf_rw_unlink_entry(p_sbi, &parent, leaf, 0);
}

int pfs_udf_rmdir(pfs_udf_sbi_t *p_sbi, const char *path)
{
    pfs_udf_inode_t parent;
    char leaf[PFS_NAME_MAX + 1];

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write(p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }

    int rc = pfs_udf_rw_path_parent_leaf(p_sbi, path, &parent, leaf, (int)sizeof(leaf));
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    return pfs_udf_rw_unlink_entry(p_sbi, &parent, leaf, 1);
}

// end
