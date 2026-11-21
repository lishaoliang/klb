// Doc Encode : UTF-8 BOM, Unix(LF)
// 属性查找/读写 (linux-7.1 fs/ntfs/attrib.c + attrlist.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

const uint16_t g_pfs_ntfs_attr_name_i30[PFS_NTFS_ATTR_NAME_I30_LEN] = { '$', 'I', '3', '0' };

static int pfs_ntfs_attr_name_match(const ntfs_attr_record_t *p_attr, const uint16_t *p_name,
        uint32_t name_len)
{
    if (0U == name_len && NULL == p_name)
    {
        return (0U == p_attr->name_length) ? 1 : 0;
    }
    if (NULL == p_name || 0U == name_len)
    {
        return 0;
    }
    if ((uint32_t)p_attr->name_length != name_len)
    {
        return 0;
    }
    const uint16_t *p_on = (const uint16_t *)((const uint8_t *)p_attr + pfs_le16_to_cpu(p_attr->name_offset));
    for (uint32_t i = 0; i < name_len; i++)
    {
        if (p_name[i] != pfs_le16_to_cpu(p_on[i]))
        {
            return 0;
        }
    }
    return 1;
}

ntfs_attr_record_t *pfs_ntfs_attr_find_unnamed(ntfs_mft_record_t *p_mrec, uint32_t type_le)
{
    return pfs_ntfs_attr_find(p_mrec, type_le, NULL, 0);
}

ntfs_attr_record_t *pfs_ntfs_attr_find(ntfs_mft_record_t *p_mrec, uint32_t type_le,
        const uint16_t *name_host_utf16, uint32_t name_len)
{
    if (NULL == p_mrec)
    {
        return NULL;
    }
    uint16_t attrs_ofs = pfs_le16_to_cpu(p_mrec->attrs_offset);
    uint32_t bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    if (attrs_ofs >= bytes_in_use)
    {
        return NULL;
    }

    uint8_t *p_base = (uint8_t *)p_mrec;
    uint8_t *p_end = p_base + bytes_in_use;
    ntfs_attr_record_t *p_attr = (ntfs_attr_record_t *)(p_base + attrs_ofs);

    while ((uint8_t *)p_attr + 8 <= p_end)
    {
        if (NTFS_AT_END == p_attr->type)
        {
            break;
        }
        uint32_t alen = pfs_le32_to_cpu(p_attr->length);
        if (alen < 24U || 0U != (alen & 7U))
        {
            break;
        }
        if ((uint8_t *)p_attr + alen > p_end)
        {
            break;
        }
        if (type_le == p_attr->type &&
                pfs_ntfs_attr_name_match(p_attr, name_host_utf16, name_len))
        {
            return p_attr;
        }
        p_attr = (ntfs_attr_record_t *)((uint8_t *)p_attr + alen);
    }
    return NULL;
}

static uint32_t pfs_ntfs_attr_align8(uint32_t v)
{
    return (v + 7U) & ~7U;
}

static int pfs_ntfs_attr_resident_max_value(const ntfs_mft_record_t *p_mrec,
        const ntfs_attr_record_t *p_attr)
{
    uint32_t bytes_alloc;
    uint32_t bytes_in_use;
    uint32_t val_ofs;
    uint32_t attr_ofs;

    bytes_alloc = pfs_le32_to_cpu(p_mrec->bytes_allocated);
    val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
    attr_ofs = (uint32_t)((const uint8_t *)p_attr - (const uint8_t *)p_mrec);
    bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    (void)bytes_in_use;
    if (attr_ofs + val_ofs >= bytes_alloc)
    {
        return 0;
    }
    return bytes_alloc - attr_ofs - val_ofs;
}

static int pfs_ntfs_attr_write_non_resident(ntfs_attr_record_t *p_attr, pfs_ntfs_inode_t *p_ino,
        uint32_t mp_cap)
{
    uint32_t built;
    uint16_t mp_ofs;
    uint32_t attr_len;
    int64_t highest_vcn;

    mp_ofs = pfs_le16_to_cpu(p_attr->data.non_resident.mapping_pairs_offset);
    if (mp_ofs < 64U)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    attr_len = pfs_le32_to_cpu(p_attr->length);
    if (mp_ofs >= attr_len)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int rc = pfs_ntfs_mapping_pairs_build(&p_ino->data_runlist,
            (uint8_t *)p_attr + mp_ofs, attr_len - mp_ofs, &built);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (0U == p_ino->data_runlist.count)
    {
        highest_vcn = 0;
    }
    else
    {
        highest_vcn = p_ino->data_runlist.p_rl[p_ino->data_runlist.count].vcn - 1;
        if (highest_vcn < 0)
        {
            highest_vcn = 0;
        }
    }

    p_attr->data.non_resident.lowest_vcn = pfs_cpu_to_le64(0);
    p_attr->data.non_resident.highest_vcn = pfs_cpu_to_le64((uint64_t)highest_vcn);
    p_attr->data.non_resident.allocated_size = pfs_cpu_to_le64((uint64_t)p_ino->allocated_size);
    p_attr->data.non_resident.data_size = pfs_cpu_to_le64((uint64_t)p_ino->data_size);
    p_attr->data.non_resident.initialized_size = pfs_cpu_to_le64((uint64_t)p_ino->initialized_size);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_attr_convert_to_non_resident(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        ntfs_attr_record_t *p_attr, pfs_ntfs_inode_t *p_ino)
{
    pfs_ntfs_bs_t *p_bs;
    uint32_t cluster_size;
    uint32_t mp_cap;
    uint32_t new_len;
    uint32_t old_len;
    uint32_t built;
    uint8_t pairs_buf[256];
    pfs_ntfs_runlist_t new_rl;
    int64_t need_vcn;
    int put;

    p_bs = &p_sbi->bs;
    cluster_size = p_bs->cluster_size;
    old_len = pfs_le32_to_cpu(p_attr->length);

    int rc;
    if (0U == p_ino->data_runlist.count)
    {
        need_vcn = (p_ino->allocated_size + (int64_t)cluster_size - 1) / (int64_t)cluster_size;
        if (need_vcn <= 0 && p_ino->data_resident_size > 0)
        {
            need_vcn = 1;
        }
        if (need_vcn > 0)
        {
            memset(&new_rl, 0, sizeof(new_rl));
            rc = pfs_ntfs_cluster_alloc(p_sbi, need_vcn, PFS_NTFS_ZONE_DATA, &new_rl);
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
            rc = pfs_ntfs_runlist_merge(&p_ino->data_runlist, &new_rl);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_ntfs_runlist_reset(&new_rl);
                return rc;
            }
            if (p_ino->allocated_size < need_vcn * (int64_t)cluster_size)
            {
                p_ino->allocated_size = need_vcn * (int64_t)cluster_size;
            }
            p_ino->runlist_dirty = true;
        }
    }

    if (p_ino->data_resident_size > 0 && p_ino->data_runlist.count > 0)
    {
        rc = pfs_ntfs_rl_pwrite(p_sbi, &p_ino->data_runlist, p_ino->allocated_size, 0,
                p_ino->p_data_resident, (int)p_ino->data_resident_size, &put);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }

    rc = pfs_ntfs_mapping_pairs_build(&p_ino->data_runlist, pairs_buf, sizeof(pairs_buf), &built);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    new_len = pfs_ntfs_attr_align8(64U + built);
    mp_cap = new_len - 64U;
    if (new_len > pfs_le32_to_cpu(p_mrec->bytes_allocated) - ((uint8_t *)p_attr - (uint8_t *)p_mrec))
    {
        return PFS_NTFS_RET_EOPNOTSUPP;
    }

    p_attr->non_resident = 1;
    p_attr->data.non_resident.lowest_vcn = pfs_cpu_to_le64(0);
    p_attr->data.non_resident.mapping_pairs_offset = pfs_cpu_to_le16(64);
    p_attr->data.non_resident.compression_unit = 0;
    memset(p_attr->data.non_resident.reserved, 0, sizeof(p_attr->data.non_resident.reserved));
    p_attr->length = pfs_cpu_to_le32(new_len);

    rc = pfs_ntfs_attr_write_non_resident(p_attr, p_ino, mp_cap);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    // 调整 AT_END 与 bytes_in_use (DATA 常为末属性)
    {
        uint8_t *p_base = (uint8_t *)p_mrec;
        uint32_t attr_ofs = (uint32_t)((uint8_t *)p_attr - p_base);
        uint32_t used;
        ntfs_attr_record_t *p_end;

        p_end = (ntfs_attr_record_t *)(p_base + attr_ofs + new_len);
        p_end->type = NTFS_AT_END;
        p_end->length = 0;
        used = pfs_ntfs_attr_align8(attr_ofs + new_len + (uint32_t)sizeof(uint32_t));
        if (used > pfs_le32_to_cpu(p_mrec->bytes_allocated))
        {
            used = pfs_le32_to_cpu(p_mrec->bytes_allocated);
        }
        p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
    }

    p_ino->data_resident = false;
    if (NULL != p_ino->p_data_resident)
    {
        pfs_free(p_ino->p_data_resident);
        p_ino->p_data_resident = NULL;
    }
    p_ino->data_resident_size = 0;
    (void)old_len;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_attr_update_data(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec,
        pfs_ntfs_inode_t *p_ino)
{
    ntfs_attr_record_t *p_attr;
    uint32_t attr_len;
    uint32_t val_len;
    uint32_t val_ofs;
    uint32_t max_val;
    uint8_t *p_val;

    if (NULL == p_sbi || NULL == p_mrec || NULL == p_ino)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_DATA);
    if (NULL == p_attr)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int rc;
    if (p_ino->data_resident)
    {
        if (0U == p_attr->non_resident)
        {
            val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
            max_val = pfs_ntfs_attr_resident_max_value(p_mrec, p_attr);
            if (p_ino->data_resident_size > max_val)
            {
                rc = pfs_ntfs_attr_convert_to_non_resident(p_sbi, p_mrec, p_attr, p_ino);
                if (PFS_NTFS_RET_OK != rc)
                {
                    return rc;
                }
                p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_DATA);
                if (NULL == p_attr || 0U == p_attr->non_resident)
                {
                    return PFS_NTFS_RET_EBADMSG;
                }
            }
            else
            {
                uint8_t *p_base = (uint8_t *)p_mrec;
                uint32_t attr_ofs = (uint32_t)((uint8_t *)p_attr - p_base);
                uint32_t old_attr_len = pfs_le32_to_cpu(p_attr->length);
                uint32_t used;
                ntfs_attr_record_t *p_next;
                ntfs_attr_record_t *p_end;

                // $DATA 须为末属性, 其后为 AT_END; 扩 length 时同步 bytes_in_use
                if (attr_ofs + old_attr_len + (uint32_t)sizeof(uint32_t) >
                        pfs_le32_to_cpu(p_mrec->bytes_in_use))
                {
                    return PFS_NTFS_RET_EBADMSG;
                }
                p_next = (ntfs_attr_record_t *)(p_base + attr_ofs + old_attr_len);
                if (NTFS_AT_END != p_next->type)
                {
                    return PFS_NTFS_RET_EOPNOTSUPP;
                }

                val_len = p_ino->data_resident_size;
                attr_len = pfs_ntfs_attr_align8(val_ofs + val_len);
                if (attr_ofs + attr_len + (uint32_t)sizeof(uint32_t) >
                        pfs_le32_to_cpu(p_mrec->bytes_allocated))
                {
                    return PFS_NTFS_RET_ENOSPC;
                }
                p_val = (uint8_t *)p_attr + val_ofs;
                if (NULL != p_ino->p_data_resident && val_len > 0U)
                {
                    memcpy(p_val, p_ino->p_data_resident, val_len);
                }
                p_attr->data.resident.value_length = pfs_cpu_to_le32(val_len);
                p_attr->length = pfs_cpu_to_le32(attr_len);
                p_end = (ntfs_attr_record_t *)(p_base + attr_ofs + attr_len);
                p_end->type = NTFS_AT_END;
                p_end->length = 0;
                used = pfs_ntfs_attr_align8(attr_ofs + attr_len + (uint32_t)sizeof(uint32_t));
                if (used > pfs_le32_to_cpu(p_mrec->bytes_allocated))
                {
                    used = pfs_le32_to_cpu(p_mrec->bytes_allocated);
                }
                p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
                p_ino->data_size = (int64_t)val_len;
                p_ino->initialized_size = (int64_t)val_len;
                p_ino->allocated_size = (int64_t)val_len;
                return PFS_NTFS_RET_OK;
            }
        }
        else
        {
            rc = pfs_ntfs_attr_convert_to_non_resident(p_sbi, p_mrec, p_attr, p_ino);
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
            p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_DATA);
            if (NULL == p_attr)
            {
                return PFS_NTFS_RET_EBADMSG;
            }
        }
    }

    if (0U == p_attr->non_resident)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    if (p_ino->runlist_dirty)
    {
        rc = pfs_ntfs_attr_write_non_resident(p_attr, p_ino,
                pfs_le32_to_cpu(p_attr->length) - pfs_le16_to_cpu(p_attr->data.non_resident.mapping_pairs_offset));
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    else if (p_ino->sizes_dirty)
    {
        p_attr->data.non_resident.allocated_size = pfs_cpu_to_le64((uint64_t)p_ino->allocated_size);
        p_attr->data.non_resident.data_size = pfs_cpu_to_le64((uint64_t)p_ino->data_size);
        p_attr->data.non_resident.initialized_size = pfs_cpu_to_le64((uint64_t)p_ino->initialized_size);
    }

    return PFS_NTFS_RET_OK;
}

static uint32_t pfs_ntfs_mft_attr_append_off(ntfs_mft_record_t *p_mrec)
{
    uint8_t *p_base = (uint8_t *)p_mrec;
    uint32_t off = pfs_le16_to_cpu(p_mrec->attrs_offset);
    uint32_t bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    ntfs_attr_record_t *p_a;

    while (off + (uint32_t)sizeof(uint32_t) <= bytes_in_use)
    {
        p_a = (ntfs_attr_record_t *)(p_base + off);
        if (NTFS_AT_END == p_a->type)
        {
            return off;
        }
        if (0U == p_a->length)
        {
            break;
        }
        off += pfs_le32_to_cpu(p_a->length);
    }
    return off;
}

int pfs_ntfs_mft_add_nonres(ntfs_mft_record_t *p_mrec, uint32_t rec_size, uint32_t type,
        const uint16_t *name, uint32_t name_len, const pfs_ntfs_runlist_t *p_rl,
        uint32_t cluster_size, uint64_t data_size)
{
    ntfs_attr_record_t *p_a;
    uint8_t pairs_buf[64];
    uint32_t built;
    uint32_t append_off;
    uint32_t hdr = 64U;
    uint32_t name_bytes;
    uint32_t name_off;
    uint32_t mp_off;
    uint32_t total;
    uint64_t run_clusters;
    uint64_t alloc_size;
    uint16_t next_inst;

    if (NULL == p_mrec || NULL == p_rl || 0U == p_rl->count || 0U == cluster_size)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (NULL == p_rl->p_rl || p_rl->p_rl[0].length <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    int rc = pfs_ntfs_mapping_pairs_build(p_rl, pairs_buf, sizeof(pairs_buf), &built);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    run_clusters = (uint64_t)p_rl->p_rl[0].length;
    alloc_size = run_clusters * (uint64_t)cluster_size;
    if (data_size > alloc_size)
    {
        data_size = alloc_size;
    }

    name_bytes = name_len * 2U;
    name_off = hdr;
    mp_off = pfs_ntfs_attr_align8(hdr + name_bytes);
    total = pfs_ntfs_attr_align8(mp_off + built) + (uint32_t)sizeof(uint32_t);
    append_off = pfs_ntfs_mft_attr_append_off(p_mrec);
    if (append_off + total > rec_size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }

    p_a = (ntfs_attr_record_t *)((uint8_t *)p_mrec + append_off);
    memset(p_a, 0, pfs_ntfs_attr_align8(mp_off + built));
    p_a->type = type;
    p_a->non_resident = 1;
    p_a->name_length = (uint8_t)name_len;
    p_a->name_offset = pfs_cpu_to_le16((uint16_t)name_off);
    p_a->flags = pfs_cpu_to_le16(0);
    next_inst = pfs_le16_to_cpu(p_mrec->next_attr_instance);
    p_a->instance = pfs_cpu_to_le16(next_inst);
    p_mrec->next_attr_instance = pfs_cpu_to_le16((uint16_t)(next_inst + 1U));
    p_a->data.non_resident.lowest_vcn = pfs_cpu_to_le64(0);
    p_a->data.non_resident.highest_vcn = pfs_cpu_to_le64(run_clusters - 1ULL);
    p_a->data.non_resident.mapping_pairs_offset = pfs_cpu_to_le16((uint16_t)mp_off);
    p_a->data.non_resident.compression_unit = 0;
    p_a->data.non_resident.allocated_size = pfs_cpu_to_le64(alloc_size);
    p_a->data.non_resident.data_size = pfs_cpu_to_le64(data_size);
    p_a->data.non_resident.initialized_size = pfs_cpu_to_le64(data_size);
    if (name_len > 0U && NULL != name)
    {
        memcpy((uint8_t *)p_a + name_off, name, name_bytes);
    }
    memcpy((uint8_t *)p_a + mp_off, pairs_buf, built);
    p_a->length = pfs_cpu_to_le32(pfs_ntfs_attr_align8(mp_off + built));

    {
        ntfs_attr_record_t *p_end = (ntfs_attr_record_t *)((uint8_t *)p_a + pfs_le32_to_cpu(p_a->length));
        uint32_t used = pfs_ntfs_attr_align8((uint32_t)((uint8_t *)p_end - (uint8_t *)p_mrec) +
                (uint32_t)sizeof(uint32_t));

        p_end->type = NTFS_AT_END;
        p_end->length = 0;
        p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mft_add_resident(ntfs_mft_record_t *p_mrec, uint32_t rec_size, uint32_t type,
        const uint16_t *name, uint32_t name_len, const void *val, uint32_t val_len)
{
    ntfs_attr_record_t *p_a;
    uint32_t append_off;
    uint32_t hdr = 24U;
    uint32_t name_bytes;
    uint32_t name_off;
    uint32_t val_off;
    uint32_t total;
    uint16_t next_inst;

    if (NULL == p_mrec || rec_size < sizeof(ntfs_mft_record_t) || (NULL == val && val_len > 0U))
    {
        return PFS_NTFS_RET_EINVAL;
    }

    name_bytes = name_len * 2U;
    name_off = hdr;
    val_off = pfs_ntfs_attr_align8(hdr + name_bytes);
    total = pfs_ntfs_attr_align8(val_off + val_len) + (uint32_t)sizeof(uint32_t);
    append_off = pfs_ntfs_mft_attr_append_off(p_mrec);
    if (append_off + total > rec_size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }

    p_a = (ntfs_attr_record_t *)((uint8_t *)p_mrec + append_off);
    memset(p_a, 0, pfs_ntfs_attr_align8(val_off + val_len));
    p_a->type = type;
    p_a->non_resident = 0;
    p_a->name_length = (uint8_t)name_len;
    p_a->name_offset = pfs_cpu_to_le16((uint16_t)name_off);
    p_a->flags = pfs_cpu_to_le16(0);
    next_inst = pfs_le16_to_cpu(p_mrec->next_attr_instance);
    p_a->instance = pfs_cpu_to_le16(next_inst);
    p_mrec->next_attr_instance = pfs_cpu_to_le16((uint16_t)(next_inst + 1U));
    p_a->data.resident.value_length = pfs_cpu_to_le32(val_len);
    p_a->data.resident.value_offset = pfs_cpu_to_le16((uint16_t)val_off);
    p_a->data.resident.flags = (NTFS_AT_FILE_NAME == type) ?
            (uint8_t)NTFS_RESIDENT_ATTR_IS_INDEXED : (uint8_t)0;
    p_a->data.resident.reserved = 0;
    if (name_len > 0U && NULL != name)
    {
        memcpy((uint8_t *)p_a + name_off, name, name_bytes);
    }
    if (val_len > 0U && NULL != val)
    {
        memcpy((uint8_t *)p_a + val_off, val, val_len);
    }
    p_a->length = pfs_cpu_to_le32(pfs_ntfs_attr_align8(val_off + val_len));

    {
        ntfs_attr_record_t *p_end = (ntfs_attr_record_t *)((uint8_t *)p_a + pfs_le32_to_cpu(p_a->length));
        uint32_t used = pfs_ntfs_attr_align8((uint32_t)((uint8_t *)p_end - (uint8_t *)p_mrec) +
                (uint32_t)sizeof(uint32_t));

        p_end->type = NTFS_AT_END;
        p_end->length = 0;
        p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_mft_attr_remove(pfs_ntfs_sbi_t *p_sbi, ntfs_mft_record_t *p_mrec, uint32_t rec_size,
        uint32_t type, const uint16_t *name, uint32_t name_len,
        const pfs_ntfs_uni_name_t *p_fn_match)
{
    uint8_t *p_base = (uint8_t *)p_mrec;
    uint32_t attrs_ofs = pfs_le16_to_cpu(p_mrec->attrs_offset);
    uint32_t bytes_in_use = pfs_le32_to_cpu(p_mrec->bytes_in_use);
    uint32_t off = attrs_ofs;
    ntfs_attr_record_t *p_attr;
    uint32_t alen;

    if (NULL == p_mrec)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    while (off + 8U <= bytes_in_use)
    {
        p_attr = (ntfs_attr_record_t *)(p_base + off);
        if (NTFS_AT_END == p_attr->type)
        {
            break;
        }
        alen = pfs_le32_to_cpu(p_attr->length);
        if (alen < 24U || 0U != (alen & 7U))
        {
            break;
        }

        if (type == p_attr->type && pfs_ntfs_attr_name_match(p_attr, name, name_len))
        {
            int remove = 1;

            if (NTFS_AT_FILE_NAME == type && NULL != p_fn_match)
            {
                uint16_t val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
                const ntfs_file_name_attr_t *p_fn = (const ntfs_file_name_attr_t *)((const uint8_t *)p_attr + val_ofs);

                if (NTFS_FILE_NAME_DOS == p_fn->file_name_type)
                {
                    remove = 0;
                } else if (NULL != p_sbi && !pfs_ntfs_are_names_equal(p_fn_match->name, p_fn_match->len,
                        p_fn->file_name, (uint32_t)p_fn->file_name_length, PFS_NTFS_IGNORE_CASE,
                        &p_sbi->upcase))
                {
                    remove = 0;
                }
            }

            if (remove)
            {
                uint32_t tail = bytes_in_use - (off + alen);

                if (off + alen < bytes_in_use)
                {
                    memmove(p_base + off, p_base + off + alen, tail);
                }
                bytes_in_use -= alen;
                p_mrec->bytes_in_use = pfs_cpu_to_le32(bytes_in_use);
                (void)rec_size;
                return PFS_NTFS_RET_OK;
            }
        }

        off += alen;
    }
    return PFS_NTFS_RET_ENOENT;
}

// end
