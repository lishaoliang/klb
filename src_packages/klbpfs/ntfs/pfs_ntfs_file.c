// Doc Encode : UTF-8 BOM, Unix(LF)
// 文件读写 (linux-7.1 fs/ntfs/file.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_ntfs_file_meta_reset(pfs_ntfs_file_meta_t *p_meta)
{
    if (NULL == p_meta)
    {
        return;
    }
    if (p_meta->valid)
    {
        pfs_ntfs_inode_reset(&p_meta->ino);
    }
    memset(p_meta, 0, sizeof(*p_meta));
    p_meta->hint_vcn = 0;
    p_meta->hint_lcn = PFS_NTFS_LCN_RL_NOT_MAPPED;
}

int pfs_ntfs_file_open(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_file_meta_t *p_meta)
{
    if (NULL == p_sbi || NULL == path || NULL == p_meta)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_file_meta_reset(p_meta);
    pfs_ntfs_lookup_t lu;
    int rc = pfs_ntfs_lookup_path(p_sbi, path, &lu);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (lu.is_dir)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    rc = pfs_ntfs_inode_load(p_sbi, lu.mft_no, &p_meta->ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    p_meta->valid = true;
    p_meta->parent_valid = false;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_file_read(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t pos,
        void *buf, int len, int *p_got)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_got || len <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_meta->valid || !p_meta->ino.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 > pos)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    *p_got = 0;
    pfs_ntfs_inode_t *p_ino = &p_meta->ino;
    int64_t data_size = p_ino->data_size;
    int64_t init_size = p_ino->initialized_size;
    if (init_size > data_size)
    {
        init_size = data_size;
    }

    if (pos >= data_size)
    {
        return PFS_NTFS_RET_OK;
    }

    if (pos >= init_size)
    {
        int zero_len = len;
        if ((int64_t)zero_len > data_size - pos)
        {
            zero_len = (int)(data_size - pos);
        }
        memset(buf, 0, (size_t)zero_len);
        *p_got = zero_len;
        return PFS_NTFS_RET_OK;
    }

    if (p_ino->data_resident)
    {
        int64_t avail = init_size - pos;
        int copy_len = len;
        int got;
        int zero_len;

        if ((int64_t)copy_len > avail)
        {
            copy_len = (int)avail;
        }
        if (NULL == p_ino->p_data_resident)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        if ((uint32_t)(pos + copy_len) > p_ino->data_resident_size)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        memcpy(buf, p_ino->p_data_resident + pos, (size_t)copy_len);
        got = copy_len;
        if (got < len && pos + got >= init_size)
        {
            zero_len = len - got;
            if (pos + got + zero_len > data_size)
            {
                zero_len = (int)(data_size - (pos + got));
            }
            if (zero_len > 0)
            {
                memset((uint8_t *)buf + got, 0, (size_t)zero_len);
                got += zero_len;
            }
        }
        *p_got = got;
        return PFS_NTFS_RET_OK;
    }

    int got = 0;
    int rc = pfs_ntfs_rl_pread(p_sbi, &p_ino->data_runlist, init_size, pos, buf, len, &got);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (got < len && pos + got > init_size)
    {
        int tail = (int)(pos + got - init_size);
        if (tail > got)
        {
            tail = got;
        }
        if (tail > 0)
        {
            memset((uint8_t *)buf + got - tail, 0, (size_t)tail);
        }
    }
    if (got < len && pos + got < data_size)
    {
        int zero_len = len - got;
        if (pos + got + zero_len > data_size)
        {
            zero_len = (int)(data_size - (pos + got));
        }
        if (zero_len > 0)
        {
            memset((uint8_t *)buf + got, 0, (size_t)zero_len);
            got += zero_len;
        }
    }

    {
        int64_t vcn = pos >> p_sbi->bs.cluster_size_bits;
        int64_t lcn = pfs_ntfs_runlist_vcn_to_lcn(&p_ino->data_runlist, vcn);

        p_meta->hint_vcn = vcn;
        p_meta->hint_lcn = lcn;
    }

    *p_got = got;
    return PFS_NTFS_RET_OK;
}

static int64_t pfs_ntfs_file_cluster_count(pfs_ntfs_sbi_t *p_sbi, int64_t byte_size)
{
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;

    if (byte_size <= 0)
    {
        return 0;
    }
    return (byte_size + (int64_t)p_bs->cluster_size - 1) / (int64_t)p_bs->cluster_size;
}

static int pfs_ntfs_file_extend_allocated(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_ino,
        int64_t need_bytes)
{
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    int64_t need_vcn = pfs_ntfs_file_cluster_count(p_sbi, need_bytes);
    int64_t have_vcn = pfs_ntfs_file_cluster_count(p_sbi, p_ino->allocated_size);
    if (need_vcn <= have_vcn)
    {
        return PFS_NTFS_RET_OK;
    }

    int64_t alloc_count = need_vcn - have_vcn;
    pfs_ntfs_runlist_t new_rl;
    memset(&new_rl, 0, sizeof(new_rl));
    int rc = pfs_ntfs_cluster_alloc(p_sbi, alloc_count, PFS_NTFS_ZONE_DATA, &new_rl);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (have_vcn > 0 && new_rl.count > 0)
    {
        new_rl.p_rl[0].vcn = have_vcn;
        new_rl.p_rl[1].vcn = have_vcn + alloc_count;
    }

    rc = pfs_ntfs_runlist_merge(&p_ino->data_runlist, &new_rl);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_runlist_reset(&new_rl);
        return rc;
    }

    p_ino->allocated_size = need_vcn * (int64_t)p_bs->cluster_size;
    p_ino->runlist_dirty = true;
    p_ino->sizes_dirty = true;
    return PFS_NTFS_RET_OK;
}

// 回写 $FILE_NAME 与父目录 $I30 副本; 须先索引后 MFT, 失败不得留下不一致
static int pfs_ntfs_file_sync_filename_dups(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        const pfs_ntfs_inode_t *p_ino)
{
    uint8_t *p_base = (uint8_t *)p_mrec;
    uint32_t off = pfs_le16_to_cpu(p_mrec->attrs_offset);
    uint32_t bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    uint64_t alloc_le = pfs_cpu_to_le64((uint64_t)p_ino->allocated_size);
    uint64_t data_le = pfs_cpu_to_le64((uint64_t)p_ino->data_size);

    while (off + 8U <= bytes_in_use)
    {
        ntfs_attr_record_t *p_attr = (ntfs_attr_record_t *)(p_base + off);

        if (NTFS_AT_END == p_attr->type)
        {
            break;
        }
        uint32_t alen = pfs_le32_to_cpu(p_attr->length);
        if (alen < 24U || 0U != (alen & 7U))
        {
            break;
        }
        if (NTFS_AT_FILE_NAME != p_attr->type || 0 != p_attr->non_resident)
        {
            off += alen;
            continue;
        }
        uint16_t val_off = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
        uint32_t val_len = pfs_le32_to_cpu(p_attr->data.resident.value_length);
        if (val_len < (uint32_t)offsetof(ntfs_file_name_attr_t, file_name))
        {
            off += alen;
            continue;
        }
        ntfs_file_name_attr_t *p_fn = (ntfs_file_name_attr_t *)((uint8_t *)p_attr + val_off);
        uint64_t old_alloc_le = p_fn->allocated_size;
        uint64_t old_data_le = p_fn->data_size;
        uint8_t old_name_type = p_fn->file_name_type;
        p_fn->allocated_size = alloc_le;
        p_fn->data_size = data_le;

        // 旧卷/保留文件可能仍是纯 WIN32; 8.3 兼容名升为 WIN32_AND_DOS (CHKDSK)
        pfs_ntfs_uni_name_t uni;
        memset(&uni, 0, sizeof(uni));
        uni.len = p_fn->file_name_length;
        if (NTFS_FILE_NAME_DOS != p_fn->file_name_type && uni.len > 0U &&
                uni.len <= NTFS_MAXIMUM_FILE_NAME_LENGTH)
        {
            for (uint32_t i = 0; i < uni.len; i++)
            {
                uni.name[i] = pfs_le16_to_cpu(p_fn->file_name[i]);
            }
            if (NTFS_FILE_NAME_WIN32 == p_fn->file_name_type &&
                    0 != pfs_ntfs_uni_is_dos_compatible(&uni))
            {
                p_fn->file_name_type = NTFS_FILE_NAME_WIN32_AND_DOS;
            }
        }

        // WIN32 与 DOS 短名项均须回写 $I30 完整副本
        uint64_t parent_mft = NTFS_MREF(pfs_le64_to_cpu(p_fn->parent_directory));
        pfs_ntfs_inode_t parent_ino;
        memset(&parent_ino, 0, sizeof(parent_ino));
        int rc = pfs_ntfs_inode_load(p_sbi, parent_mft, &parent_ino);
        if (PFS_NTFS_RET_OK == rc)
        {
            rc = pfs_ntfs_index_update_filename_dup(p_sbi, &parent_ino, p_fn);
            pfs_ntfs_inode_reset(&parent_ino);
        }
        if (PFS_NTFS_RET_OK != rc)
        {
            // 索引失败则回滚 MFT 缓冲, 避免写盘后 CHKDSK 报 $I30 不正确
            p_fn->allocated_size = old_alloc_le;
            p_fn->data_size = old_data_le;
            p_fn->file_name_type = old_name_type;
            return rc;
        }
        off += alen;
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_file_sync(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta)
{
    if (NULL == p_sbi || NULL == p_meta)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_meta->valid || !p_meta->ino.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    pfs_ntfs_inode_t *p_ino = &p_meta->ino;
    if (!p_ino->sizes_dirty && !p_ino->runlist_dirty)
    {
        return PFS_NTFS_RET_OK;
    }

    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;
    uint32_t rec_size = p_bs->mft_record_size;
    uint8_t *p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int rc = pfs_ntfs_mft_read_record(p_sbi, p_ino->mft_no, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    rc = pfs_ntfs_attr_update_data(p_sbi, (ntfs_mft_record_t *)p_rec_buf, p_ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    // sizes 变更时同步 $FILE_NAME + 父目录 $I30 副本 (否则 Win 显示大小为 0)
    if (p_ino->sizes_dirty || p_ino->runlist_dirty)
    {
        rc = pfs_ntfs_file_sync_filename_dups(p_sbi, (ntfs_mft_record_t *)p_rec_buf, p_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
    }

    rc = pfs_ntfs_mft_write_record(p_sbi, p_ino->mft_no, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    p_ino->sizes_dirty = false;
    p_ino->runlist_dirty = false;

out:
    pfs_free(p_rec_buf);
    return rc;
}

int pfs_ntfs_file_write(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t pos,
        const void *buf, int len, int *p_put)
{
    if (NULL == p_sbi || NULL == p_meta || NULL == buf || NULL == p_put || len <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_meta->valid || !p_meta->ino.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 > pos)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    *p_put = 0;
    pfs_ntfs_inode_t *p_ino = &p_meta->ino;
    int64_t need_end = pos + (int64_t)len;

    if (p_ino->data_resident)
    {
        uint32_t new_res;

        if ((uint32_t)need_end > p_ino->data_resident_size)
        {
            new_res = (uint32_t)need_end;
            if (NULL == p_ino->p_data_resident)
            {
                p_ino->p_data_resident = (uint8_t *)pfs_malloc(new_res);
                if (NULL == p_ino->p_data_resident)
                {
                    return PFS_NTFS_RET_ENOMEM;
                }
            }
            else
            {
                uint8_t *p_grow = (uint8_t *)pfs_realloc(p_ino->p_data_resident, new_res);

                if (NULL == p_grow)
                {
                    return PFS_NTFS_RET_ENOMEM;
                }
                p_ino->p_data_resident = p_grow;
            }
            if (p_ino->data_resident_size < new_res)
            {
                memset(p_ino->p_data_resident + p_ino->data_resident_size, 0,
                        (size_t)(new_res - p_ino->data_resident_size));
            }
            p_ino->data_resident_size = new_res;
        }
        memcpy(p_ino->p_data_resident + pos, buf, (size_t)len);
        if (need_end > p_ino->data_size)
        {
            p_ino->data_size = need_end;
        }
        if (need_end > p_ino->initialized_size)
        {
            p_ino->initialized_size = need_end;
        }
        if (need_end > p_ino->allocated_size)
        {
            p_ino->allocated_size = need_end;
        }
        p_ino->sizes_dirty = true;
        *p_put = len;
        return pfs_ntfs_file_sync(p_sbi, p_meta);
    }

    int rc = pfs_ntfs_file_extend_allocated(p_sbi, p_ino, need_end);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    int put = 0;
    rc = pfs_ntfs_rl_pwrite(p_sbi, &p_ino->data_runlist, p_ino->allocated_size, pos, buf, len,
            &put);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    int64_t new_init = pos + (int64_t)put;
    if (new_init > p_ino->initialized_size)
    {
        p_ino->initialized_size = new_init;
        p_ino->sizes_dirty = true;
    }
    if (need_end > p_ino->data_size)
    {
        p_ino->data_size = need_end;
        p_ino->sizes_dirty = true;
    }

    *p_put = put;
    rc = pfs_ntfs_file_sync(p_sbi, p_meta);
    return rc;
}

int pfs_ntfs_file_truncate(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_file_meta_t *p_meta, int64_t size)
{
    if (NULL == p_sbi || NULL == p_meta)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_meta->valid || !p_meta->ino.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0 > size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    pfs_ntfs_inode_t *p_ino = &p_meta->ino;
    pfs_ntfs_bs_t *p_bs = &p_sbi->bs;

    if (size >= p_ino->data_size)
    {
        if (size == p_ino->data_size)
        {
            return PFS_NTFS_RET_OK;
        }
        int rc = pfs_ntfs_file_extend_allocated(p_sbi, p_ino, size);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        p_ino->data_size = size;
        p_ino->sizes_dirty = true;
        return pfs_ntfs_file_sync(p_sbi, p_meta);
    }

    // 缩小
    int64_t new_vcn = pfs_ntfs_file_cluster_count(p_sbi, size);
    int64_t old_vcn = pfs_ntfs_file_cluster_count(p_sbi, p_ino->allocated_size);

    if (!p_ino->data_resident && old_vcn > new_vcn)
    {
        int rc = pfs_ntfs_cluster_free_from_rl(p_sbi, &p_ino->data_runlist, new_vcn, old_vcn - new_vcn);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        rc = pfs_ntfs_runlist_truncate(&p_ino->data_runlist, new_vcn);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        p_ino->runlist_dirty = true;
        p_ino->allocated_size = new_vcn * (int64_t)p_bs->cluster_size;
    }

    if (p_ino->data_resident)
    {
        if ((uint32_t)size < p_ino->data_resident_size && NULL != p_ino->p_data_resident)
        {
            if (0U == (uint32_t)size)
            {
                pfs_free(p_ino->p_data_resident);
                p_ino->p_data_resident = NULL;
                p_ino->data_resident_size = 0;
            }
            else
            {
                p_ino->data_resident_size = (uint32_t)size;
            }
        }
        p_ino->allocated_size = size;
    }

    p_ino->data_size = size;
    if (p_ino->initialized_size > size)
    {
        p_ino->initialized_size = size;
    }
    p_ino->sizes_dirty = true;

    return pfs_ntfs_file_sync(p_sbi, p_meta);
}

// end
