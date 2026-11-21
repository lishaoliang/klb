// Doc Encode : UTF-8 BOM, Unix(LF)
// 路径 lookup (linux-7.1 fs/ntfs/namei.c + dir.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_ntfs_lookup_reset(pfs_ntfs_lookup_t *p_lu)
{
    if (NULL == p_lu)
    {
        return;
    }
    memset(p_lu, 0, sizeof(*p_lu));
}

void pfs_ntfs_uni_name_reset(pfs_ntfs_uni_name_t *p_name)
{
    if (NULL == p_name)
    {
        return;
    }
    memset(p_name, 0, sizeof(*p_name));
}

void pfs_ntfs_create_reset(pfs_ntfs_create_t *p_cr)
{
    if (NULL == p_cr)
    {
        return;
    }
    pfs_ntfs_uni_name_reset(&p_cr->name);
    memset(p_cr, 0, sizeof(*p_cr));
}

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

static int pfs_ntfs_lookup_fill_from_entry(const ntfs_index_entry_t *p_ent, pfs_ntfs_lookup_t *p_lu)
{
    const ntfs_file_name_attr_t *p_fn = &p_ent->key.file_name;
    uint64_t mref = pfs_le64_to_cpu(p_ent->data.dir.indexed_file);

    p_lu->mft_no = NTFS_MREF(mref);
    p_lu->seq_no = NTFS_MSEQNO(mref);
    p_lu->is_dir = (0U != (pfs_le32_to_cpu(p_fn->file_attributes) &
            pfs_le32_to_cpu(NTFS_FILE_ATTR_DIRECTORY))) ? true : false;
    p_lu->valid = true;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_lookup_scan_level(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_index_t *p_idx,
        const pfs_ntfs_uni_name_t *p_uni, ntfs_index_header_t *p_ih, uint8_t *p_block,
        pfs_ntfs_lookup_t *p_lu)
{
    const pfs_ntfs_upcase_t *p_up = &p_sbi->upcase;
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);
    uint32_t entry_ofs = entries_ofs;
    int64_t child_vcn = -1;
    int has_child = 0;

    while (entry_ofs + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + entry_ofs);
        const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;
        const ntfs_file_name_attr_t *p_fn;
        int rc_cmp;

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
                has_child = 1;
            }
            break;
        }

        p_fn = &p_ent->key.file_name;
        if (NTFS_FILE_NAME_DOS == p_fn->file_name_type)
        {
            entry_ofs += pfs_le16_to_cpu(p_ie->length);
            if (0U == pfs_le16_to_cpu(p_ie->length))
            {
                break;
            }
            continue;
        }

        if (pfs_ntfs_are_names_equal(p_uni->name, p_uni->len, p_fn->file_name,
                (uint32_t)p_fn->file_name_length, PFS_NTFS_IGNORE_CASE, p_up))
        {
            return pfs_ntfs_lookup_fill_from_entry(p_ent, p_lu);
        }

        rc_cmp = pfs_ntfs_collate_names(p_uni->name, p_uni->len, p_fn->file_name,
                (uint32_t)p_fn->file_name_length, 1, PFS_NTFS_IGNORE_CASE, p_up);
        if (-1 == rc_cmp)
        {
            if (pfs_ntfs_index_entry_is_node(p_ie))
            {
                child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
                has_child = 1;
            }
            break;
        }
        if (0 == rc_cmp)
        {
            rc_cmp = pfs_ntfs_collate_names(p_uni->name, p_uni->len, p_fn->file_name,
                    (uint32_t)p_fn->file_name_length, 1, PFS_NTFS_CASE_SENSITIVE, p_up);
            if (-1 == rc_cmp)
            {
                if (pfs_ntfs_index_entry_is_node(p_ie))
                {
                    child_vcn = pfs_ntfs_index_entry_child_vcn(p_ie);
                    has_child = 1;
                }
                break;
            }
            if (0 == rc_cmp)
            {
                return pfs_ntfs_lookup_fill_from_entry(p_ent, p_lu);
            }
        }

        entry_ofs += pfs_le16_to_cpu(p_ie->length);
        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }

    if (has_child && child_vcn >= 0 && p_idx->has_allocation)
    {
        int rc;
        ntfs_index_block_t *p_ib;

        rc = pfs_ntfs_index_read_block(p_sbi, p_idx, child_vcn, p_block);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        p_ib = (ntfs_index_block_t *)p_block;
        return pfs_ntfs_lookup_scan_level(p_sbi, p_idx, p_uni, &p_ib->index, p_block, p_lu);
    }

    return PFS_NTFS_RET_ENOENT;
}

int pfs_ntfs_lookup_iname(pfs_ntfs_sbi_t *p_sbi, const pfs_ntfs_inode_t *p_dir_ino,
        const pfs_ntfs_uni_name_t *p_uni_name, pfs_ntfs_lookup_t *p_lu)
{
    ntfs_index_root_t *p_root;
    uint8_t *p_block = NULL;

    if (NULL == p_sbi || NULL == p_dir_ino || NULL == p_uni_name || NULL == p_lu)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_dir_ino->valid || !p_dir_ino->is_dir || !p_dir_ino->i30.valid)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_sbi->upcase.valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    pfs_ntfs_lookup_reset(p_lu);
    p_root = (ntfs_index_root_t *)p_dir_ino->i30.p_root;
    if (NULL == p_root)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    if (p_dir_ino->i30.has_allocation)
    {
        p_block = (uint8_t *)pfs_malloc(p_dir_ino->i30.block_size);
        if (NULL == p_block)
        {
            return PFS_NTFS_RET_ENOMEM;
        }
    }

    int rc = pfs_ntfs_lookup_scan_level(p_sbi, &p_dir_ino->i30, p_uni_name, &p_root->index, p_block,
            p_lu);
    pfs_free(p_block);
    return rc;
}

static void pfs_ntfs_path_skip_sep(const char *path, int *p_pos)
{
    while ('/' == path[*p_pos] || '\\' == path[*p_pos])
    {
        (*p_pos)++;
    }
}

static int pfs_ntfs_path_next_component(const char *path, int *p_pos, pfs_ntfs_uni_name_t *p_uni)
{
    int len = 0;
    char comp[256];

    while ('\0' != path[*p_pos] && '/' != path[*p_pos] && '\\' != path[*p_pos])
    {
        if (len >= (int)sizeof(comp) - 1)
        {
            return PFS_NTFS_RET_ENAMETOOLONG;
        }
        comp[len++] = path[*p_pos];
        (*p_pos)++;
    }
    comp[len] = '\0';
    if (0 == len)
    {
        p_uni->len = 0;
        return PFS_NTFS_RET_OK;
    }
    int rc = pfs_ntfs_utf8_to_uni(NULL, comp, len, p_uni);
    return rc;
}

int pfs_ntfs_lookup_path(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_lookup_t *p_lu)
{
    const pfs_ntfs_inode_t *p_dir;
    pfs_ntfs_inode_t loaded_dir;
    pfs_ntfs_lookup_t cur;
    pfs_ntfs_uni_name_t uni;
    int pos;
    bool loaded;

    if (NULL == p_sbi || NULL == path || NULL == p_lu)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_sbi->root.valid)
    {
        return PFS_NTFS_RET_EBUSY;
    }

    pfs_ntfs_lookup_reset(p_lu);
    memset(&loaded_dir, 0, sizeof(loaded_dir));
    p_dir = &p_sbi->root;
    loaded = false;
    pos = 0;
    pfs_ntfs_path_skip_sep(path, &pos);

    if ('\0' == path[pos])
    {
        p_lu->mft_no = p_sbi->root.mft_no;
        p_lu->seq_no = p_sbi->root.seq_no;
        p_lu->is_dir = true;
        p_lu->valid = true;
        return PFS_NTFS_RET_OK;
    }

    for (;;)
    {
        int rc = pfs_ntfs_path_next_component(path, &pos, &uni);
        if (PFS_NTFS_RET_OK != rc)
        {
            if (loaded)
            {
                pfs_ntfs_inode_reset(&loaded_dir);
            }
            return rc;
        }
        if (0U == uni.len)
        {
            break;
        }

        rc = pfs_ntfs_lookup_iname(p_sbi, p_dir, &uni, &cur);
        if (PFS_NTFS_RET_OK != rc)
        {
            if (loaded)
            {
                pfs_ntfs_inode_reset(&loaded_dir);
            }
            return rc;
        }

        pfs_ntfs_path_skip_sep(path, &pos);
        if ('\0' == path[pos])
        {
            *p_lu = cur;
            if (loaded)
            {
                pfs_ntfs_inode_reset(&loaded_dir);
            }
            return PFS_NTFS_RET_OK;
        }

        if (!cur.is_dir)
        {
            if (loaded)
            {
                pfs_ntfs_inode_reset(&loaded_dir);
            }
            return PFS_NTFS_RET_ENOENT;
        }

        if (loaded)
        {
            pfs_ntfs_inode_reset(&loaded_dir);
        }
        rc = pfs_ntfs_inode_load(p_sbi, cur.mft_no, &loaded_dir);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        p_dir = &loaded_dir;
        loaded = true;
    }

    if (loaded)
    {
        pfs_ntfs_inode_reset(&loaded_dir);
    }
    return PFS_NTFS_RET_ENOENT;
}

#define PFS_NTFS_INDEX_BLOCK_SIZE       4096u

static uint32_t pfs_ntfs_namei_align8(uint32_t v)
{
    return (v + 7U) & ~7U;
}

static uint8_t pfs_ntfs_index_root_clusters_per_block(uint32_t index_block_size, uint32_t cluster_size)
{
    if (index_block_size >= cluster_size)
    {
        return (uint8_t)(index_block_size / cluster_size);
    }
    return (uint8_t)(index_block_size / PFS_NTFS_MST_BLOCK_SIZE);
}

int pfs_ntfs_root_reload(pfs_ntfs_sbi_t *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_inode_reset(&p_sbi->root);
    return pfs_ntfs_inode_load(p_sbi, NTFS_FILE_root, &p_sbi->root);
}

static int pfs_ntfs_path_parent_leaf(pfs_ntfs_sbi_t *p_sbi, const char *path,
        pfs_ntfs_lookup_t *p_parent, pfs_ntfs_uni_name_t *p_leaf)
{
    pfs_ntfs_lookup_t lu;
    char parent_path[512];
    const char *leaf_start;
    int parent_len;
    int pos;

    if (NULL == p_sbi || NULL == path || NULL == p_parent || NULL == p_leaf)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_lookup_reset(p_parent);
    pfs_ntfs_uni_name_reset(p_leaf);

    leaf_start = path + strlen(path);
    while (leaf_start > path && ('/' == leaf_start[-1] || '\\' == leaf_start[-1]))
    {
        leaf_start--;
    }
    while (leaf_start > path && '/' != leaf_start[-1] && '\\' != leaf_start[-1])
    {
        leaf_start--;
    }

    if (leaf_start <= path)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pos = (int)(leaf_start - path);
    int rc = pfs_ntfs_path_next_component(path, &pos, p_leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (0U == p_leaf->len)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && ('/' == path[parent_len - 1] || '\\' == path[parent_len - 1]))
    {
        parent_len--;
    }
    if (parent_len <= 0)
    {
        p_parent->mft_no = p_sbi->root.mft_no;
        p_parent->seq_no = p_sbi->root.seq_no;
        p_parent->is_dir = true;
        p_parent->valid = true;
        return PFS_NTFS_RET_OK;
    }

    if (parent_len >= (int)sizeof(parent_path))
    {
        return PFS_NTFS_RET_ENAMETOOLONG;
    }
    memcpy(parent_path, path, (size_t)parent_len);
    parent_path[parent_len] = '\0';

    rc = pfs_ntfs_lookup_path(p_sbi, parent_path, &lu);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (!lu.is_dir)
    {
        return PFS_NTFS_RET_ENOENT;
    }
    *p_parent = lu;
    return PFS_NTFS_RET_OK;
}

// NTFS 3.x 用户文件默认 security_id (与 mkfs SDS id100 / linux SECURITY_ID_FIRST 对齐)
#define PFS_NTFS_DEF_SECURITY_ID            0x100U

// 读父目录 SI 的 security_id; 无效/过短 SI 返回 0
static uint32_t pfs_ntfs_mft_security_id(pfs_ntfs_sbi_t *p_sbi, uint64_t mft_no)
{
    uint8_t *p_buf = NULL;
    ntfs_mft_record_t *p_mrec;
    ntfs_attr_record_t *p_attr;
    const ntfs_standard_information_t *p_si;
    uint32_t rec_size;
    uint32_t sid = 0U;
    uint16_t val_off;
    uint32_t val_len;

    if (NULL == p_sbi)
    {
        return 0U;
    }
    rec_size = p_sbi->bs.mft_record_size;
    p_buf = (uint8_t *)pfs_malloc(rec_size);
    if (NULL == p_buf)
    {
        return 0U;
    }
    int rc = pfs_ntfs_mft_read_record(p_sbi, mft_no, p_buf);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_free(p_buf);
        return 0U;
    }
    p_mrec = (ntfs_mft_record_t *)p_buf;
    p_attr = pfs_ntfs_attr_find_unnamed(p_mrec, NTFS_AT_STANDARD_INFORMATION);
    if (NULL != p_attr && 0 == p_attr->non_resident)
    {
        val_off = pfs_le16_to_cpu(p_attr->data.resident.value_offset);
        val_len = pfs_le32_to_cpu(p_attr->data.resident.value_length);
        // NTFS 3.x SI 72 字节含 security_id; 48 字节旧式无此字段
        if (val_len >= 72U && (uint32_t)val_off + val_len <= pfs_le32_to_cpu(p_attr->length))
        {
            p_si = (const ntfs_standard_information_t *)((const uint8_t *)p_attr + val_off);
            sid = pfs_le32_to_cpu(p_si->ver.v3.security_id);
        }
    }
    pfs_free(p_buf);
    return sid;
}

static int pfs_ntfs_build_file_name_attr(pfs_ntfs_sbi_t *p_sbi, uint64_t parent_mref,
        const pfs_ntfs_uni_name_t *p_name, uint32_t file_attrs, uint64_t now_le,
        uint64_t alloc_size, uint64_t data_size, uint8_t name_type,
        ntfs_file_name_attr_t *p_fn, uint32_t fn_cap, uint32_t *p_fn_size)
{
    uint32_t fn_size;

    (void)p_sbi;
    fn_size = (uint32_t)(offsetof(ntfs_file_name_attr_t, file_name) + p_name->len * 2U);
    if (fn_size > fn_cap)
    {
        return PFS_NTFS_RET_ENAMETOOLONG;
    }

    memset(p_fn, 0, fn_size);
    p_fn->parent_directory = pfs_cpu_to_le64(parent_mref);
    p_fn->creation_time = now_le;
    p_fn->last_data_change_time = now_le;
    p_fn->last_mft_change_time = now_le;
    p_fn->last_access_time = now_le;
    p_fn->allocated_size = pfs_cpu_to_le64(alloc_size);
    p_fn->data_size = pfs_cpu_to_le64(data_size);
    p_fn->file_attributes = file_attrs;
    p_fn->file_name_length = (uint8_t)p_name->len;
    p_fn->file_name_type = name_type;
    for (uint32_t i = 0; i < p_name->len; i++)
    {
        p_fn->file_name[i] = pfs_cpu_to_le16(p_name->name[i]);
    }
    *p_fn_size = fn_size;
    return PFS_NTFS_RET_OK;
}

// SMALL 空根: 仅 END (对齐 mkfs 空目录 / Windows 常见形态)
static int pfs_ntfs_build_index_root_small(uint8_t *buf, uint32_t buf_cap,
        uint32_t cluster_size, uint32_t *p_val_len)
{
    ntfs_index_root_t *p_root;
    ntfs_index_entry_header_t *p_ent;
    uint32_t val_len;

    if (NULL == buf || buf_cap < 256U || NULL == p_val_len)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    memset(buf, 0, buf_cap);
    p_root = (ntfs_index_root_t *)buf;
    p_root->type = NTFS_AT_FILE_NAME;
    p_root->collation_rule = NTFS_COLLATION_FILE_NAME;
    p_root->index_block_size = pfs_cpu_to_le32(PFS_NTFS_INDEX_BLOCK_SIZE);
    p_root->clusters_per_index_block = pfs_ntfs_index_root_clusters_per_block(
            PFS_NTFS_INDEX_BLOCK_SIZE, cluster_size);
    p_root->index.entries_offset = pfs_cpu_to_le32((uint32_t)sizeof(ntfs_index_header_t));
    p_root->index.index_length = pfs_cpu_to_le32(
            (uint32_t)(sizeof(ntfs_index_header_t) + sizeof(ntfs_index_entry_header_t)));
    p_root->index.allocated_size = p_root->index.index_length;
    p_root->index.flags = (uint8_t)NTFS_SMALL_INDEX;
    p_ent = (ntfs_index_entry_header_t *)((uint8_t *)p_root + sizeof(ntfs_index_root_t));
    p_ent->length = pfs_cpu_to_le16((uint16_t)sizeof(ntfs_index_entry_header_t));
    p_ent->key_length = 0;
    p_ent->flags = NTFS_INDEX_ENTRY_END;
    p_ent->reserved = 0;
    val_len = (uint32_t)(sizeof(ntfs_index_root_t) + sizeof(ntfs_index_entry_header_t));
    *p_val_len = val_len;
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_create_entry(pfs_ntfs_sbi_t *p_sbi, const char *path, int is_dir)
{
    pfs_ntfs_lookup_t parent_lu;
    pfs_ntfs_inode_t parent_ino;
    pfs_ntfs_lookup_t exist;
    pfs_ntfs_uni_name_t leaf;
    // offsetof(file_name)+255*2 UTF-16; 512 装不下 PFS_NAME_MAX
    uint8_t fn_buf[offsetof(ntfs_file_name_attr_t, file_name) + 255U * 2U];
    uint8_t dos_fn_buf[offsetof(ntfs_file_name_attr_t, file_name) + 24U];
    uint8_t idx_buf[256];
    uint8_t si_buf[72];
    ntfs_file_name_attr_t *p_fn = (ntfs_file_name_attr_t *)fn_buf;
    ntfs_file_name_attr_t *p_dos_fn = (ntfs_file_name_attr_t *)dos_fn_buf;
    ntfs_standard_information_t *p_si = (ntfs_standard_information_t *)si_buf;
    pfs_ntfs_uni_name_t dos_leaf;
    uint8_t *p_rec = NULL;
    uint64_t mft_no;
    uint64_t parent_mref;
    uint64_t mref;
    uint64_t now_le;
    uint16_t seq_no;
    uint32_t fn_size;
    uint32_t dos_fn_size = 0U;
    uint32_t idx_len;
    uint32_t file_attrs;
    uint32_t rec_size;
    uint32_t security_id;
    uint32_t si_len;
    int need_dos = 0;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    int rc = pfs_ntfs_path_parent_leaf(p_sbi, path, &parent_lu, &leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    memset(&parent_ino, 0, sizeof(parent_ino));
    rc = pfs_ntfs_inode_load(p_sbi, parent_lu.mft_no, &parent_ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_ntfs_lookup_iname(p_sbi, &parent_ino, &leaf, &exist);
    if (PFS_NTFS_RET_OK == rc)
    {
        pfs_ntfs_inode_t probe;

        // 索引残留指向已释放 MFT: 摘掉后继续创建
        memset(&probe, 0, sizeof(probe));
        rc = pfs_ntfs_inode_load(p_sbi, exist.mft_no, &probe);
        if (PFS_NTFS_RET_OK == rc)
        {
            pfs_ntfs_inode_reset(&probe);
            pfs_ntfs_inode_reset(&parent_ino);
            return PFS_NTFS_RET_EEXIST;
        }
        // 仅 ENOENT (无 IN_USE) 才清位图; 其它错误清位会造成 IN_USE 与 $MFT/$BITMAP 不一致
        if (PFS_NTFS_RET_ENOENT != rc)
        {
            pfs_ntfs_inode_reset(&parent_ino);
            return rc;
        }
        rc = pfs_ntfs_index_remove_filename(p_sbi, &parent_ino, &leaf);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_ntfs_inode_reset(&parent_ino);
            return rc;
        }
        if (exist.mft_no >= (uint64_t)NTFS_FILE_first_user)
        {
            (void)pfs_ntfs_mft_bitmap_set_bit(p_sbi, exist.mft_no, 0);
        }
        // parent_ino 可能已在 remove 中 writeback; 重新加载再建
        pfs_ntfs_inode_reset(&parent_ino);
        rc = pfs_ntfs_inode_load(p_sbi, parent_lu.mft_no, &parent_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
    }
    else if (PFS_NTFS_RET_ENOENT != rc)
    {
        pfs_ntfs_inode_reset(&parent_ino);
        return rc;
    }

    rc = pfs_ntfs_mft_record_alloc(p_sbi, is_dir, &mft_no, &seq_no, &p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_inode_reset(&parent_ino);
        return rc;
    }

    // 复用 MFT 前摘掉父目录中仍指向该号的陈旧 $I30 (曾致 TID11/12 文件与目录同指 MFT16)
    rc = pfs_ntfs_index_remove_by_mref(p_sbi, &parent_ino, mft_no);
    if (PFS_NTFS_RET_OK != rc)
    {
        (void)pfs_ntfs_mft_bitmap_set_bit(p_sbi, mft_no, 0);
        pfs_free(p_rec);
        pfs_ntfs_inode_reset(&parent_ino);
        return rc;
    }

    rec_size = p_sbi->bs.mft_record_size;
    now_le = pfs_ntfs_current_time_le();
    parent_mref = NTFS_MK_MREF(parent_lu.mft_no, parent_lu.seq_no);
    mref = NTFS_MK_MREF(mft_no, seq_no);

    file_attrs = NTFS_FILE_ATTR_ARCHIVE;
    if (is_dir)
    {
        file_attrs = (uint32_t)(NTFS_FILE_ATTR_DIRECTORY | NTFS_FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT |
                NTFS_FILE_ATTR_ARCHIVE);
    }

    // NTFS 3.x: 继承父 security_id; 根/无效则用 mkfs 默认 id100 (Everyone Full, 资源管理器可访问)
    security_id = pfs_ntfs_mft_security_id(p_sbi, parent_lu.mft_no);
    if (security_id < PFS_NTFS_DEF_SECURITY_ID)
    {
        security_id = PFS_NTFS_DEF_SECURITY_ID;
    }

    memset(si_buf, 0, sizeof(si_buf));
    p_si->creation_time = now_le;
    p_si->last_data_change_time = now_le;
    p_si->last_mft_change_time = now_le;
    p_si->last_access_time = now_le;
    p_si->file_attributes = file_attrs;
    p_si->ver.v3.security_id = pfs_cpu_to_le32(security_id);
    si_len = 72U;

    rc = pfs_ntfs_mft_add_resident((ntfs_mft_record_t *)p_rec, rec_size, NTFS_AT_STANDARD_INFORMATION,
            NULL, 0, si_buf, si_len);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail;
    }

    // 8.3 兼容: WIN32_AND_DOS; 长名: WIN32 + 独立 DOS 短名 (否则 CHKDSK 报索引不正确)
    need_dos = (0 == pfs_ntfs_uni_is_dos_compatible(&leaf)) ? 1 : 0;
    rc = pfs_ntfs_build_file_name_attr(p_sbi, parent_mref, &leaf, file_attrs, now_le, 0, 0,
            (0 != need_dos) ? NTFS_FILE_NAME_WIN32 : NTFS_FILE_NAME_WIN32_AND_DOS,
            p_fn, (uint32_t)sizeof(fn_buf), &fn_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail;
    }

    rc = pfs_ntfs_mft_add_resident((ntfs_mft_record_t *)p_rec, rec_size, NTFS_AT_FILE_NAME, NULL, 0,
            fn_buf, fn_size);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail;
    }

    if (0 != need_dos)
    {
        memset(&dos_leaf, 0, sizeof(dos_leaf));
        rc = pfs_ntfs_uni_make_dos_name(p_sbi, &parent_ino, &leaf, &dos_leaf);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto fail;
        }
        rc = pfs_ntfs_build_file_name_attr(p_sbi, parent_mref, &dos_leaf, file_attrs, now_le, 0, 0,
                NTFS_FILE_NAME_DOS, p_dos_fn, (uint32_t)sizeof(dos_fn_buf), &dos_fn_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto fail;
        }
        rc = pfs_ntfs_mft_add_resident((ntfs_mft_record_t *)p_rec, rec_size, NTFS_AT_FILE_NAME, NULL,
                0, dos_fn_buf, dos_fn_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto fail;
        }
        ((ntfs_mft_record_t *)p_rec)->link_count = pfs_cpu_to_le16(2);
    }

    if (is_dir)
    {
        // 空目录用 SMALL (对齐 Windows/mkfs); 塞满时 index_insert 升 LARGE
        rc = pfs_ntfs_build_index_root_small(idx_buf, (uint32_t)sizeof(idx_buf),
                p_sbi->bs.cluster_size, &idx_len);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto fail;
        }
        rc = pfs_ntfs_mft_add_resident((ntfs_mft_record_t *)p_rec, rec_size, NTFS_AT_INDEX_ROOT,
                g_pfs_ntfs_attr_name_i30, PFS_NTFS_ATTR_NAME_I30_LEN, idx_buf, idx_len);
    }
    else
    {
        rc = pfs_ntfs_mft_add_resident((ntfs_mft_record_t *)p_rec, rec_size, NTFS_AT_DATA, NULL, 0,
                NULL, 0);
    }
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail;
    }

    rc = pfs_ntfs_mft_write_record(p_sbi, mft_no, p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto fail;
    }

    rc = pfs_ntfs_index_insert_filename(p_sbi, &parent_ino, p_fn, mref);
    if (PFS_NTFS_RET_OK != rc)
    {
        (void)pfs_ntfs_mft_record_free(p_sbi, mft_no, p_rec);
        goto out;
    }
    if (0 != need_dos)
    {
        rc = pfs_ntfs_index_insert_filename(p_sbi, &parent_ino, p_dos_fn, mref);
        if (PFS_NTFS_RET_OK != rc)
        {
            (void)pfs_ntfs_index_remove_by_mref(p_sbi, &parent_ino, mft_no);
            (void)pfs_ntfs_mft_record_free(p_sbi, mft_no, p_rec);
            goto out;
        }
    }

    rc = PFS_NTFS_RET_OK;
    goto out;

fail:
    (void)pfs_ntfs_mft_bitmap_set_bit(p_sbi, mft_no, 0);

out:
    pfs_free(p_rec);
    pfs_ntfs_inode_reset(&parent_ino);
    return rc;
}

int pfs_ntfs_create_file(pfs_ntfs_sbi_t *p_sbi, const char *path)
{
    return pfs_ntfs_create_entry(p_sbi, path, 0);
}

int pfs_ntfs_mkdir(pfs_ntfs_sbi_t *p_sbi, const char *path)
{
    return pfs_ntfs_create_entry(p_sbi, path, 1);
}

static int pfs_ntfs_delete_entry(pfs_ntfs_sbi_t *p_sbi, const char *path, int dir_only)
{
    pfs_ntfs_lookup_t target;
    pfs_ntfs_lookup_t parent_lu;
    pfs_ntfs_inode_t parent_ino;
    pfs_ntfs_inode_t target_ino;
    pfs_ntfs_uni_name_t leaf;
    uint8_t *p_rec = NULL;
    ntfs_mft_record_t *p_mrec;
    uint32_t rec_size;
    uint16_t link_count;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    int rc = pfs_ntfs_lookup_path(p_sbi, path, &target);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (dir_only && !target.is_dir)
    {
        return PFS_NTFS_RET_ENOENT;
    }
    if (!dir_only && target.is_dir)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    rc = pfs_ntfs_path_parent_leaf(p_sbi, path, &parent_lu, &leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    memset(&parent_ino, 0, sizeof(parent_ino));
    memset(&target_ino, 0, sizeof(target_ino));
    rc = pfs_ntfs_inode_load(p_sbi, parent_lu.mft_no, &parent_ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    if (dir_only)
    {
        rc = pfs_ntfs_inode_load(p_sbi, target.mft_no, &target_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_ntfs_inode_reset(&parent_ino);
            return rc;
        }
        rc = pfs_ntfs_index_dir_is_empty(p_sbi, &target_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            pfs_ntfs_inode_reset(&parent_ino);
            pfs_ntfs_inode_reset(&target_ino);
            return rc;
        }
    }
    else
    {
        // 文件: inode 损坏/MFT 已释放时仍须摘掉索引残留, 否则 access 有名但无法 unlink/CREAT
        rc = pfs_ntfs_inode_load(p_sbi, target.mft_no, &target_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            int rc_load = rc;
            // 长名可能有 DOS 短名项, 按 mref 全摘
            int rc_rm = pfs_ntfs_index_remove_by_mref(p_sbi, &parent_ino, target.mft_no);

            pfs_ntfs_inode_reset(&parent_ino);
            if (PFS_NTFS_RET_OK != rc_rm)
            {
                return rc_load;
            }
            // 仅确认无 IN_USE 时清位图; 避免坏 inode 但仍 IN_USE 时清位触发 chkdsk
            if (PFS_NTFS_RET_ENOENT == rc_load &&
                    target.mft_no >= (uint64_t)NTFS_FILE_first_user)
            {
                (void)pfs_ntfs_mft_bitmap_set_bit(p_sbi, target.mft_no, 0);
            }
            return PFS_NTFS_RET_OK;
        }
    }

    // WIN32 + DOS 可能各有一条 $I30
    rc = pfs_ntfs_index_remove_by_mref(p_sbi, &parent_ino, target.mft_no);
    if (PFS_NTFS_RET_OK != rc)
    {
        pfs_ntfs_inode_reset(&parent_ino);
        pfs_ntfs_inode_reset(&target_ino);
        return rc;
    }

    rec_size = p_sbi->bs.mft_record_size;
    p_rec = (uint8_t *)pfs_mallocz(rec_size);
    if (NULL == p_rec)
    {
        pfs_ntfs_inode_reset(&parent_ino);
        pfs_ntfs_inode_reset(&target_ino);
        return PFS_NTFS_RET_ENOMEM;
    }

    rc = pfs_ntfs_mft_read_record(p_sbi, target.mft_no, p_rec);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    p_mrec = (ntfs_mft_record_t *)p_rec;
    link_count = pfs_le16_to_cpu(p_mrec->link_count);
    for (;;)
    {
        rc = pfs_ntfs_mft_attr_remove(p_sbi, p_mrec, rec_size, NTFS_AT_FILE_NAME, NULL, 0, NULL);
        if (PFS_NTFS_RET_ENOENT == rc)
        {
            rc = PFS_NTFS_RET_OK;
            break;
        }
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
        if (link_count > 0U)
        {
            link_count--;
        }
    }
    p_mrec->link_count = pfs_cpu_to_le16(link_count);

    if (0U == link_count)
    {
        if (!dir_only)
        {
            int64_t vcn_count = 0;

            if (target_ino.allocated_size > 0)
            {
                vcn_count = (target_ino.allocated_size + (int64_t)p_sbi->bs.cluster_size - 1) /
                        (int64_t)p_sbi->bs.cluster_size;
            }
            if (vcn_count > 0)
            {
                (void)pfs_ntfs_cluster_free_from_rl(p_sbi, &target_ino.data_runlist, 0, vcn_count);
            }
        }
        else if (target_ino.i30.has_allocation && target_ino.i30.alloc_runlist.count > 0)
        {
            const pfs_ntfs_run_t *p_rl = target_ino.i30.alloc_runlist.p_rl;
            int64_t vcn_count = p_rl[target_ino.i30.alloc_runlist.count].vcn;

            if (vcn_count > 0)
            {
                (void)pfs_ntfs_cluster_free_from_rl(p_sbi, &target_ino.i30.alloc_runlist, 0, vcn_count);
            }
        }
        rc = pfs_ntfs_mft_record_free(p_sbi, target.mft_no, p_rec);
    }
    else
    {
        rc = pfs_ntfs_mft_write_record(p_sbi, target.mft_no, p_rec);
    }

out:
    pfs_free(p_rec);
    pfs_ntfs_inode_reset(&parent_ino);
    pfs_ntfs_inode_reset(&target_ino);
    return rc;
}

int pfs_ntfs_unlink(pfs_ntfs_sbi_t *p_sbi, const char *path)
{
    return pfs_ntfs_delete_entry(p_sbi, path, 0);
}

int pfs_ntfs_rmdir(pfs_ntfs_sbi_t *p_sbi, const char *path)
{
    return pfs_ntfs_delete_entry(p_sbi, path, 1);
}

int pfs_ntfs_rename(pfs_ntfs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    pfs_ntfs_lookup_t old_target;
    pfs_ntfs_lookup_t new_exist;
    pfs_ntfs_lookup_t old_parent;
    pfs_ntfs_lookup_t new_parent;
    pfs_ntfs_inode_t old_dir_ino;
    pfs_ntfs_inode_t new_dir_ino;
    pfs_ntfs_uni_name_t old_leaf;
    pfs_ntfs_uni_name_t new_leaf;
    uint8_t fn_buf[offsetof(ntfs_file_name_attr_t, file_name) + 255U * 2U];
    uint8_t *p_rec = NULL;
    ntfs_file_name_attr_t *p_fn = (ntfs_file_name_attr_t *)fn_buf;
    ntfs_mft_record_t *p_mrec;
    uint64_t new_parent_mref;
    uint64_t now_le;
    uint32_t fn_size;
    uint32_t file_attrs;
    uint32_t rec_size;
    uint16_t link_count;

    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!pfs_ntfs_sbi_can_write(p_sbi))
    {
        return PFS_NTFS_RET_EPERM;
    }

    int rc = pfs_ntfs_lookup_path(p_sbi, oldpath, &old_target);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_ntfs_lookup_path(p_sbi, newpath, &new_exist);
    if (PFS_NTFS_RET_OK == rc)
    {
        return PFS_NTFS_RET_EEXIST;
    }
    if (PFS_NTFS_RET_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_ntfs_path_parent_leaf(p_sbi, oldpath, &old_parent, &old_leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_ntfs_path_parent_leaf(p_sbi, newpath, &new_parent, &new_leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    // 同目录 rename 须共用一份 dir inode; 否则 insert 后用过期副本 remove 会抹掉新名
    {
        int same_dir = (old_parent.mft_no == new_parent.mft_no);

        memset(&old_dir_ino, 0, sizeof(old_dir_ino));
        memset(&new_dir_ino, 0, sizeof(new_dir_ino));
        rc = pfs_ntfs_inode_load(p_sbi, old_parent.mft_no, &old_dir_ino);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        if (!same_dir)
        {
            rc = pfs_ntfs_inode_load(p_sbi, new_parent.mft_no, &new_dir_ino);
            if (PFS_NTFS_RET_OK != rc)
            {
                pfs_ntfs_inode_reset(&old_dir_ino);
                return rc;
            }
        }

        rec_size = p_sbi->bs.mft_record_size;
        p_rec = (uint8_t *)pfs_mallocz(rec_size);
        if (NULL == p_rec)
        {
            pfs_ntfs_inode_reset(&old_dir_ino);
            if (!same_dir)
            {
                pfs_ntfs_inode_reset(&new_dir_ino);
            }
            return PFS_NTFS_RET_ENOMEM;
        }

        rc = pfs_ntfs_mft_read_record(p_sbi, old_target.mft_no, p_rec);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }

        p_mrec = (ntfs_mft_record_t *)p_rec;
        now_le = pfs_ntfs_current_time_le();
        new_parent_mref = NTFS_MK_MREF(new_parent.mft_no, new_parent.seq_no);
        file_attrs = old_target.is_dir ?
                (uint32_t)(NTFS_FILE_ATTR_DIRECTORY | NTFS_FILE_ATTR_DUP_FILE_NAME_INDEX_PRESENT |
                        NTFS_FILE_ATTR_ARCHIVE) :
                (uint32_t)NTFS_FILE_ATTR_ARCHIVE;

        rc = pfs_ntfs_build_file_name_attr(p_sbi, new_parent_mref, &new_leaf, file_attrs, now_le, 0, 0,
                pfs_ntfs_uni_is_dos_compatible(&new_leaf) ? NTFS_FILE_NAME_WIN32_AND_DOS :
                        NTFS_FILE_NAME_WIN32,
                p_fn, (uint32_t)sizeof(fn_buf), &fn_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }

        rc = pfs_ntfs_mft_add_resident(p_mrec, rec_size, NTFS_AT_FILE_NAME, NULL, 0, fn_buf, fn_size);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }

        link_count = pfs_le16_to_cpu(p_mrec->link_count);
        link_count++;
        p_mrec->link_count = pfs_cpu_to_le16(link_count);

        rc = pfs_ntfs_index_insert_filename(p_sbi, same_dir ? &old_dir_ino : &new_dir_ino, p_fn,
                NTFS_MK_MREF(old_target.mft_no, old_target.seq_no));
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }

        rc = pfs_ntfs_index_remove_filename(p_sbi, &old_dir_ino, &old_leaf);
        if (PFS_NTFS_RET_OK != rc)
        {
            goto out;
        }
    }

    rc = pfs_ntfs_mft_attr_remove(p_sbi, p_mrec, rec_size, NTFS_AT_FILE_NAME, NULL, 0, &old_leaf);
    if (PFS_NTFS_RET_OK != rc)
    {
        goto out;
    }

    link_count = pfs_le16_to_cpu(p_mrec->link_count);
    if (link_count > 0U)
    {
        link_count--;
    }
    p_mrec->link_count = pfs_cpu_to_le16(link_count);

    rc = pfs_ntfs_mft_write_record(p_sbi, old_target.mft_no, p_rec);

out:
    pfs_free(p_rec);
    pfs_ntfs_inode_reset(&old_dir_ino);
    pfs_ntfs_inode_reset(&new_dir_ino);
    if (PFS_NTFS_RET_OK == rc)
    {
        if (NTFS_FILE_root == old_parent.mft_no || NTFS_FILE_root == new_parent.mft_no)
        {
            (void)pfs_ntfs_root_reload(p_sbi);
        }
    }
    return rc;
}

// end
