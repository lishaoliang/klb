// Doc Encode : UTF-8 BOM, Unix(LF)
// 簇位图 / 分配 (linux-7.1 fs/ntfs/bitmap.c + lcnalloc.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_ntfs_lcn_bitmap_reset(pfs_ntfs_lcn_bitmap_t *p_bmp)
{
    if (NULL == p_bmp)
    {
        return;
    }
    pfs_ntfs_runlist_reset(&p_bmp->runlist);
    (void)pfs_cache_cluster_deinit(&p_bmp->bmp_cache);
    memset(p_bmp, 0, sizeof(*p_bmp));
    p_bmp->runlist.hint = -1;
    p_bmp->free_clusters = -1;
    p_bmp->hint_lcn = -1;
    p_bmp->mft_zone_start = -1;
    p_bmp->mft_zone_end = -1;
    p_bmp->mft_zone_pos = -1;
    p_bmp->data_zone_pos = -1;
}

void pfs_ntfs_mft_bitmap_reset(pfs_ntfs_mft_bitmap_t *p_bmp)
{
    if (NULL == p_bmp)
    {
        return;
    }
    if (NULL != p_bmp->p_resident)
    {
        pfs_free(p_bmp->p_resident);
        p_bmp->p_resident = NULL;
    }
    pfs_ntfs_runlist_reset(&p_bmp->runlist);
    memset(p_bmp, 0, sizeof(*p_bmp));
    p_bmp->runlist.hint = -1;
    p_bmp->free_mft_records = -1;
}

int pfs_ntfs_lcn_bitmap_load(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi || !p_sbi->mft.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_lcn_bitmap_reset(&p_sbi->lcn_bmp);
    pfs_ntfs_lcn_bitmap_t *p_bmp = &p_sbi->lcn_bmp;
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;

    pfs_ntfs_inode_t ino;
    memset(&ino, 0, sizeof(ino));
    int rc = pfs_ntfs_inode_load(p_sbi, NTFS_FILE_Bitmap, &ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (ino.data_resident || 0U == ino.data_runlist.count)
    {
        pfs_ntfs_inode_reset(&ino);
        return PFS_NTFS_RET_EBADMSG;
    }

    p_bmp->valid = true;
    p_bmp->mft_no = NTFS_FILE_Bitmap;
    p_bmp->seq_no = ino.seq_no;
    p_bmp->data_size = ino.data_size;
    p_bmp->initialized_size = ino.initialized_size;
    p_bmp->allocated_size = ino.allocated_size;
    p_bmp->runlist = ino.data_runlist;
    ino.data_runlist.p_rl = NULL;
    ino.data_runlist.count = 0;

    rc = pfs_cache_cluster_init(&p_bmp->bmp_cache, p_sbi->p_blkio, p_bs->cluster_size);
    if (0 != rc)
    {
        pfs_ntfs_lcn_bitmap_reset(p_bmp);
        pfs_ntfs_inode_reset(&ino);
        if (-12 == rc)
        {
            return PFS_NTFS_RET_ENOMEM;
        }
        return PFS_NTFS_RET_EIO;
    }

    p_bmp->mft_zone_start = (int64_t)p_bs->mft_lcn;
    p_bmp->mft_zone_end = (int64_t)(p_bs->num_clusters / 8U);
    if (p_bmp->mft_zone_end <= p_bmp->mft_zone_start)
    {
        p_bmp->mft_zone_end = p_bmp->mft_zone_start + 1;
    }
    p_bmp->mft_zone_pos = p_bmp->mft_zone_start;
    p_bmp->data_zone_pos = p_bmp->mft_zone_end;
    p_bmp->hint_lcn = p_bmp->data_zone_pos;
    p_bmp->free_clusters = -1;

    pfs_ntfs_inode_reset(&ino);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_bitmap_rw_byte(pfs_ntfs_sbi_t *p_sbi, int64_t byte_off, uint8_t *p_byte, int write)
{
    pfs_ntfs_lcn_bitmap_t *p_bmp;
    pfs_ntfs_bs_t *p_bs;
    uint32_t cluster_size;
    int64_t vcn;
    int64_t lcn;

    p_bmp = &p_sbi->lcn_bmp;
    p_bs = &p_sbi->bs;
    cluster_size = p_bs->cluster_size;
    vcn = byte_off / (int64_t)cluster_size;
    lcn = pfs_ntfs_runlist_vcn_to_lcn(&p_bmp->runlist, vcn);
    if (lcn < 0)
    {
        return PFS_NTFS_RET_EIO;
    }

    {
        int64_t clu_base = p_sbi->part_offset + lcn * (int64_t)cluster_size;
        uint32_t in_clu = (uint32_t)(byte_off % (int64_t)cluster_size);
        pfs_cache_cluster_slot_t *p_slot;

        p_slot = pfs_cache_cluster_get(&p_bmp->bmp_cache, (uint32_t)lcn, clu_base);
        if (NULL == p_slot || NULL == p_slot->p_data)
        {
            return PFS_NTFS_RET_EIO;
        }
        if (write)
        {
            p_slot->p_data[in_clu] = *p_byte;
            int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_slot->p_data, (int)cluster_size, clu_base);
            if (rc < 0 || rc != (int)cluster_size)
            {
                return PFS_NTFS_RET_EIO;
            }
            pfs_cache_cluster_invalidate(&p_bmp->bmp_cache, (uint32_t)lcn);
            pfs_cache_cluster_invalidate(&p_sbi->cache, (uint32_t)lcn);
            return PFS_NTFS_RET_OK;
        }
        *p_byte = p_slot->p_data[in_clu];
        return PFS_NTFS_RET_OK;
    }
}

int pfs_ntfs_bitmap_set_run(pfs_ntfs_sbi_t *p_sbi, int64_t lcn, int64_t count, int set)
{
    pfs_ntfs_lcn_bitmap_t *p_bmp;
    int64_t end_lcn;
    int64_t cur;

    if (NULL == p_sbi || lcn < 0 || count <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    p_bmp = &p_sbi->lcn_bmp;
    if (!p_bmp->valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    end_lcn = lcn + count;
    if ((uint64_t)end_lcn > p_sbi->bs.num_clusters)
    {
        return PFS_NTFS_RET_ERANGE;
    }

    for (cur = lcn; cur < end_lcn; cur++)
    {
        int64_t byte_off = cur >> 3;
        uint8_t bit = (uint8_t)(1U << (cur & 7));
        uint8_t b;

        int rc = pfs_ntfs_bitmap_rw_byte(p_sbi, byte_off, &b, 0);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        if (set)
        {
            b = (uint8_t)(b | bit);
        }
        else
        {
            b = (uint8_t)(b & (uint8_t)(~bit));
        }
        rc = pfs_ntfs_bitmap_rw_byte(p_sbi, byte_off, &b, 1);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }

    if (p_bmp->free_clusters >= 0)
    {
        if (set)
        {
            p_bmp->free_clusters -= count;
        }
        else
        {
            p_bmp->free_clusters += count;
        }
    }
    p_bmp->hint_lcn = lcn + count;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_bitmap_test_bit(pfs_ntfs_sbi_t *p_sbi, int64_t lcn, int *p_used)
{
    int64_t byte_off = lcn >> 3;
    uint8_t bit = (uint8_t)(1U << (lcn & 7));
    uint8_t b;

    int rc = pfs_ntfs_bitmap_rw_byte(p_sbi, byte_off, &b, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    *p_used = (0U != (b & bit)) ? 1 : 0;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_cluster_alloc(pfs_ntfs_sbi_t *p_sbi, int64_t count, pfs_ntfs_alloc_zone_e zone,
        pfs_ntfs_runlist_t *p_out_rl)
{
    pfs_ntfs_lcn_bitmap_t *p_bmp;
    pfs_ntfs_bs_t *p_bs;
    int64_t start_lcn;
    int64_t end_lcn;
    int64_t lcn;
    int64_t run_start;
    int64_t run_len;
    int64_t wrap_start;
    pfs_ntfs_run_t *p_rl;
    int used;

    if (NULL == p_sbi || NULL == p_out_rl || count <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    p_bmp = &p_sbi->lcn_bmp;
    if (!p_bmp->valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    int rc;
    pfs_ntfs_runlist_reset(p_out_rl);
    p_bs = &p_sbi->bs;
    end_lcn = (int64_t)p_bs->num_clusters;

    if (PFS_NTFS_ZONE_MFT == zone)
    {
        start_lcn = p_bmp->mft_zone_pos;
        if (start_lcn < p_bmp->mft_zone_start)
        {
            start_lcn = p_bmp->mft_zone_start;
        }
        wrap_start = p_bmp->mft_zone_start;
    }
    else
    {
        start_lcn = p_bmp->data_zone_pos;
        if (p_bmp->hint_lcn >= 0)
        {
            start_lcn = p_bmp->hint_lcn;
        }
        if (start_lcn < p_bmp->data_zone_pos)
        {
            start_lcn = p_bmp->data_zone_pos;
        }
        wrap_start = p_bmp->data_zone_pos;
    }

    run_start = -1;
    run_len = 0;
    lcn = start_lcn;

    for (;;)
    {
        if (lcn >= end_lcn)
        {
            if (PFS_NTFS_ZONE_MFT == zone && lcn >= p_bmp->mft_zone_end)
            {
                break;
            }
            if (PFS_NTFS_ZONE_DATA == zone)
            {
                lcn = wrap_start;
                if (lcn >= start_lcn)
                {
                    break;
                }
                continue;
            }
            break;
        }
        if (PFS_NTFS_ZONE_MFT == zone && lcn >= p_bmp->mft_zone_end)
        {
            break;
        }

        rc = pfs_ntfs_bitmap_test_bit(p_sbi, lcn, &used);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }

        if (!used)
        {
            if (run_start < 0)
            {
                run_start = lcn;
                run_len = 1;
            }
            else
            {
                run_len++;
            }
            if (run_len >= count)
            {
                break;
            }
        }
        else
        {
            run_start = -1;
            run_len = 0;
        }

        lcn++;
        if (PFS_NTFS_ZONE_DATA == zone && lcn >= end_lcn)
        {
            lcn = wrap_start;
            if (lcn >= start_lcn)
            {
                break;
            }
        }
    }

    if (run_len < count || run_start < 0)
    {
        return PFS_NTFS_RET_ENOSPC;
    }

    rc = pfs_ntfs_bitmap_set_run(p_sbi, run_start, count, 1);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    p_rl = (pfs_ntfs_run_t *)pfs_malloc(2U * sizeof(pfs_ntfs_run_t));
    if (NULL == p_rl)
    {
        (void)pfs_ntfs_bitmap_set_run(p_sbi, run_start, count, 0);
        return PFS_NTFS_RET_ENOMEM;
    }

    p_rl[0].vcn = 0;
    p_rl[0].lcn = run_start;
    p_rl[0].length = count;
    p_rl[1].vcn = count;
    p_rl[1].lcn = PFS_NTFS_LCN_ENOENT;
    p_rl[1].length = 0;

    p_out_rl->p_rl = p_rl;
    p_out_rl->count = 1;
    p_out_rl->hint = -1;

    if (PFS_NTFS_ZONE_MFT == zone)
    {
        p_bmp->mft_zone_pos = run_start + count;
    }
    else
    {
        p_bmp->data_zone_pos = run_start + count;
        p_bmp->hint_lcn = run_start + count;
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_cluster_free_from_rl(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_runlist_t *p_rl,
        int64_t start_vcn, int64_t count)
{
    const pfs_ntfs_run_t *p_runs;
    int64_t end_vcn;

    if (NULL == p_sbi || NULL == p_rl || start_vcn < 0 || count <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    if (!p_sbi->lcn_bmp.valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }
    if (NULL == p_rl->p_rl || 0U == p_rl->count)
    {
        return PFS_NTFS_RET_OK;
    }

    end_vcn = start_vcn + count;
    p_runs = p_rl->p_rl;
    for (int i = 0; p_runs[i].length != 0; i++)
    {
        int64_t run_vcn_end = p_runs[i].vcn + p_runs[i].length;

        if (run_vcn_end <= start_vcn)
        {
            continue;
        }
        if (p_runs[i].vcn >= end_vcn)
        {
            break;
        }
        if (p_runs[i].lcn < 0)
        {
            continue;
        }

        {
            int64_t free_vcn = p_runs[i].vcn;
            int64_t free_len = p_runs[i].length;

            if (free_vcn < start_vcn)
            {
                free_len -= start_vcn - free_vcn;
                free_vcn = start_vcn;
            }
            if (free_vcn + free_len > end_vcn)
            {
                free_len = end_vcn - free_vcn;
            }
            if (free_len <= 0)
            {
                continue;
            }

            int rc = pfs_ntfs_bitmap_set_run(p_sbi, p_runs[i].lcn + (free_vcn - p_runs[i].vcn),
                    free_len, 0);
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    return PFS_NTFS_RET_OK;
}

#define PFS_NTFS_INDEX_BLOCK_SIZE       4096u

static uint32_t pfs_ntfs_attr_align8(uint32_t v)
{
    return (v + 7U) & ~7U;
}

static int pfs_ntfs_mft_bitmap_rw_byte(pfs_ntfs_sbi_t *p_sbi, int64_t byte_off, uint8_t *p_byte,
        int write)
{
    pfs_ntfs_mft_bitmap_t *p_bmp;
    pfs_ntfs_bs_t *p_bs;
    pfs_cache_cluster_slot_t *p_slot;
    int64_t data_size;
    int64_t vcn;
    int64_t lcn;
    int64_t clu_base;
    uint32_t cluster_size;
    uint32_t in_clu;

    p_bmp = &p_sbi->mft_bmp;
    data_size = p_bmp->data_size;
    if (byte_off < 0 || byte_off >= data_size)
    {
        return PFS_NTFS_RET_ERANGE;
    }

    if (p_bmp->is_resident)
    {
        if (NULL == p_bmp->p_resident || byte_off >= (int64_t)p_bmp->resident_size)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        if (write)
        {
            p_bmp->p_resident[byte_off] = *p_byte;
        }
        else
        {
            *p_byte = p_bmp->p_resident[byte_off];
        }
        return PFS_NTFS_RET_OK;
    }

    // 非常驻: 整簇 RMW (对齐卷 $Bitmap); 单字节 pwrite 易使多 bit 同字节更新丢失, chkdsk 报 MFT BITMAP incorrect
    p_bs = &p_sbi->bs;
    cluster_size = p_bs->cluster_size;
    if (0U == cluster_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    vcn = byte_off / (int64_t)cluster_size;
    in_clu = (uint32_t)(byte_off % (int64_t)cluster_size);
    lcn = pfs_ntfs_runlist_vcn_to_lcn(&p_bmp->runlist, vcn);
    if (lcn < 0)
    {
        return PFS_NTFS_RET_EIO;
    }
    clu_base = p_sbi->part_offset + lcn * (int64_t)cluster_size;
    p_slot = pfs_cache_cluster_get(&p_sbi->cache, (uint32_t)lcn, clu_base);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_NTFS_RET_EIO;
    }
    if (!write)
    {
        *p_byte = p_slot->p_data[in_clu];
        return PFS_NTFS_RET_OK;
    }
    p_slot->p_data[in_clu] = *p_byte;
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_slot->p_data, (int)cluster_size, clu_base);
    if (rc < 0 || rc != (int)cluster_size)
    {
        pfs_cache_cluster_invalidate(&p_sbi->cache, (uint32_t)lcn);
        return PFS_NTFS_RET_EIO;
    }
    pfs_cache_cluster_invalidate(&p_sbi->cache, (uint32_t)lcn);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mft_bitmap_flush_resident(pfs_ntfs_sbi_t *p_sbi)
{
    pfs_ntfs_mft_bitmap_t *p_bmp;
    uint8_t *p_rec_buf;
    ntfs_mft_record_t *p_mrec;
    ntfs_attr_record_t *p_attr;
    uint32_t rec_size;
    uint32_t val_ofs;
    uint32_t val_len;
    uint32_t attr_len;

    p_bmp = &p_sbi->mft_bmp;
    if (!p_bmp->is_resident || NULL == p_bmp->p_resident)
    {
        return PFS_NTFS_RET_OK;
    }

    rec_size = p_sbi->bs.mft_record_size;
    p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int rc = pfs_ntfs_mft_read_record(p_sbi, NTFS_FILE_MFT, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_rec_buf);
        return rc;
    }

    p_mrec = (ntfs_mft_record_t *)p_rec_buf;
    p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_BITMAP);
    if (NULL == p_attr || 0U != p_attr->non_resident)
    {
        pfs_free(p_rec_buf);
        return PFS_NTFS_RET_EBADMSG;
    }

    val_len = p_bmp->resident_size;
    val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
    attr_len = pfs_ntfs_attr_align8(val_ofs + val_len);
    if (attr_len > rec_size - ((uint8_t *)p_attr - p_rec_buf))
    {
        pfs_free(p_rec_buf);
        return PFS_NTFS_RET_ENOSPC;
    }

    memcpy((uint8_t *)p_attr + val_ofs, p_bmp->p_resident, val_len);
    p_attr->data.resident.value_length = pfs_cpu_to_le32(val_len);
    p_attr->length = pfs_cpu_to_le32(attr_len);

    rc = pfs_ntfs_mft_write_record(p_sbi, NTFS_FILE_MFT, p_rec_buf);
    pfs_free(p_rec_buf);
    return rc;
}

int pfs_ntfs_mft_bitmap_load(pfs_ntfs_sbi_t *p_sbi)
{
    pfs_ntfs_inode_t ino;
    ntfs_attr_record_t *p_attr;
    uint8_t *p_rec_buf = NULL;
    ntfs_mft_record_t *p_mrec;
    uint32_t rec_size;
    pfs_ntfs_mft_bitmap_t *p_bmp;

    if (NULL == p_sbi || !p_sbi->mft.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_mft_bitmap_reset(&p_sbi->mft_bmp);
    p_bmp = &p_sbi->mft_bmp;
    rec_size = p_sbi->bs.mft_record_size;

    p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int rc = pfs_ntfs_mft_read_record(p_sbi, NTFS_FILE_MFT, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    p_mrec = (ntfs_mft_record_t *)p_rec_buf;
    p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_BITMAP);
    if (NULL == p_attr)
    {
        rc = PFS_NTFS_RET_EBADMSG;
        goto out;
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
            rc = PFS_NTFS_RET_EBADMSG;
            goto out;
        }
        p_val = (uint8_t *)p_attr + val_ofs;
        p_bmp->p_resident = (uint8_t *)pfs_malloc(val_len);
        if (NULL == p_bmp->p_resident)
        {
            rc = PFS_NTFS_RET_ENOMEM;
            goto out;
        }
        memcpy(p_bmp->p_resident, p_val, val_len);
        p_bmp->resident_size = val_len;
        p_bmp->is_resident = true;
        p_bmp->data_size = (int64_t)val_len;
    }
    else
    {
        memset(&ino, 0, sizeof(ino));
        rc = pfs_ntfs_inode_load(p_sbi, NTFS_FILE_MFT, &ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
        p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_BITMAP);
        if (NULL == p_attr)
        {
            pfs_ntfs_inode_reset(&ino);
            rc = PFS_NTFS_RET_EBADMSG;
            goto out;
        }
        if (0ULL != pfs_le64_to_cpu(p_attr->data.non_resident.lowest_vcn))
        {
            pfs_ntfs_inode_reset(&ino);
            rc = PFS_NTFS_RET_EOPNOTSUPP;
            goto out;
        }
        rc = pfs_ntfs_runlist_decompress(&p_bmp->runlist, p_attr);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_ntfs_inode_reset(&ino);
            goto out;
        }
        p_bmp->data_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.data_size);
        p_bmp->allocated_size = (int64_t)pfs_le64_to_cpu(p_attr->data.non_resident.allocated_size);
        p_bmp->is_resident = false;
        pfs_ntfs_inode_reset(&ino);
    }

    p_bmp->valid = true;
    p_bmp->hint_mft_no = NTFS_FILE_first_user;
    p_bmp->free_mft_records = -1;
    rc = PFS_NTFS_RET_OK;

out:
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_mft_bitmap_reset(p_bmp);
    }
    pfs_free(p_rec_buf);
    return rc;
}

int pfs_ntfs_mft_bitmap_set_bit(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, int set)
{
    pfs_ntfs_mft_bitmap_t *p_bmp;
    int64_t byte_off;
    uint8_t bit;
    uint8_t b;

    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    // 0..15 须始终占用位图 (layout.h); 禁止清位
    if (!set && mft_no < (uint64_t)NTFS_FILE_first_user)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    p_bmp = &p_sbi->mft_bmp;
    if (!p_bmp->valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    byte_off = (int64_t)(mft_no >> 3);
    bit = (uint8_t)(1U << (mft_no & 7U));
    int rc = pfs_ntfs_mft_bitmap_rw_byte(p_sbi, byte_off, &b, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (set)
    {
        b = (uint8_t)(b | bit);
    }
    else
    {
        b = (uint8_t)(b & (uint8_t)(~bit));
    }
    rc = pfs_ntfs_mft_bitmap_rw_byte(p_sbi, byte_off, &b, 1);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (p_bmp->is_resident)
    {
        rc = pfs_ntfs_mft_bitmap_flush_resident(p_sbi);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    if (p_bmp->free_mft_records >= 0)
    {
        if (set)
        {
            p_bmp->free_mft_records--;
        }
        else
        {
            p_bmp->free_mft_records++;
        }
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_mft_bitmap_test_bit(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, int *p_used)
{
    int64_t byte_off = (int64_t)(mft_no >> 3);
    uint8_t bit = (uint8_t)(1U << (mft_no & 7U));
    uint8_t b;

    int rc = pfs_ntfs_mft_bitmap_rw_byte(p_sbi, byte_off, &b, 0);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    *p_used = (0U != (b & bit)) ? 1 : 0;
    return PFS_NTFS_RET_OK;
}

static void pfs_ntfs_mft_init_usa(uint8_t *p_buf, uint32_t rec_size)
{
    ntfs_mft_record_t *p_hdr = (ntfs_mft_record_t *)p_buf;
    uint16_t *p_usa;
    uint32_t sectors;
    uint16_t usn;

    sectors = rec_size / PFS_NTFS_MST_BLOCK_SIZE;
    p_hdr->usa_ofs = pfs_cpu_to_le16(48);
    p_hdr->usa_count = pfs_cpu_to_le16((uint16_t)(sectors + 1U));
    p_usa = (uint16_t *)(p_buf + 48);
    usn = 1;
    p_usa[0] = pfs_cpu_to_le16(usn);
    for (uint32_t i = 0; i < sectors; i++)
    {
        uint32_t sec_end = (i + 1U) * PFS_NTFS_MST_BLOCK_SIZE;

        p_usa[i + 1U] = pfs_read_le16(p_buf + sec_end - 2U);
        p_buf[sec_end - 2U] = (uint8_t)(usn & 0xffU);
        p_buf[sec_end - 1U] = (uint8_t)((usn >> 8) & 0xffU);
    }
}

int pfs_ntfs_mft_record_alloc(pfs_ntfs_sbi_t *p_sbi, int is_dir, uint64_t *p_mft_no,
        uint16_t *p_seq_no, uint8_t **p_rec_buf)
{
    pfs_ntfs_mft_bitmap_t *p_bmp;
    pfs_ntfs_bs_t *p_bs;
    uint64_t max_mft;
    uint64_t mft_no;
    uint64_t start;
    uint8_t *p_buf;
    ntfs_mft_record_t *p_mrec;
    ntfs_attr_record_t *p_end;
    uint32_t rec_size;
    uint32_t used;
    uint16_t flags;
    int used_bit;

    if (NULL == p_sbi || NULL == p_mft_no || NULL == p_seq_no || NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    p_bmp = &p_sbi->mft_bmp;
    if (!p_bmp->valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    p_bs = &p_sbi->bs;
    rec_size = p_bs->mft_record_size;
    max_mft = (uint64_t)(p_bmp->data_size * 8);
    start = p_bmp->hint_mft_no;
    if (start < NTFS_FILE_first_user)
    {
        start = NTFS_FILE_first_user;
    }

    p_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    mft_no = start;
    for (;;)
    {
        uint16_t old_seq = 1;

        if (mft_no >= max_mft)
        {
            if (start > NTFS_FILE_first_user)
            {
                mft_no = NTFS_FILE_first_user;
                start = NTFS_FILE_first_user;
                continue;
            }
            pfs_free(p_buf);
            return PFS_NTFS_RET_ENOSPC;
        }
        int rc = pfs_ntfs_mft_bitmap_test_bit(p_sbi, mft_no, &used_bit);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_free(p_buf);
            return rc;
        }
        if (used_bit)
        {
            mft_no++;
            if (mft_no >= max_mft && start > NTFS_FILE_first_user)
            {
                mft_no = NTFS_FILE_first_user;
            }
            else if (mft_no >= max_mft)
            {
                pfs_free(p_buf);
                return PFS_NTFS_RET_ENOSPC;
            }
            continue;
        }

        // 位图空闲但记录仍 IN_USE: 位图陈旧, 补位并跳过 (防覆盖仍存活文件)
        rc = pfs_ntfs_mft_read_record(p_sbi, mft_no, p_buf);
        if (PFS_NTFS_RET_OK == rc)
        {
            p_mrec = (ntfs_mft_record_t *)p_buf;
            if (NTFS_MAGIC_FILE == p_mrec->magic)
            {
                flags = pfs_le16_to_cpu(p_mrec->flags);
                if (0U != (flags & (uint16_t)pfs_le16_to_cpu(NTFS_MFT_RECORD_IN_USE)))
                {
                    (void)pfs_ntfs_mft_bitmap_set_bit(p_sbi, mft_no, 1);
                    mft_no++;
                    if (mft_no >= max_mft && start > NTFS_FILE_first_user)
                    {
                        mft_no = NTFS_FILE_first_user;
                    }
                    else if (mft_no >= max_mft)
                    {
                        pfs_free(p_buf);
                        return PFS_NTFS_RET_ENOSPC;
                    }
                    continue;
                }
                old_seq = pfs_le16_to_cpu(p_mrec->sequence_number);
                if (0U == old_seq || 0xffffU == old_seq)
                {
                    old_seq = 1;
                }
                else
                {
                    old_seq = (uint16_t)(old_seq + 1U);
                    if (0U == old_seq || 0xffffU == old_seq)
                    {
                        old_seq = 1;
                    }
                }
            }
        }

        rc = pfs_ntfs_mft_bitmap_set_bit(p_sbi, mft_no, 1);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_free(p_buf);
            return rc;
        }

        p_mrec = (ntfs_mft_record_t *)p_buf;
        memset(p_buf, 0, rec_size);
        p_mrec->magic = NTFS_MAGIC_FILE;
        p_mrec->sequence_number = pfs_cpu_to_le16(old_seq);
        p_mrec->link_count = pfs_cpu_to_le16(1);
        p_mrec->attrs_offset = pfs_cpu_to_le16(56);
        flags = NTFS_MFT_RECORD_IN_USE;
        if (is_dir)
        {
            flags = (uint16_t)(flags | NTFS_MFT_RECORD_IS_DIRECTORY);
        }
        p_mrec->flags = flags;
        p_mrec->bytes_allocated = pfs_cpu_to_le32(rec_size);
        p_mrec->base_mft_record = pfs_cpu_to_le64(0);
        p_mrec->next_attr_instance = pfs_cpu_to_le16(0);
        p_mrec->mft_record_number = pfs_cpu_to_le32((uint32_t)mft_no);

        p_end = (ntfs_attr_record_t *)(p_buf + 56);
        p_end->type = NTFS_AT_END;
        p_end->length = 0;
        used = pfs_ntfs_attr_align8(56U + (uint32_t)sizeof(uint32_t));
        p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
        pfs_ntfs_mft_init_usa(p_buf, rec_size);

        p_bmp->hint_mft_no = mft_no + 1U;
        *p_mft_no = mft_no;
        *p_seq_no = old_seq;
        *p_rec_buf = p_buf;
        return PFS_NTFS_RET_OK;
    }
}

int pfs_ntfs_mft_record_free(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no, uint8_t *p_rec)
{
    ntfs_mft_record_t *p_mrec;
    uint16_t seq;
    uint16_t flags;

    if (NULL == p_sbi || NULL == p_rec)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    p_mrec = (ntfs_mft_record_t *)p_rec;
    seq = pfs_le16_to_cpu(p_mrec->sequence_number);
    seq++;
    if (0U == seq || 0xffffU == seq)
    {
        seq = 1;
    }
    p_mrec->sequence_number = pfs_cpu_to_le16(seq);
    flags = pfs_le16_to_cpu(p_mrec->flags);
    flags = (uint16_t)(flags & ~pfs_le16_to_cpu(NTFS_MFT_RECORD_IN_USE));
    p_mrec->flags = pfs_cpu_to_le16(flags);
    p_mrec->link_count = pfs_cpu_to_le16(0);

    int rc = pfs_ntfs_mft_write_record(p_sbi, mft_no, p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    return pfs_ntfs_mft_bitmap_set_bit(p_sbi, mft_no, 0);
}

// end
