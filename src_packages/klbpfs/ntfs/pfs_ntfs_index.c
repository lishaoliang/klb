// Doc Encode : UTF-8 BOM, Unix(LF)
// 目录索引 (linux-7.1 fs/ntfs/index.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

#include <stddef.h>
#include <string.h>

#define PFS_NTFS_INDEX_BLOCK_SIZE       4096u

static int pfs_ntfs_index_entry_is_end(const ntfs_index_entry_header_t *p_ie)
{
    uint16_t flags = pfs_le16_to_cpu(p_ie->flags);

    if (0U != (flags & (uint16_t)pfs_le16_to_cpu(NTFS_INDEX_ENTRY_END)))
    {
        return 1;
    }
    return 0;
}

static int pfs_ntfs_index_entry_is_node(const ntfs_index_entry_header_t *p_ie)
{
    uint16_t flags = pfs_le16_to_cpu(p_ie->flags);

    if (0U != (flags & (uint16_t)pfs_le16_to_cpu(NTFS_INDEX_ENTRY_NODE)))
    {
        return 1;
    }
    return 0;
}

static int64_t pfs_ntfs_index_entry_child_vcn(const ntfs_index_entry_header_t *p_ie)
{
    uint16_t elen = pfs_le16_to_cpu(p_ie->length);

    if (elen < 8U)
    {
        return -1;
    }
    return (int64_t)pfs_le64_to_cpu(*(const uint64_t *)((const uint8_t *)p_ie + elen - 8U));
}

static uint32_t pfs_ntfs_index_align8(uint32_t v)
{
    return (v + 7U) & ~7U;
}

static int pfs_ntfs_index_ih_is_leaf(const ntfs_index_header_t *p_ih)
{
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        const ntfs_index_entry_header_t *p_ie = (const ntfs_index_entry_header_t *)((const uint8_t *)p_ih + off);

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            // LARGE 根常仅有 END|NODE; END 带 NODE 表示有子块, 非叶
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                return 0;
            }
            break;
        }
        if (pfs_ntfs_index_entry_is_node(p_ie))
        {
            return 0;
        }
        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }
    return 1;
}

static int pfs_ntfs_index_find_insert_ofs(pfs_ntfs_sbi_t *p_sbi, ntfs_index_header_t *p_ih,
        const ntfs_file_name_attr_t *p_fn_key, uint32_t *p_ins_ofs)
{
    const pfs_ntfs_upcase_t *p_up = &p_sbi->upcase;
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;
    pfs_ntfs_uni_name_t uni;

    uni.len = p_fn_key->file_name_length;
    for (uint32_t i = 0; i < (uint32_t)p_fn_key->file_name_length; i++)
    {
        uni.name[i] = pfs_le16_to_cpu(p_fn_key->file_name[i]);
    }

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + off);
        const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            *p_ins_ofs = off;
            return PFS_NTFS_RET_OK;
        }

        // DOS/WIN32 同序排列; 插入时不得跳过 DOS (否则 btree 乱序致 CHKDSK)
        int rc_cmp = pfs_ntfs_collate_names(uni.name, uni.len, p_ent->key.file_name.file_name,
                (uint32_t)p_ent->key.file_name.file_name_length, 1, PFS_NTFS_IGNORE_CASE, p_up);
        if (0 == rc_cmp)
        {
            return PFS_NTFS_RET_EEXIST;
        }
        if (-1 == rc_cmp)
        {
            *p_ins_ofs = off;
            return PFS_NTFS_RET_OK;
        }

        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }

    return PFS_NTFS_RET_EBADMSG;
}

static int pfs_ntfs_index_ih_insert(pfs_ntfs_sbi_t *p_sbi, ntfs_index_header_t *p_ih,
        uint32_t ins_ofs, const ntfs_file_name_attr_t *p_fn_key, uint64_t mref)
{
    uint32_t key_len = (uint32_t)(offsetof(ntfs_file_name_attr_t, file_name) +
            (uint32_t)p_fn_key->file_name_length * 2U);
    uint32_t ent_len = pfs_ntfs_index_align8((uint32_t)sizeof(ntfs_index_entry_header_t) + key_len);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t alloc_size = pfs_le32_to_cpu(p_ih->allocated_size);
    uint32_t end_ofs = ins_ofs;
    while (end_ofs + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_end = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + end_ofs);
        uint16_t end_len = pfs_le16_to_cpu(p_end->length);
        if (pfs_ntfs_index_entry_is_end(p_end))
        {
            break;
        }
        end_ofs += end_len;
        if (0U == end_len)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
    }
    if (end_ofs + sizeof(ntfs_index_entry_header_t) > index_len)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    ntfs_index_entry_header_t *p_end = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + end_ofs);
    uint16_t end_len = pfs_le16_to_cpu(p_end->length);

    if (index_len + ent_len > alloc_size)
    {
        return PFS_NTFS_RET_ENOSPC;
    }

    memmove((uint8_t *)p_ih + ins_ofs + ent_len, (uint8_t *)p_ih + ins_ofs, index_len - ins_ofs);

    ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + ins_ofs);
    memset(p_ie, 0, ent_len);
    p_ie->data.dir.indexed_file = pfs_cpu_to_le64(mref);
    p_ie->length = pfs_cpu_to_le16((uint16_t)ent_len);
    p_ie->key_length = pfs_cpu_to_le16((uint16_t)key_len);
    p_ie->flags = pfs_cpu_to_le16(0);
    memcpy((uint8_t *)p_ie + sizeof(ntfs_index_entry_header_t), p_fn_key, key_len);

    p_ih->index_length = pfs_cpu_to_le32(index_len + ent_len);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_index_descend_child(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx,
        ntfs_index_header_t *p_ih, const ntfs_file_name_attr_t *p_fn_key, int64_t *p_vcn)
{
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;
    int64_t child_vcn = -1;
    pfs_ntfs_uni_name_t uni;

    uni.len = p_fn_key->file_name_length;
    for (uint32_t i = 0; i < (uint32_t)p_fn_key->file_name_length; i++)
    {
        uni.name[i] = pfs_le16_to_cpu(p_fn_key->file_name[i]);
    }

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + off);
        const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
            }
            break;
        }

        // 下降路径含 DOS 短名项 (与 insert 同序)
        if (pfs_ntfs_are_names_equal(uni.name, uni.len, p_ent->key.file_name.file_name,
                (uint32_t)p_ent->key.file_name.file_name_length, PFS_NTFS_IGNORE_CASE,
                &p_sbi->upcase))
        {
            // 下降路径命中键: 有 NODE 则进入子节点 (勿当 EEXIST)
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
            }
            break;
        }

        int rc_cmp = pfs_ntfs_collate_names(uni.name, uni.len, p_ent->key.file_name.file_name,
                (uint32_t)p_ent->key.file_name.file_name_length, 1, PFS_NTFS_IGNORE_CASE,
                &p_sbi->upcase);
        if (-1 == rc_cmp)
        {
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
            }
            break;
        }

        if (pfs_ntfs_index_entry_is_node(p_ie))
        {
            child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
        }

        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }

    if (0 > child_vcn)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    *p_vcn = child_vcn;
    (void)p_idx;
    return PFS_NTFS_RET_OK;
}

static uint8_t pfs_ntfs_index_root_clusters_per_block(uint32_t index_block_size, uint32_t cluster_size)
{
    if (index_block_size >= cluster_size)
    {
        return (uint8_t)(index_block_size / cluster_size);
    }
    return (uint8_t)(index_block_size / PFS_NTFS_MST_BLOCK_SIZE);
}

// LARGE 空根: NODE|END + VCN0, 指向 INDEX_ALLOCATION 首块 (对齐 mkfs)
static int pfs_ntfs_index_build_root_large_empty(uint8_t *buf, uint32_t buf_cap,
        uint32_t cluster_size, uint32_t *p_val_len)
{
    if (NULL == buf || buf_cap < 256U || NULL == p_val_len)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    memset(buf, 0, buf_cap);
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)buf;
    p_root->type = NTFS_AT_FILE_NAME;
    p_root->collation_rule = NTFS_COLLATION_FILE_NAME;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_index_root_clusters_per_block(
            PFS_NTFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    uint32_t ent_len = (uint32_t)(sizeof(ntfs_index_entry_header_t) + sizeof(uint64_t));
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)sizeof(ntfs_index_header_t) + ent_len);
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_LARGE_INDEX;
    ntfs_index_entry_header_t *p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)ent_len);
    p_ent->key_length = 0;
    p_ent->flags = (uint16_t)(NTFS_INDEX_ENTRY_NODE | NTFS_INDEX_ENTRY_END);
    p_ent->reserved = 0;
    uint64_t *p_vcn = (uint64_t *)((uint8_t *)p_ent + ent_len - sizeof(uint64_t));
    *p_vcn = pfs_cpu_to_le64(0ULL);
    uint32_t val_len = (uint32_t)(sizeof(ntfs_index_root_t) + ent_len);
    *p_val_len = val_len;
    return PFS_NTFS_RET_OK;
}

// 空叶 INDX (仅 END); write 前由 index_write_block / pre_write_mst 打 USA
static int pfs_ntfs_index_build_empty_leaf(uint8_t *buf, uint32_t buf_size)
{
    if (NULL == buf || buf_size < PFS_NTFS_INDEX_BLOCK_SIZE)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    memset(buf, 0, buf_size);
    ntfs_index_block_t *p_ib = (ntfs_index_block_t *)buf;
    uint16_t usa_count = (uint16_t)(1U + buf_size / PFS_NTFS_MST_BLOCK_SIZE);
    uint32_t entries_off = (uint32_t)((sizeof(ntfs_index_header_t) + (uint32_t)usa_count * 2U + 7U) & ~7U);
    uint32_t ih_base = (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t));
    p_ib->magic = NTFS_MAGIC_INDX;
    p_ib->usa_ofs = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_block_t));
    p_ib->usa_count = pfs_cpu_to_le16(usa_count);
    p_ib->lsn = pfs_cpu_to_le64(0);
    p_ib->index_block_vcn = pfs_cpu_to_le64(0);
    ntfs_index_header_t *p_ih = &p_ib->index;
    p_ih->entries_offset = pfs_cpu_to_le32(entries_off);
    p_ih->allocated_size = pfs_cpu_to_le32(buf_size - ih_base);
    p_ih->flags = (uint8_t)NTFS_LEAF_NODE;
    ntfs_index_entry_header_t *p_end = (ntfs_index_entry_header_t *)(buf + ih_base + entries_off);
    p_end->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_end->key_length = 0;
    p_end->flags = NTFS_INDEX_ENTRY_END;
    p_end->reserved = 0;
    uint32_t index_len = entries_off + (uint32_t)sizeof(ntfs_index_entry_header_t);
    p_ih->index_length = pfs_cpu_to_le32(index_len);
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_index_root_writeback(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir,
        const uint8_t *p_root_val, uint32_t root_size)
{
    uint32_t rec_size = p_sbi->bs.mft_record_size;
    uint8_t *p_rec_buf = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec_buf)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    int rc = pfs_ntfs_mft_read_record(p_sbi, p_dir->mft_no, p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_rec_buf);
        return rc;
    }

    ntfs_mft_record_t *p_mrec = (ntfs_mft_record_t *)p_rec_buf;
    ntfs_attr_record_t *p_attr = pfs_ntfs_attr_find(p_mrec, NTFS_AT_INDEX_ROOT, g_pfs_ntfs_attr_name_i30,
            PFS_NTFS_ATTR_NAME_I30_LEN);
    if (NULL == p_attr || 0U != p_attr->non_resident)
    {
        pfs_free(p_rec_buf);
        return PFS_NTFS_RET_EBADMSG;
    }

    uint32_t old_attr_len = pfs_le32_to_cpu(p_attr->length);
    uint32_t attr_ofs = (uint32_t)((uint8_t *)p_attr - p_rec_buf);
    uint16_t val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
    uint32_t attr_len = pfs_ntfs_index_align8((uint32_t)val_ofs + root_size);
    if (attr_ofs + attr_len + (uint32_t)sizeof(uint32_t) > rec_size)
    {
        pfs_free(p_rec_buf);
        return PFS_NTFS_RET_ENOSPC;
    }

    // 扩容时要求其后为 AT_END (SMALL 目录 INDEX_ROOT 为末属性)
    if (attr_len > old_attr_len)
    {
        ntfs_attr_record_t *p_end = (ntfs_attr_record_t *)((uint8_t *)p_attr + old_attr_len);
        if (NTFS_AT_END != p_end->type)
        {
            pfs_free(p_rec_buf);
            return PFS_NTFS_RET_EOPNOTSUPP;
        }
    }

    memcpy((uint8_t *)p_attr + val_ofs, p_root_val, root_size);
    p_attr->data.resident.value_length = pfs_cpu_to_le32(root_size);

    ntfs_attr_record_t *p_after = (ntfs_attr_record_t *)((uint8_t *)p_attr + old_attr_len);
    if (NTFS_AT_END == p_after->type)
    {
        p_attr->length = pfs_cpu_to_le32(attr_len);
        ntfs_attr_record_t *p_end = (ntfs_attr_record_t *)(p_rec_buf + attr_ofs + attr_len);
        p_end->type = NTFS_AT_END;
        p_end->length = 0;
        uint32_t used = pfs_ntfs_index_align8(attr_ofs + attr_len + (uint32_t)sizeof(uint32_t));
        if (used > rec_size)
        {
            used = rec_size;
        }
        p_mrec->bytes_in_use = pfs_cpu_to_le32(used);
    }
    else
    {
        // LARGE: 其后有 INDEX_ALLOCATION/BITMAP, 仅改 value, 保持属性槽长
        if (attr_len > old_attr_len)
        {
            pfs_free(p_rec_buf);
            return PFS_NTFS_RET_EOPNOTSUPP;
        }
        p_attr->length = pfs_cpu_to_le32(old_attr_len);
    }

    rc = pfs_ntfs_mft_write_record(p_sbi, p_dir->mft_no, p_rec_buf);
    pfs_free(p_rec_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (NULL == p_dir->i30.p_root)
    {
        return PFS_NTFS_RET_OK;
    }
    if (p_dir->i30.root_size < root_size)
    {
        uint8_t *p_mem = (uint8_t *)pfs_realloc(p_dir->i30.p_root, root_size);
        if (NULL == p_mem)
        {
            return PFS_NTFS_RET_ENOMEM;
        }
        p_dir->i30.p_root = p_mem;
    }
    memcpy(p_dir->i30.p_root, p_root_val, root_size);
    p_dir->i30.root_size = root_size;
    return PFS_NTFS_RET_OK;
}

// SMALL 根写回 ENOSPC: 条目迁入叶, 升为 LARGE ($INDEX_ROOT + IA + BITMAP)
static int pfs_ntfs_index_promote_small_to_large(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir)
{
    if (NULL == p_sbi || NULL == p_dir)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_index_t *p_idx = &p_dir->i30;
    if (!p_idx->valid || p_idx->has_allocation || NULL == p_idx->p_root)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_runlist_t idx_rl;
    memset(&idx_rl, 0, sizeof(idx_rl));

    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    ntfs_index_header_t *p_ih = &p_root->index;
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;
    uint32_t mid_len = 0U;
    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + off);
        uint16_t elen = pfs_le16_to_cpu(p_ie->length);

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            mid_len = off - entries_ofs;
            break;
        }
        if (0U == elen)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        off += elen;
    }
    if (off + sizeof(ntfs_index_entry_header_t) > index_len)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    uint8_t *p_mid = NULL;
    uint8_t *p_leaf = NULL;
    uint8_t *p_zero = NULL;
    uint8_t *p_rec = NULL;
    int64_t idx_clusters = 0;

    if (mid_len > 0U)
    {
        p_mid = (uint8_t *)pfs_malloc(mid_len);
        if (NULL == p_mid)
        {
            return PFS_NTFS_RET_ENOMEM;
        }
        memcpy(p_mid, (uint8_t *)p_ih + entries_ofs, mid_len);
    }

    idx_clusters = (int64_t)((PFS_NTFS_INDEX_BLOCK_SIZE + p_sbi->bs.cluster_size - 1U) /
            p_sbi->bs.cluster_size);
    if (idx_clusters < 1)
    {
        idx_clusters = 1;
    }
    uint32_t alloc_bytes = (uint32_t)idx_clusters * p_sbi->bs.cluster_size;

    int rc = pfs_ntfs_cluster_alloc(p_sbi, idx_clusters, PFS_NTFS_ZONE_DATA, &idx_rl);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_mid);
        return rc;
    }

    p_leaf = (uint8_t *)pfs_malloc(PFS_NTFS_INDEX_BLOCK_SIZE);
    if (NULL == p_leaf)
    {
        rc = PFS_NTFS_RET_ENOMEM;
        goto fail_rl;
    }
    rc = pfs_ntfs_index_build_empty_leaf(p_leaf, PFS_NTFS_INDEX_BLOCK_SIZE);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    ntfs_index_block_t *p_ib = (ntfs_index_block_t *)p_leaf;
    ntfs_index_header_t *p_leaf_ih = &p_ib->index;
    uint32_t leaf_entries_ofs = pfs_le32_to_cpu(p_leaf_ih->entries_offset);
    uint32_t leaf_ih_base = (uint32_t)(sizeof(ntfs_index_block_t) - sizeof(ntfs_index_header_t));
    if (mid_len > 0U)
    {
        uint32_t leaf_alloc = pfs_le32_to_cpu(p_leaf_ih->allocated_size);
        uint32_t leaf_index_len = leaf_entries_ofs + mid_len + (uint32_t)sizeof(ntfs_index_entry_header_t);
        if (leaf_index_len > leaf_alloc)
        {
            rc = PFS_NTFS_RET_ENOSPC;
            goto fail_rl;
        }
        memmove(p_leaf + leaf_ih_base + leaf_entries_ofs + mid_len,
                p_leaf + leaf_ih_base + leaf_entries_ofs,
                sizeof(ntfs_index_entry_header_t));
        memcpy(p_leaf + leaf_ih_base + leaf_entries_ofs, p_mid, mid_len);
        p_leaf_ih->index_length = pfs_cpu_to_le32(leaf_index_len);
    }
    pfs_free(p_mid);
    p_mid = NULL;

    uint8_t ir_buf[256];
    uint32_t ir_len = 0;
    rc = pfs_ntfs_index_build_root_large_empty(ir_buf, (uint32_t)sizeof(ir_buf),
            p_sbi->bs.cluster_size, &ir_len);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    uint32_t rec_size = p_sbi->bs.mft_record_size;
    p_rec = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec)
    {
        rc = PFS_NTFS_RET_ENOMEM;
        goto fail_rl;
    }
    uint64_t mft_no = p_dir->mft_no;
    rc = pfs_ntfs_mft_read_record(p_sbi, mft_no, p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    ntfs_mft_record_t *p_mrec = (ntfs_mft_record_t *)p_rec;
    ntfs_attr_record_t *p_attr = pfs_ntfs_attr_find(p_mrec, NTFS_AT_INDEX_ROOT, g_pfs_ntfs_attr_name_i30,
            PFS_NTFS_ATTR_NAME_I30_LEN);
    if (NULL == p_attr || 0U != p_attr->non_resident)
    {
        rc = PFS_NTFS_RET_EBADMSG;
        goto fail_rl;
    }

    uint32_t old_attr_len = pfs_le32_to_cpu(p_attr->length);
    uint32_t attr_ofs = (uint32_t)((uint8_t *)p_attr - p_rec);
    uint16_t val_ofs = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
    uint32_t attr_len = pfs_ntfs_index_align8((uint32_t)val_ofs + ir_len);
    if (attr_ofs + attr_len + (uint32_t)sizeof(uint32_t) > rec_size)
    {
        rc = PFS_NTFS_RET_ENOSPC;
        goto fail_rl;
    }
    ntfs_attr_record_t *p_end = (ntfs_attr_record_t *)((uint8_t *)p_attr + old_attr_len);
    if (NTFS_AT_END != p_end->type)
    {
        rc = PFS_NTFS_RET_EBADMSG;
        goto fail_rl;
    }

    memcpy((uint8_t *)p_attr + val_ofs, ir_buf, ir_len);
    p_attr->data.resident.value_length = pfs_cpu_to_le32(ir_len);
    p_attr->length = pfs_cpu_to_le32(attr_len);
    p_end = (ntfs_attr_record_t *)(p_rec + attr_ofs + attr_len);
    p_end->type = NTFS_AT_END;
    p_end->length = 0;
    uint32_t used = pfs_ntfs_index_align8(attr_ofs + attr_len + (uint32_t)sizeof(uint32_t));
    if (used > rec_size)
    {
        used = rec_size;
    }
    p_mrec->bytes_in_use = pfs_cpu_to_le32(used);

    rc = pfs_ntfs_mft_add_nonres(p_mrec, rec_size, NTFS_AT_INDEX_ALLOCATION,
            g_pfs_ntfs_attr_name_i30, PFS_NTFS_ATTR_NAME_I30_LEN, &idx_rl,
            p_sbi->bs.cluster_size, (uint64_t)PFS_NTFS_INDEX_BLOCK_SIZE);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    uint8_t ibm[8];
    memset(ibm, 0, sizeof(ibm));
    ibm[0] = 0x01;
    rc = pfs_ntfs_mft_add_resident(p_mrec, rec_size, NTFS_AT_BITMAP,
            g_pfs_ntfs_attr_name_i30, PFS_NTFS_ATTR_NAME_I30_LEN, ibm, (uint32_t)sizeof(ibm));
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    // 整簇清零后再写叶, 避免簇尾垃圾触发 chkdsk $I30
    p_zero = (uint8_t *)pfs_mallocz(alloc_bytes);
    if (NULL == p_zero)
    {
        rc = PFS_NTFS_RET_ENOMEM;
        goto fail_rl;
    }
    rc = pfs_ntfs_pre_write_mst_fixup(p_leaf, PFS_NTFS_INDEX_BLOCK_SIZE);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }
    memcpy(p_zero, p_leaf, PFS_NTFS_INDEX_BLOCK_SIZE);
    int put = 0;
    rc = pfs_ntfs_rl_pwrite(p_sbi, &idx_rl, (int64_t)alloc_bytes, 0, p_zero, (int)alloc_bytes,
            &put);
    pfs_ntfs_post_write_mst_fixup(p_leaf);
    if (PFS_NTFS_RET_OK != rc || put != (int)alloc_bytes)
    {
        if (PFS_NTFS_RET_OK == rc)
        {
            rc = PFS_NTFS_RET_EIO;
        }
        goto fail_rl;
    }

    rc = pfs_ntfs_mft_write_record(p_sbi, mft_no, p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail_rl;
    }

    pfs_free(p_zero);
    pfs_free(p_leaf);
    pfs_free(p_rec);
    pfs_ntfs_runlist_reset(&idx_rl);

    pfs_ntfs_inode_reset(p_dir);
    rc = pfs_ntfs_inode_load(p_sbi, mft_no, p_dir);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (NTFS_FILE_root == mft_no)
    {
        (void)pfs_ntfs_root_reload(p_sbi);
    }
    return PFS_NTFS_RET_OK;

fail_rl:
    if (idx_rl.count > 0)
    {
        (void)pfs_ntfs_cluster_free_from_rl(p_sbi, &idx_rl, 0, idx_clusters);
    }
    pfs_ntfs_runlist_reset(&idx_rl);
    pfs_free(p_mid);
    pfs_free(p_zero);
    pfs_free(p_leaf);
    pfs_free(p_rec);
    return rc;
}

// SMALL $INDEX_ROOT 扩容: 使 allocated_size 能容纳再插入一条 FILE_NAME 项
static int pfs_ntfs_index_root_ensure_space(pfs_ntfs_inode_t *p_dir,
        const ntfs_file_name_attr_t *p_fn)
{
    pfs_ntfs_index_t *p_idx = &p_dir->i30;
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root || p_idx->has_allocation)
    {
        return PFS_NTFS_RET_OK;
    }

    ntfs_index_header_t *p_ih = &p_root->index;
    uint32_t key_len = (uint32_t)(offsetof(ntfs_file_name_attr_t, file_name) +
            (uint32_t)p_fn->file_name_length * 2U);
    uint32_t ent_len = pfs_ntfs_index_align8((uint32_t)sizeof(ntfs_index_entry_header_t) + key_len);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t alloc_size = pfs_le32_to_cpu(p_ih->allocated_size);
    if (index_len + ent_len <= alloc_size)
    {
        return PFS_NTFS_RET_OK;
    }

    uint32_t need_alloc = index_len + ent_len;
    uint32_t new_root_size = (uint32_t)offsetof(ntfs_index_root_t, index) + need_alloc;
    uint8_t *p_new = (uint8_t *)pfs_realloc(p_idx->p_root, new_root_size);
    if (NULL == p_new)
    {
        return PFS_NTFS_RET_ENOMEM;
    }
    if (new_root_size > p_idx->root_size)
    {
        memset(p_new + p_idx->root_size, 0, (size_t)(new_root_size - p_idx->root_size));
    }
    p_idx->p_root = p_new;
    p_idx->root_size = new_root_size;
    p_root = (ntfs_index_root_t *)p_new;
    p_root->index.allocated_size = pfs_cpu_to_le32(need_alloc);
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_index_read_block(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx, int64_t vcn,
        uint8_t *p_buf)
{
    if (NULL == p_sbi || NULL == p_idx || NULL == p_buf)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_idx->valid || !p_idx->has_allocation)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (0U == p_idx->block_size || 0U == p_idx->vcn_size)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int64_t pos = (int64_t)vcn * (int64_t)p_idx->vcn_size;
    int got = 0;
    int rc = pfs_ntfs_rl_pread(p_sbi, &p_idx->alloc_runlist, pos + (int64_t)p_idx->block_size, pos,
            p_buf, (int)p_idx->block_size, &got);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (got != (int)p_idx->block_size)
    {
        return PFS_NTFS_RET_EIO;
    }

    rc = pfs_ntfs_post_read_mst_fixup(p_buf, p_idx->block_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    ntfs_index_block_t *p_ib = (ntfs_index_block_t *)p_buf;
    if (NTFS_MAGIC_INDX != p_ib->magic)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_index_write_block(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx, int64_t vcn,
        uint8_t *p_buf)
{
    if (NULL == p_sbi || NULL == p_idx || NULL == p_buf)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }
    if (!p_idx->valid || !p_idx->has_allocation)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    int rc = pfs_ntfs_pre_write_mst_fixup(p_buf, p_idx->block_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    int64_t pos = (int64_t)vcn * (int64_t)p_idx->vcn_size;
    int put = 0;
    rc = pfs_ntfs_rl_pwrite(p_sbi, &p_idx->alloc_runlist, pos + (int64_t)p_idx->block_size, pos,
            p_buf, (int)p_idx->block_size, &put);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_post_write_mst_fixup(p_buf);
        return rc;
    }
    if (put != (int)p_idx->block_size)
    {
        pfs_ntfs_post_write_mst_fixup(p_buf);
        return PFS_NTFS_RET_EIO;
    }

    pfs_ntfs_post_write_mst_fixup(p_buf);
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_index_insert_filename(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const ntfs_file_name_attr_t *p_fn, uint64_t mref)
{
    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_fn)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi) || !p_dir_ino->valid || !p_dir_ino->is_dir ||
            !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    ntfs_index_header_t *p_ih = &p_root->index;
    int in_root = 1;
    uint8_t *p_block = NULL;
    int64_t vcn = 0;
    uint32_t ins_ofs = 0;

    if (p_idx->has_allocation)
    {
        p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
        if (NULL == p_block)
        {
            return PFS_NTFS_RET_ENOMEM;
        }

        int rc;
        while (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            int rc = pfs_ntfs_index_descend_child(p_sbi, p_idx, p_ih, p_fn, &vcn);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            p_ih = &((ntfs_index_block_t *)p_block)->index;
            in_root = 0;
        }
    }

    int rc = pfs_ntfs_index_find_insert_ofs(p_sbi, p_ih, p_fn, &ins_ofs);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_block);
        return rc;
    }

    // SMALL 根: 插入前扩容 allocated_size / p_root
    if (in_root && !p_idx->has_allocation)
    {
        rc = pfs_ntfs_index_root_ensure_space(p_dir_ino, p_fn);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_free(p_block);
            return rc;
        }
        p_root = (ntfs_index_root_t *)p_idx->p_root;
        p_ih = &p_root->index;
    }

    rc = pfs_ntfs_index_ih_insert(p_sbi, p_ih, ins_ofs, p_fn, mref);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_block);
        return rc;
    }

    if (in_root)
    {
        uint32_t new_root_size;

        // SMALL 根保持 allocated_size == index_length
        if (!p_idx->has_allocation)
        {
            p_ih->allocated_size = p_ih->index_length;
        }
        // index_length 相对 INDEX_HEADER; 属性值长度 = offsetof(index) + index_length
        new_root_size = (uint32_t)(offsetof(ntfs_index_root_t, index) +
                pfs_le32_to_cpu(p_ih->index_length));

        rc = pfs_ntfs_index_root_writeback(p_sbi, p_dir_ino, (const uint8_t *)p_root, new_root_size);
        // MFT 装不下扩大的 SMALL 根: 内存已含新项, 升 LARGE 写入叶
        if ((PFS_NTFS_RET_ENOSPC == rc || PFS_NTFS_RET_EOPNOTSUPP == rc) &&
                !p_idx->has_allocation)
        {
            rc = pfs_ntfs_index_promote_small_to_large(p_sbi, p_dir_ino);
            pfs_free(p_block);
            return rc;
        }
    }
    else
    {
        rc = pfs_ntfs_index_write_block(p_sbi, p_idx, vcn, p_block);
    }

    pfs_free(p_block);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (NTFS_FILE_root == p_dir_ino->mft_no)
    {
        (void)pfs_ntfs_root_reload(p_sbi);
    }
    return PFS_NTFS_RET_OK;
}

// want_dos: 0=只匹配非 DOS (长名); 1=只匹配 DOS 短名
static int pfs_ntfs_index_find_remove_ofs(pfs_ntfs_sbi_t *p_sbi, ntfs_index_header_t *p_ih,
        const pfs_ntfs_uni_name_t *p_uni, int want_dos, uint32_t *p_rm_ofs, uint16_t *p_rm_len)
{
    const pfs_ntfs_upcase_t *p_up = &p_sbi->upcase;
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + off);
        const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            break;
        }

        const ntfs_file_name_attr_t *p_fn = &p_ent->key.file_name;
        int is_dos = (NTFS_FILE_NAME_DOS == p_fn->file_name_type) ? 1 : 0;
        if (0 != want_dos)
        {
            if (0 == is_dos)
            {
                off += pfs_le16_to_cpu(p_ie->length);
                if (0U == pfs_le16_to_cpu(p_ie->length))
                {
                    break;
                }
                continue;
            }
        }
        else if (0 != is_dos)
        {
            off += pfs_le16_to_cpu(p_ie->length);
            if (0U == pfs_le16_to_cpu(p_ie->length))
            {
                break;
            }
            continue;
        }

        // 与 lookup_scan_level 一致: 先 are_names_equal 再命中
        if (pfs_ntfs_are_names_equal(p_uni->name, p_uni->len, p_fn->file_name,
                (uint32_t)p_fn->file_name_length, PFS_NTFS_IGNORE_CASE, p_up))
        {
            *p_rm_ofs = off;
            *p_rm_len = pfs_le16_to_cpu(p_ie->length);
            return PFS_NTFS_RET_OK;
        }

        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }
    return PFS_NTFS_RET_ENOENT;
}

static int pfs_ntfs_index_ih_remove(pfs_ntfs_sbi_t *p_sbi, ntfs_index_header_t *p_ih,
        uint32_t rm_ofs, uint16_t rm_len)
{
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    if (rm_ofs + rm_len > index_len)
    {
        return PFS_NTFS_RET_EBADMSG;
    }
    uint32_t tail = index_len - (rm_ofs + rm_len);
    if (tail > 0U)
    {
        memmove((uint8_t *)p_ih + rm_ofs, (uint8_t *)p_ih + rm_ofs + rm_len, tail);
    }
    p_ih->index_length = pfs_cpu_to_le32(index_len - rm_len);
    (void)p_sbi;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_index_remove_filename(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni)
{
    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_uni)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi) || !p_dir_ino->valid || !p_dir_ino->is_dir ||
            !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    ntfs_index_header_t *p_ih = &p_root->index;
    int in_root = 1;
    uint8_t *p_block = NULL;
    int64_t vcn = 0;
    uint32_t rm_ofs = 0;
    uint16_t rm_len = 0;

    if (p_idx->has_allocation)
    {
        p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
        if (NULL == p_block)
        {
            return PFS_NTFS_RET_ENOMEM;
        }

        int rc;
        while (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            // offsetof(file_name)+255*2; 512 装不下 NTFS_MAXIMUM_FILE_NAME_LENGTH
            uint8_t key_buf[offsetof(ntfs_file_name_attr_t, file_name) +
                    NTFS_MAXIMUM_FILE_NAME_LENGTH * 2U];
            ntfs_file_name_attr_t *p_key = (ntfs_file_name_attr_t *)key_buf;

            memset(key_buf, 0, sizeof(key_buf));
            p_key->file_name_length = (uint8_t)p_uni->len;
            for (uint32_t ki = 0; ki < p_uni->len; ki++)
            {
                p_key->file_name[ki] = pfs_cpu_to_le16(p_uni->name[ki]);
            }
            int rc = pfs_ntfs_index_descend_child(p_sbi, p_idx, p_ih, p_key, &vcn);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            p_ih = &((ntfs_index_block_t *)p_block)->index;
            in_root = 0;
        }
    }

    int rc = pfs_ntfs_index_find_remove_ofs(p_sbi, p_ih, p_uni, 0, &rm_ofs, &rm_len);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_block);
        return rc;
    }

    rc = pfs_ntfs_index_ih_remove(p_sbi, p_ih, rm_ofs, rm_len);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_block);
        return rc;
    }

    if (in_root)
    {
        // index_length 相对 INDEX_HEADER; 属性值长度 = offsetof(index) + index_length
        uint32_t new_root_size = (uint32_t)(offsetof(ntfs_index_root_t, index) +
                pfs_le32_to_cpu(p_ih->index_length));

        rc = pfs_ntfs_index_root_writeback(p_sbi, p_dir_ino, (const uint8_t *)p_root, new_root_size);
    }
    else
    {
        rc = pfs_ntfs_index_write_block(p_sbi, p_idx, vcn, p_block);
    }

    pfs_free(p_block);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (NTFS_FILE_root == p_dir_ino->mft_no)
    {
        (void)pfs_ntfs_root_reload(p_sbi);
    }
    return PFS_NTFS_RET_OK;
}

// 摘掉目录中指向 mft_no 的全部 $I30 项 (MFT 复用前清陈旧索引, 防双名同 mref)
int pfs_ntfs_index_remove_by_mref(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        uint64_t mft_no)
{
    if (NULL == p_sbi || NULL == p_dir_ino)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi) || !p_dir_ino->valid || !p_dir_ino->is_dir ||
            !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    int removed = 0;
    if (!p_idx->has_allocation)
    {
        for (;;)
        {
            uint32_t entries_ofs = pfs_le32_to_cpu(p_root->index.entries_offset);
            uint32_t index_len = pfs_le32_to_cpu(p_root->index.index_length);
            uint32_t off = entries_ofs;
            uint32_t rm_ofs = 0;
            uint16_t rm_len = 0;
            int found = 0;

            while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
            {
                ntfs_index_entry_header_t *p_ie =
                        (ntfs_index_entry_header_t *)((uint8_t *)&p_root->index + off);
                const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

                if (pfs_ntfs_index_entry_is_end(p_ie))
                {
                    break;
                }
                if (NTFS_MREF(pfs_le64_to_cpu(p_ent->data.dir.indexed_file)) == mft_no)
                {
                    rm_ofs = off;
                    rm_len = pfs_le16_to_cpu(p_ie->length);
                    found = 1;
                    break;
                }
                off += pfs_le16_to_cpu(p_ie->length);
                if (0U == pfs_le16_to_cpu(p_ie->length))
                {
                    break;
                }
            }
            if (!found)
            {
                break;
            }
            int rc = pfs_ntfs_index_ih_remove(p_sbi, &p_root->index, rm_ofs, rm_len);
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
            {
                uint32_t new_root_size = (uint32_t)(offsetof(ntfs_index_root_t, index) +
                        pfs_le32_to_cpu(p_root->index.index_length));

                p_root->index.allocated_size = p_root->index.index_length;
                rc = pfs_ntfs_index_root_writeback(p_sbi, p_dir_ino, (const uint8_t *)p_root,
                        new_root_size);
            }
            if (PFS_NTFS_RET_OK != rc)
            {
                return rc;
            }
            p_root = (ntfs_index_root_t *)p_dir_ino->i30.p_root;
            removed = 1;
        }
        if (removed && NTFS_FILE_root == p_dir_ino->mft_no)
        {
            (void)pfs_ntfs_root_reload(p_sbi);
        }
        return PFS_NTFS_RET_OK;
    }

    if (0U == p_idx->alloc_runlist.count || NULL == p_idx->alloc_runlist.p_rl)
    {
        return PFS_NTFS_RET_OK;
    }
    int64_t max_vcn = p_idx->alloc_runlist.p_rl[p_idx->alloc_runlist.count].vcn - 1;
    if (0 > max_vcn)
    {
        return PFS_NTFS_RET_OK;
    }

    uint8_t *p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
    if (NULL == p_block)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    for (int64_t vcn = 0; vcn <= max_vcn; vcn++)
    {
        int rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
        if (PFS_NTFS_RET_OK != rc)
        {
            continue;
        }
        ntfs_index_header_t *p_ih = &((ntfs_index_block_t *)p_block)->index;
        if (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            continue;
        }

        for (;;)
        {
            uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
            uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
            uint32_t off = entries_ofs;
            uint32_t rm_ofs = 0;
            uint16_t rm_len = 0;
            int found = 0;

            while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
            {
                ntfs_index_entry_header_t *p_ie =
                        (ntfs_index_entry_header_t *)((uint8_t *)p_ih + off);
                const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

                if (pfs_ntfs_index_entry_is_end(p_ie))
                {
                    break;
                }
                if (NTFS_MREF(pfs_le64_to_cpu(p_ent->data.dir.indexed_file)) == mft_no)
                {
                    rm_ofs = off;
                    rm_len = pfs_le16_to_cpu(p_ie->length);
                    found = 1;
                    break;
                }
                off += pfs_le16_to_cpu(p_ie->length);
                if (0U == pfs_le16_to_cpu(p_ie->length))
                {
                    break;
                }
            }
            if (!found)
            {
                break;
            }
            rc = pfs_ntfs_index_ih_remove(p_sbi, p_ih, rm_ofs, rm_len);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            rc = pfs_ntfs_index_write_block(p_sbi, p_idx, vcn, p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            removed = 1;
        }
    }

    pfs_free(p_block);
    if (removed && NTFS_FILE_root == p_dir_ino->mft_no)
    {
        (void)pfs_ntfs_root_reload(p_sbi);
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_index_ih_has_name(pfs_ntfs_sbi_t *p_sbi, const ntfs_index_header_t *p_ih,
        const pfs_ntfs_uni_name_t *p_uni)
{
    const pfs_ntfs_upcase_t *p_up = &p_sbi->upcase;
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        const ntfs_index_entry_header_t *p_ie =
                (const ntfs_index_entry_header_t *)((const uint8_t *)p_ih + off);
        const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            break;
        }
        const ntfs_file_name_attr_t *p_fn = &p_ent->key.file_name;
        if (pfs_ntfs_are_names_equal(p_uni->name, p_uni->len, p_fn->file_name,
                (uint32_t)p_fn->file_name_length, PFS_NTFS_IGNORE_CASE, p_up))
        {
            return PFS_NTFS_RET_OK;
        }
        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }
    return PFS_NTFS_RET_ENOENT;
}

// 父目录是否已有同名 (含 DOS; 用于短名 ~n 避让)
int pfs_ntfs_index_has_filename(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni)
{
    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_uni || 0U == p_uni->len)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_dir_ino->valid || !p_dir_ino->is_dir || !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    const pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    const ntfs_index_root_t *p_root = (const ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    if (!p_idx->has_allocation)
    {
        return pfs_ntfs_index_ih_has_name(p_sbi, &p_root->index, p_uni);
    }

    if (0U == p_idx->alloc_runlist.count || NULL == p_idx->alloc_runlist.p_rl)
    {
        return PFS_NTFS_RET_ENOENT;
    }
    int64_t max_vcn = p_idx->alloc_runlist.p_rl[p_idx->alloc_runlist.count].vcn - 1;
    if (0 > max_vcn)
    {
        return PFS_NTFS_RET_ENOENT;
    }

    uint8_t *p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
    if (NULL == p_block)
    {
        return PFS_NTFS_RET_ENOMEM;
    }

    for (int64_t vcn = 0; vcn <= max_vcn; vcn++)
    {
        const ntfs_index_header_t *p_ih;

        int rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
        if (PFS_NTFS_RET_OK != rc)
        {
            continue;
        }
        p_ih = &((const ntfs_index_block_t *)p_block)->index;
        if (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            continue;
        }
        rc = pfs_ntfs_index_ih_has_name(p_sbi, p_ih, p_uni);
        if (PFS_NTFS_RET_OK == rc)
        {
            pfs_free(p_block);
            return PFS_NTFS_RET_OK;
        }
    }

    pfs_free(p_block);
    return PFS_NTFS_RET_ENOENT;
}

// 回写索引项中完整 $FILE_NAME 副本 (CHKDSK / 资源管理器与 MFT 对照)
int pfs_ntfs_index_update_filename_dup(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_inode_t *p_dir_ino,
        const ntfs_file_name_attr_t *p_fn)
{
    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_fn)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi) || !p_dir_ino->valid || !p_dir_ino->is_dir ||
            !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_uni_name_t uni;
    memset(&uni, 0, sizeof(uni));
    uni.len = p_fn->file_name_length;
    if (0U == uni.len || uni.len > NTFS_MAXIMUM_FILE_NAME_LENGTH)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    for (uint32_t i = 0; i < uni.len; i++)
    {
        uni.name[i] = pfs_le16_to_cpu(p_fn->file_name[i]);
    }
    uint32_t key_len = (uint32_t)(offsetof(ntfs_file_name_attr_t, file_name) + uni.len * 2U);

    pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    ntfs_index_header_t *p_ih = &p_root->index;
    int in_root = 1;
    uint8_t *p_block = NULL;
    int64_t vcn = 0;
    uint32_t rm_ofs = 0;
    uint16_t rm_len = 0;

    if (p_idx->has_allocation)
    {
        p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
        if (NULL == p_block)
        {
            return PFS_NTFS_RET_ENOMEM;
        }

        while (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            int rc = pfs_ntfs_index_descend_child(p_sbi, p_idx, p_ih, p_fn, &vcn);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            p_ih = &((ntfs_index_block_t *)p_block)->index;
            in_root = 0;
        }
    }

    // DOS 短名项单独匹配; 长名路径跳过 DOS
    int rc = pfs_ntfs_index_find_remove_ofs(p_sbi, p_ih, &uni,
            (NTFS_FILE_NAME_DOS == p_fn->file_name_type) ? 1 : 0, &rm_ofs, &rm_len);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_block);
        return rc;
    }

    ntfs_index_entry_t *p_ent = (ntfs_index_entry_t *)((uint8_t *)p_ih + rm_ofs);
    uint32_t ie_key_len = (uint32_t)pfs_le16_to_cpu(p_ent->key_length);
    if (ie_key_len != key_len)
    {
        pfs_free(p_block);
        return PFS_NTFS_RET_EBADMSG;
    }
    memcpy(&p_ent->key.file_name, p_fn, key_len);

    if (in_root)
    {
        uint32_t new_root_size = (uint32_t)(offsetof(ntfs_index_root_t, index) +
                pfs_le32_to_cpu(p_ih->index_length));

        rc = pfs_ntfs_index_root_writeback(p_sbi, p_dir_ino, (const uint8_t *)p_root, new_root_size);
    }
    else
    {
        rc = pfs_ntfs_index_write_block(p_sbi, p_idx, vcn, p_block);
    }

    pfs_free(p_block);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (NTFS_FILE_root == p_dir_ino->mft_no)
    {
        (void)pfs_ntfs_root_reload(p_sbi);
    }
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_index_dir_is_empty(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino)
{
    if (NULL == p_sbi || NULL == p_dir_ino || !p_dir_ino->valid || !p_dir_ino->is_dir ||
            !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    const pfs_ntfs_index_t *p_idx = &p_dir_ino->i30;
    const ntfs_index_root_t *p_root = (const ntfs_index_root_t *)p_idx->p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    const ntfs_index_header_t *p_ih = &p_root->index;
    uint8_t *p_block = NULL;

    if (p_idx->has_allocation)
    {
        p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
        if (NULL == p_block)
        {
            return PFS_NTFS_RET_ENOMEM;
        }

        while (!pfs_ntfs_index_ih_is_leaf(p_ih))
        {
            ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih +
                    pfs_le32_to_cpu(p_ih->entries_offset));

            if (!pfs_ntfs_index_entry_is_end(p_ie) || !pfs_ntfs_index_entry_is_node(p_ie))
            {
                pfs_free(p_block);
                return PFS_NTFS_RET_EBADMSG;
            }
            int64_t vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
            int rc = pfs_ntfs_index_read_block(p_sbi, p_idx, vcn, p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_free(p_block);
                return rc;
            }
            p_ih = &((ntfs_index_block_t *)p_block)->index;
        }
    }

    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t off = entries_ofs;
    int count = 0;

    while (off + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        const ntfs_index_entry_header_t *p_ie = (const ntfs_index_entry_header_t *)((const uint8_t *)p_ih + off);

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            break;
        }
        if (!pfs_ntfs_index_entry_is_node(p_ie))
        {
            count++;
        }
        off += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }

    pfs_free(p_block);
    if (0 == count)
    {
        return PFS_NTFS_RET_OK;
    }
    return PFS_NTFS_RET_ENOTEMPTY;
}

// end
