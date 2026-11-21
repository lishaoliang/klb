// Doc Encode : UTF-8 BOM, Unix(LF)
// MFT 记录读写 / MST (linux-7.1 fs/ntfs/mft.c + mst.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

#include <stdint.h>

// 对齐 linux-7.1 post_read_mst_fixup
int pfs_ntfs_post_read_mst_fixup(void *p_buf, uint32_t size)
{
    if (NULL == p_buf || 0U == size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0U != (size & (PFS_NTFS_MST_BLOCK_SIZE - 1U)))
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    ntfs_record_t *p_rec = (ntfs_record_t *)p_buf;
    uint8_t *p_bytes = (uint8_t *)p_buf;
    uint16_t usa_ofs = pfs_le16_to_cpu(p_rec->usa_ofs);
    uint16_t usa_count = pfs_le16_to_cpu(p_rec->usa_count);
    if (0U == usa_count)
    {
        // 无 USA: 视为未保护, 成功
        return PFS_NTFS_RET_OK;
    }
    usa_count = (uint16_t)(usa_count - 1U);
    if (0U != (usa_ofs & 1U))
    {
        return PFS_NTFS_RET_OK;
    }
    if ((uint32_t)usa_ofs + (uint32_t)usa_count * 2U > size)
    {
        return PFS_NTFS_RET_OK;
    }
    if (size / PFS_NTFS_MST_BLOCK_SIZE != (uint32_t)usa_count)
    {
        return PFS_NTFS_RET_OK;
    }

    uint16_t *p_usa = (uint16_t *)(p_bytes + usa_ofs);
    uint16_t usn = pfs_le16_to_cpu(p_usa[0]);
    for (uint32_t i = 0; i < (uint32_t)usa_count; i++)
    {
        uint32_t off = (i + 1U) * PFS_NTFS_MST_BLOCK_SIZE - 2U;
        uint16_t cur = pfs_read_le16(p_bytes + off);

        if (cur != usn)
        {
            p_rec->magic = NTFS_MAGIC_BAAD;
            return PFS_NTFS_RET_EBADMSG;
        }
    }
    for (int i = 0; i < (uint32_t)usa_count; i++)
    {
        uint32_t off = (i + 1U) * PFS_NTFS_MST_BLOCK_SIZE - 2U;
        uint16_t saved = pfs_le16_to_cpu(p_usa[i + 1U]);

        p_bytes[off] = (uint8_t)(saved & 0xffU);
        p_bytes[off + 1U] = (uint8_t)((saved >> 8) & 0xffU);
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_pre_write_mst_fixup(void *p_buf, uint32_t size)
{
    if (NULL == p_buf || 0U == size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    ntfs_record_t *p_rec = (ntfs_record_t *)p_buf;
    if (NTFS_MAGIC_BAAD == p_rec->magic || NTFS_MAGIC_CHKD == p_rec->magic)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    uint8_t *p_bytes = (uint8_t *)p_buf;
    uint16_t usa_ofs = pfs_le16_to_cpu(p_rec->usa_ofs);
    uint16_t usa_count = pfs_le16_to_cpu(p_rec->usa_count);
    if (0U == usa_count)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    usa_count = (uint16_t)(usa_count - 1U);
    if (0U != (size & (PFS_NTFS_MST_BLOCK_SIZE - 1U)) || 0U != (usa_ofs & 1U) ||
            (uint32_t)usa_ofs + (uint32_t)usa_count * 2U > size ||
            size / PFS_NTFS_MST_BLOCK_SIZE != (uint32_t)usa_count)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    uint16_t *p_usa = (uint16_t *)(p_bytes + usa_ofs);
    uint16_t usn = pfs_le16_to_cpu(p_usa[0]) + 1U;
    if (0U == usn || 0xffffU == usn)
    {
        usn = 1;
    }
    p_usa[0] = pfs_cpu_to_le16(usn);

    for (uint32_t i = 0; i < (uint32_t)usa_count; i++)
    {
        uint32_t off = (i + 1U) * PFS_NTFS_MST_BLOCK_SIZE - 2U;

        p_usa[i + 1U] = pfs_read_le16(p_bytes + off);
        p_bytes[off] = (uint8_t)(usn & 0xffU);
        p_bytes[off + 1U] = (uint8_t)((usn >> 8) & 0xffU);
    }
    return PFS_NTFS_RET_OK;
}

void pfs_ntfs_post_write_mst_fixup(void *p_buf)
{
    if (NULL == p_buf)
    {
        return;
    }
    ntfs_record_t *p_rec = (ntfs_record_t *)p_buf;
    uint8_t *p_bytes = (uint8_t *)p_buf;
    uint16_t usa_ofs = pfs_le16_to_cpu(p_rec->usa_ofs);
    uint16_t usa_count = pfs_le16_to_cpu(p_rec->usa_count);
    if (0U == usa_count)
    {
        return;
    }
    usa_count = (uint16_t)(usa_count - 1U);
    uint16_t *p_usa = (uint16_t *)(p_bytes + usa_ofs);

    for (uint32_t i = 0; i < (uint32_t)usa_count; i++)
    {
        uint32_t off = (i + 1U) * PFS_NTFS_MST_BLOCK_SIZE - 2U;
        uint16_t saved = pfs_le16_to_cpu(p_usa[i + 1U]);

        p_bytes[off] = (uint8_t)(saved & 0xffU);
        p_bytes[off + 1U] = (uint8_t)((saved >> 8) & 0xffU);
    }
}

void pfs_ntfs_mft_reset(pfs_ntfs_mft_t *p_mft)
{
    if (NULL == p_mft)
    {
        return;
    }
    pfs_ntfs_runlist_reset(&p_mft->runlist);
    memset(p_mft, 0, sizeof(*p_mft));
    p_mft->runlist.hint = -1;
}

void pfs_ntfs_mft_rec_reset(pfs_ntfs_mft_rec_t *p_rec)
{
    if (NULL == p_rec)
    {
        return;
    }
    if (NULL != p_rec->p_mrec)
    {
        pfs_free(p_rec->p_mrec);
        p_rec->p_mrec = NULL;
    }
    memset(p_rec, 0, sizeof(*p_rec));
}

void pfs_ntfs_index_reset(pfs_ntfs_index_t *p_idx)
{
    if (NULL == p_idx)
    {
        return;
    }
    // 仅 valid 时释放; 未初始化栈对象勿 free 野指针
    if (p_idx->valid)
    {
        if (NULL != p_idx->p_root)
        {
            pfs_free(p_idx->p_root);
            p_idx->p_root = NULL;
        }
        pfs_ntfs_runlist_reset(&p_idx->alloc_runlist);
    }
    memset(p_idx, 0, sizeof(*p_idx));
    p_idx->alloc_runlist.hint = -1;
}

void pfs_ntfs_inode_reset(pfs_ntfs_inode_t *p_ino)
{
    if (NULL == p_ino)
    {
        return;
    }
    // 仅 valid 时释放; 栈上未 memset 的 inode 禁止按野指针 free
    if (p_ino->valid)
    {
        if (NULL != p_ino->p_data_resident)
        {
            pfs_free(p_ino->p_data_resident);
            p_ino->p_data_resident = NULL;
        }
        pfs_ntfs_runlist_reset(&p_ino->data_runlist);
        pfs_ntfs_index_reset(&p_ino->i30);
    }
    memset(p_ino, 0, sizeof(*p_ino));
    p_ino->data_runlist.hint = -1;
    p_ino->i30.alloc_runlist.hint = -1;
}

static int pfs_ntfs_mft_record_check(const ntfs_mft_record_t *p_mrec, uint32_t mft_record_size)
{
    if (NULL == p_mrec)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (NTFS_MAGIC_FILE != p_mrec->magic)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint32_t bytes_alloc = pfs_le32_to_cpu(p_mrec->bytes_allocated);
    if (bytes_alloc != mft_record_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint32_t bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    if (bytes_in_use < sizeof(ntfs_mft_record_t) || bytes_in_use > mft_record_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint16_t attrs_ofs = pfs_le16_to_cpu(p_mrec->attrs_offset);
    if (attrs_ofs < sizeof(ntfs_mft_record_t) || 0U != (attrs_ofs & 7U) || attrs_ofs >= bytes_in_use)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint16_t usa_ofs = pfs_le16_to_cpu(p_mrec->usa_ofs);
    uint16_t usa_count = pfs_le16_to_cpu(p_mrec->usa_count);
    if (usa_count != (uint16_t)(mft_record_size / PFS_NTFS_MST_BLOCK_SIZE + 1U))
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    if (usa_ofs >= attrs_ofs)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mft_load_bootstrap(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi || NULL == p_sbi->p_blkio)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t rec_size = p_bs->mft_record_size;
    if (0U == rec_size || rec_size > 65536U)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    pfs_ntfs_mft_reset(&p_sbi->mft);

    uint8_t *p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int64_t off = p_sbi->part_offset + (int64_t)p_bs->mft_lcn * (int64_t)p_bs->cluster_size;
    int rc = pfs_blkio_pread(p_sbi->p_blkio, p_rec_buf, (int)rec_size, off);
    if (0 > rc)
    {
        rc = pfs_ntfs_map_blkio_rc(rc);
        goto out;
    }
    else if (rc != (int)rec_size)
    {
        rc = PFS_NTFS_RET_EIO;
        goto out;
    }

    rc = pfs_ntfs_post_read_mst_fixup(p_rec_buf, rec_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    ntfs_mft_record_t *p_mrec = (ntfs_mft_record_t *)p_rec_buf;
    rc = pfs_ntfs_mft_record_check(p_mrec, rec_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    ntfs_attr_record_t *p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_DATA);
    if (NULL == p_attr)
    {
        rc = PFS_NTFS_RET_EBADMSG;
        goto out;
    }
    if (0U == p_attr->non_resident)
    {
        rc = PFS_NTFS_RET_EBADMSG;
        goto out;
    }
    if (0ULL != pfs_le64_to_cpu(p_attr->data.non_resident.lowest_vcn))
    {
        // 首 extent 须自 VCN0; 跨记录 attrlist 暂不支持
        rc = PFS_NTFS_RET_EOPNOTSUPP;
        goto out;
    }
    uint16_t flags = pfs_le16_to_cpu(p_attr->flags);
    if (0U != (flags & (0x0001U | 0x4000U | 0x8000U)))
    {
        // COMPRESSED / ENCRYPTED / SPARSE: $MFT/$DATA 不允许
        rc = PFS_NTFS_RET_EBADMSG;
        goto out;
    }

    rc = pfs_ntfs_runlist_decompress(&p_sbi->mft.runlist, p_attr);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    p_sbi->mft.mft_no = 0;
    p_sbi->mft.seq_no = pfs_le16_to_cpu(p_mrec->sequence_number);
    p_sbi->mft.data_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.data_size);
    p_sbi->mft.initialized_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.initialized_size);
    p_sbi->mft.allocated_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.allocated_size);
    p_sbi->mft.valid = true;
    rc = PFS_NTFS_RET_OK;

out:
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_mft_reset(&p_sbi->mft);
    }
    pfs_free(p_rec_buf);
    return rc;
}

int pfs_ntfs_mft_read_record(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_buf)
{
    if (NULL == p_sbi || NULL == p_buf || !p_sbi->mft.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t rec_size = p_bs->mft_record_size;
    if (0U == rec_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int64_t pos = (int64_t)mft_no * (int64_t)rec_size;
    int got;
    int rc = pfs_ntfs_rl_pread(p_sbi, &p_sbi->mft.runlist, p_sbi->mft.data_size, pos, p_buf,
            (int)rec_size, &got);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (got != (int)rec_size)
    {
        return PFS_NTFS_RET_EIO;
    }

    rc = pfs_ntfs_post_read_mst_fixup(p_buf, rec_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    {
        ntfs_mft_record_t *p_mrec = (ntfs_mft_record_t *)p_buf;
        uint16_t flags;

        rc = pfs_ntfs_mft_record_check(p_mrec, rec_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        flags = pfs_le16_to_cpu(p_mrec->flags);
        if (0U == (flags & pfs_le16_to_cpu(NTFS_MFT_RECORD_IN_USE)))
        {
            return PFS_NTFS_RET_ENOENT;
        }
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_inode_load_data(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        pfs_ntfs_inode_t *p_ino)
{
    ntfs_attr_record_t *p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_DATA);
    if (NULL == p_attr)
    {
        p_ino->data_size = 0;
        p_ino->initialized_size = 0;
        p_ino->allocated_size = 0;
        return PFS_NTFS_RET_OK;
    }

    if (0U == p_attr->non_resident)
    {
        uint32_t val_len;
        uint16_t val_ofs;
        uint8_t *p_val;

        val_len = pfs_le32_to_cpu(p_attr->data.resident.value_length);
        val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
        if ((uint32_t)val_ofs + val_len > pfs_le32_to_cpu(p_attr->length))
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        p_val = (uint8_t *)p_attr + val_ofs;
        // 空常驻 $DATA 亦标 resident, 否则写路径会误走非常驻
        p_ino->data_resident = true;
        p_ino->data_resident_size = val_len;
        if (val_len > 0U)
        {
            p_ino->p_data_resident = (uint8_t *)pfs_malloc(val_len);
            if (NULL == p_ino->p_data_resident)
            {
                return PFS_NTFS_RET_ENOMEM;
            }
            memcpy(p_ino->p_data_resident, p_val, val_len);
        }
        else
        {
            p_ino->p_data_resident = NULL;
        }
        p_ino->data_size = (int64_t)val_len;
        p_ino->initialized_size = (int64_t)val_len;
        p_ino->allocated_size = (int64_t)val_len;
        return PFS_NTFS_RET_OK;
    }

    {
        uint16_t flags = pfs_le16_to_cpu(p_attr->flags);
        if (0U != (flags & (pfs_le16_to_cpu(NTFS_ATTR_IS_COMPRESSED) |
                pfs_le16_to_cpu(NTFS_ATTR_IS_ENCRYPTED))))
        {
            return PFS_NTFS_RET_EOPNOTSUPP;
        }
        if (0ULL != pfs_le64_to_cpu(p_attr->data.non_resident.lowest_vcn))
        {
            return PFS_NTFS_RET_EOPNOTSUPP;
        }

        {
            int rc = pfs_ntfs_runlist_decompress(&p_ino->data_runlist, p_attr);

            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
        }

        p_ino->data_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.data_size);
        p_ino->initialized_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.initialized_size);
        p_ino->allocated_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.allocated_size);
        return PFS_NTFS_RET_OK;
    }
}

static int pfs_ntfs_inode_load_i30(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        pfs_ntfs_inode_t *p_ino)
{
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    ntfs_attr_record_t *p_attr = pfs_ntfs_attr_find(p_mrec, NTFS_AT_INDEX_ROOT, g_pfs_ntfs_attr_name_i30,
            PFS_NTFS_ATTR_NAME_I30_LEN);
    if (NULL == p_attr)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    if (0U != p_attr->non_resident)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    uint32_t val_len = pfs_le32_to_cpu(p_attr->data.resident.value_length);
    uint16_t val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
    if ((uint32_t)val_ofs + val_len < sizeof(ntfs_index_root_t))
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint8_t *p_val = (uint8_t *)p_attr + val_ofs;
    p_ino->i30.p_root = (uint8_t *)pfs_malloc(val_len);
    if (NULL == p_ino->i30.p_root)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    memcpy(p_ino->i30.p_root, p_val, val_len);
    p_ino->i30.root_size = val_len;

    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_ino->i30.p_root;
    p_ino->i30.collation_rule = pfs_le32_to_cpu(p_root->collation_rule);
    p_ino->i30.block_size = pfs_le32_to_cpu(p_root->index_block_size);
    if (0U == p_ino->i30.block_size)
    {
        p_ino->i30.block_size = p_bs->index_record_size;
    }

    if (p_bs->cluster_size <= p_ino->i30.block_size)
    {
        p_ino->i30.vcn_size = p_bs->cluster_size;
    }
    else
    {
        p_ino->i30.vcn_size = p_bs->sector_size;
    }

    if (0U != (p_root->index.flags & (uint8_t)NTFS_LARGE_INDEX))
    {
        p_ino->i30.has_allocation = true;
        p_attr = pfs_ntfs_attr_find(p_mrec, NTFS_AT_INDEX_ALLOCATION, g_pfs_ntfs_attr_name_i30,
                PFS_NTFS_ATTR_NAME_I30_LEN);
        if (NULL == p_attr)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        if (0U != p_attr->non_resident)
        {
            if (0ULL != pfs_le64_to_cpu(p_attr->data.non_resident.lowest_vcn))
            {
                return PFS_NTFS_RET_EOPNOTSUPP;
            }
            int rc = pfs_ntfs_runlist_decompress(&p_ino->i30.alloc_runlist, p_attr);
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            return PFS_NTFS_RET_EBADMSG;
        }
    }

    p_ino->i30.valid = true;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_inode_load(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, pfs_ntfs_inode_t *p_ino)
{
    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_sbi->mft.valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    pfs_ntfs_inode_reset(p_ino);
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t rec_size = p_bs->mft_record_size;
    uint8_t *p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int rc = pfs_ntfs_mft_read_record(p_sbi, mft_no, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    ntfs_mft_record_t *p_mrec = (ntfs_mft_record_t *)p_rec_buf;
    uint16_t flags = pfs_le16_to_cpu(p_mrec->flags);
    p_ino->mft_no = mft_no;
    p_ino->seq_no = pfs_le16_to_cpu(p_mrec->sequence_number);
    p_ino->is_dir = (0U != (flags & pfs_le16_to_cpu(NTFS_MFT_RECORD_IS_DIRECTORY))) ? true : false;

    if (p_ino->is_dir)
    {
        rc = pfs_ntfs_inode_load_i30(p_sbi, p_mrec, p_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
    }
    else
    {
        rc = pfs_ntfs_inode_load_data(p_sbi, p_mrec, p_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
    }

    p_ino->valid = true;
    rc = PFS_NTFS_RET_OK;

out:
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_inode_reset(p_ino);
    }
    pfs_free(p_rec_buf);
    return rc;
}

int pfs_ntfs_rl_pread(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl, int64_t data_size,
        int64_t pos, void *buf, int len, int *p_got)
{
    if (NULL == p_sbi || NULL == p_rl || NULL == buf || NULL == p_got || len <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (pos < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    *p_got = 0;
    if (pos >= data_size)
    {
        return PFS_NTFS_RET_OK;
    }

    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t cluster_size = p_bs->cluster_size;
    if (0U == cluster_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int64_t end = pos + (int64_t)len;
    if (end > data_size)
    {
        end = data_size;
    }
    int64_t valid_end = data_size;

    uint8_t *dst = (uint8_t *)buf;
    int64_t cur = pos;
    int remain = (int)(end - pos);
    int got = 0;

    while (remain > 0)
    {
        uint32_t clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)((uint64_t)cur & (uint64_t)p_bs->cluster_size_mask);
        int64_t vcn = (int64_t)clu_off;
        int64_t lcn = pfs_ntfs_runlist_vcn_to_lcn(p_rl, vcn);
        int chunk;

        if (PFS_NTFS_LCN_HOLE == lcn)
        {
            chunk = (int)(cluster_size - in_clu);
            if (chunk > remain)
            {
                chunk = remain;
            }
            memset(dst + got, 0, (size_t)chunk);
            got += chunk;
            cur += chunk;
            remain -= chunk;
            continue;
        }
        if (lcn < 0)
        {
            if (PFS_NTFS_LCN_ENOENT == lcn)
            {
                chunk = remain;
                memset(dst + got, 0, (size_t)chunk);
                got += chunk;
                return PFS_NTFS_RET_OK;
            }
            return PFS_NTFS_RET_EIO;
        }

        chunk = (int)(cluster_size - in_clu);
        if (chunk > remain)
        {
            chunk = remain;
        }

        {
            int64_t byte_off = p_sbi->part_offset + lcn * (int64_t)cluster_size + (int64_t)in_clu;
            pfs_cache_cluster_slot_t *p_slot;
            uint32_t clu_id = (uint32_t)lcn;
            int64_t clu_base = p_sbi->part_offset + lcn * (int64_t)cluster_size;

            if (in_clu + (uint32_t)chunk <= cluster_size)
            {
                p_slot = pfs_cache_cluster_get(&p_sbi->cache, clu_id, clu_base);
                if (NULL != p_slot && NULL != p_slot->p_data)
                {
                    memcpy(dst + got, p_slot->p_data + in_clu, (size_t)chunk);
                    got += chunk;
                    cur += chunk;
                    remain -= chunk;
                    continue;
                }
            }

            int rc = pfs_blkio_pread(p_sbi->p_blkio, dst + got, chunk, byte_off);
            if (0 > rc)
            {
                return pfs_ntfs_map_blkio_rc(rc);
            }
            else if (rc != chunk)
            {
                return PFS_NTFS_RET_EIO;
            }
        }

        got += chunk;
        cur += chunk;
        remain -= chunk;
    }

    *p_got = got;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_rl_pwrite(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl, int64_t data_size,
        int64_t pos, const void *buf, int len, int *p_put)
{
    if (NULL == p_sbi || NULL == p_rl || NULL == buf || NULL == p_put || len <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (pos < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    *p_put = 0;
    if (pos >= data_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t cluster_size = p_bs->cluster_size;
    if (0U == cluster_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int64_t end = pos + (int64_t)len;
    if (end > data_size)
    {
        end = data_size;
    }

    const uint8_t *src = (const uint8_t *)buf;
    int64_t cur = pos;
    int remain = (int)(end - pos);
    int put = 0;

    while (remain > 0)
    {
        uint32_t clu_off = (uint32_t)((uint64_t)cur >> p_bs->cluster_size_bits);
        uint32_t in_clu = (uint32_t)((uint64_t)cur & (uint64_t)p_bs->cluster_size_mask);
        int64_t vcn = (int64_t)clu_off;
        int64_t lcn = pfs_ntfs_runlist_vcn_to_lcn(p_rl, vcn);
        int chunk;

        if (PFS_NTFS_LCN_HOLE == lcn || lcn < 0)
        {
            return PFS_NTFS_RET_EINVAL;
        }

        chunk = (int)(cluster_size - in_clu);
        if (chunk > remain)
        {
            chunk = remain;
        }

        {
            int64_t byte_off = p_sbi->part_offset + lcn * (int64_t)cluster_size + (int64_t)in_clu;

            int rc = pfs_blkio_pwrite(p_sbi->p_blkio, src + put, chunk, byte_off);
            if (0 > rc)
            {
                return pfs_ntfs_map_blkio_rc(rc);
            }
            else if (rc != chunk)
            {
                return PFS_NTFS_RET_EIO;
            }
            pfs_cache_cluster_invalidate(&p_sbi->cache, (uint32_t)lcn);
        }

        put += chunk;
        cur += chunk;
        remain -= chunk;
    }

    *p_put = put;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mft_write_record(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_buf)
{
    if (NULL == p_sbi || NULL == p_buf || !p_sbi->mft.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t rec_size = p_bs->mft_record_size;
    if (0U == rec_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int rc = pfs_ntfs_pre_write_mst_fixup(p_buf, rec_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    int64_t pos = (int64_t)mft_no * (int64_t)rec_size;
    int put;
    rc = pfs_ntfs_rl_pwrite(p_sbi, &p_sbi->mft.runlist, p_sbi->mft.data_size, pos, p_buf,
            (int)rec_size, &put);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_post_write_mst_fixup(p_buf);
        return rc;
    }
    if (put != (int)rec_size)
    {
        pfs_ntfs_post_write_mst_fixup(p_buf);
        return PFS_NTFS_RET_EIO;
    }

    pfs_ntfs_post_write_mst_fixup(p_buf);

    {
        int64_t vcn = pos >> p_bs->cluster_size_bits;
        int64_t lcn = pfs_ntfs_runlist_vcn_to_lcn(&p_sbi->mft.runlist, vcn);

        if (lcn >= 0)
        {
            pfs_cache_cluster_invalidate(&p_sbi->cache, (uint32_t)lcn);
        }
    }
    return PFS_NTFS_RET_OK;
}

// end
