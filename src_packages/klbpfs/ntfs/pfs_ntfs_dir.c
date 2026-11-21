// Doc Encode : UTF-8 BOM, Unix(LF)
// 目录枚举 (linux-7.1 fs/ntfs/dir.c)
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_mem.h"
#include "pfs.h"

#include <string.h>

void pfs_ntfs_dir_meta_reset(pfs_ntfs_dir_meta_t *p_meta)
{
    if (NULL == p_meta)
    {
        return;
    }
    if (NULL != p_meta->p_block)
    {
        pfs_free(p_meta->p_block);
        p_meta->p_block = NULL;
    }
    pfs_ntfs_inode_reset(&p_meta->ino);
    memset(p_meta, 0, sizeof(*p_meta));
    p_meta->in_root = true;
    p_meta->block_vcn = -1;
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

static int pfs_ntfs_readdir_skip_name(const ntfs_file_name_attr_t *p_fn)
{

    if (NTFS_FILE_NAME_DOS == p_fn->file_name_type)
    {
        return 1;
    }
    if (1U == p_fn->file_name_length && '.' == pfs_le16_to_cpu(p_fn->file_name[0]))
    {
        return 1;
    }
    if (2U == p_fn->file_name_length &&
            '.' == pfs_le16_to_cpu(p_fn->file_name[0]) &&
            '.' == pfs_le16_to_cpu(p_fn->file_name[1]))
    {
        return 1;
    }
    for (uint32_t i = 0; i < (uint32_t)p_fn->file_name_length; i++)
    {
        if (0U == pfs_le16_to_cpu(p_fn->file_name[i]))
        {
            return 1;
        }
    }
    return 0;
}

static int pfs_ntfs_readdir_emit(const ntfs_index_entry_t *p_ent, uint8_t *p_d_type,
        char *utf8_name, int name_cap)
{
    const ntfs_file_name_attr_t *p_fn = &p_ent->key.file_name;
    uint16_t host_name[NTFS_MAXIMUM_FILE_NAME_LENGTH];

    if (pfs_ntfs_readdir_skip_name(p_fn))
    {
        return PFS_NTFS_RET_OK;
    }

    for (uint32_t i = 0; i < (uint32_t)p_fn->file_name_length; i++)
    {
        host_name[i] = pfs_le16_to_cpu(p_fn->file_name[i]);
    }

    int rc = pfs_ntfs_utf16_to_utf8(host_name, (uint32_t)p_fn->file_name_length, utf8_name, name_cap);
    if (rc < 0)
    {
        return rc;
    }
    if ('\0' == utf8_name[0])
    {
        utf8_name[0] = '\0';
        return PFS_NTFS_RET_OK;
    }

    if (NULL != p_d_type)
    {
        if (0U != (pfs_le32_to_cpu(p_fn->file_attributes) & pfs_le32_to_cpu(NTFS_FILE_ATTR_DIRECTORY)))
        {
            *p_d_type = PFS_DT_DIR;
        }
        else
        {
            *p_d_type = PFS_DT_REG;
        }
    }
    return 1;
}

static int pfs_ntfs_readdir_scan_ih(pfs_ntfs_dir_meta_t *p_meta, ntfs_index_header_t *p_ih,
        uint8_t *p_d_type, char *utf8_name, int name_cap)
{
    uint32_t entries_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
    uint32_t index_len = pfs_le32_to_cpu(p_ih->index_length);

    while (p_meta->entry_ofs + sizeof(ntfs_index_entry_header_t) <= index_len)
    {
        ntfs_index_entry_header_t *p_ie = (ntfs_index_entry_header_t *)((uint8_t *)p_ih + p_meta->entry_ofs);

        if (pfs_ntfs_index_entry_is_end(p_ie))
        {
            p_meta->entry_ofs = index_len;
            return 0;
        }

        {
            const ntfs_index_entry_t *p_ent = (const ntfs_index_entry_t *)p_ie;

            int emit = pfs_ntfs_readdir_emit(p_ent, p_d_type, utf8_name, name_cap);
            p_meta->entry_ofs += pfs_le16_to_cpu(p_ie->length);
            if (emit > 0)
            {
                return 1;
            }
            if (emit < 0)
            {
                return emit;
            }
        }

        if (0U == pfs_le16_to_cpu(p_ie->length))
        {
            break;
        }
    }

    return 0;
}

int pfs_ntfs_dir_open(pfs_ntfs_sbi_t *p_sbi, const char *path, pfs_ntfs_dir_meta_t *p_meta)
{
    pfs_ntfs_lookup_t lu;

    if (NULL == p_sbi || NULL == path || NULL == p_meta)
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_dir_meta_reset(p_meta);
    int rc = pfs_ntfs_lookup_path(p_sbi, path, &lu);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (!lu.is_dir)
    {
        return PFS_NTFS_RET_ENOENT;
    }

    rc = pfs_ntfs_inode_load(p_sbi, lu.mft_no, &p_meta->ino);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }

    {
        ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_meta->ino.i30.p_root;

        if (NULL != p_root)
        {
            p_meta->entry_ofs = pfs_le32_to_cpu(p_root->index.entries_offset);
        }
    }

    p_meta->valid = true;
    p_meta->in_root = true;
    p_meta->block_vcn = -1;
    p_meta->eof = false;
    return PFS_NTFS_RET_OK;
}

int pfs_ntfs_readdir(pfs_ntfs_sbi_t *p_sbi, pfs_ntfs_dir_meta_t *p_meta, uint8_t *p_d_type,
        char *utf8_name, int name_cap)
{
    pfs_ntfs_index_t *p_idx;
    uint32_t vcn_per_block;
    int found;

    if (NULL == p_sbi || NULL == p_meta || NULL == utf8_name || name_cap <= 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    if (!p_meta->valid || p_meta->eof)
    {
        utf8_name[0] = '\0';
        return PFS_NTFS_RET_OK;
    }

    p_idx = &p_meta->ino.i30;
    if (!p_idx->valid)
    {
        return PFS_NTFS_RET_EBADMSG;
    }

    // INDX VCN 单位为簇或扇区; 每块步进 = block_size / vcn_size (如 4K/512=8)
    vcn_per_block = 1U;
    if (0U != p_idx->vcn_size && p_idx->block_size >= p_idx->vcn_size)
    {
        vcn_per_block = p_idx->block_size / p_idx->vcn_size;
    }

    if (p_meta->in_root)
    {
        ntfs_index_root_t *p_root = (ntfs_index_root_t *)p_idx->p_root;

        if (NULL == p_root)
        {
            return PFS_NTFS_RET_EBADMSG;
        }
        found = pfs_ntfs_readdir_scan_ih(p_meta, &p_root->index, p_d_type, utf8_name, name_cap);
        if (found > 0)
        {
            return PFS_NTFS_RET_OK;
        }
        if (found < 0)
        {
            return found;
        }

        p_meta->in_root = false;
        p_meta->block_vcn = 0;
        p_meta->entry_ofs = 0;
        if (!p_idx->has_allocation)
        {
            p_meta->eof = true;
            utf8_name[0] = '\0';
            return PFS_NTFS_RET_OK;
        }
    }

    while (!p_meta->eof)
    {
        ntfs_index_block_t *p_ib;
        ntfs_index_header_t *p_ih;
        int64_t lcn;

        if (NULL == p_meta->p_block)
        {
            // 跳过洞 / 未映射, 直到越界
            for (;;)
            {
                lcn = pfs_ntfs_runlist_vcn_to_lcn(&p_idx->alloc_runlist, p_meta->block_vcn);
                if (PFS_NTFS_LCN_ENOENT == lcn || PFS_NTFS_LCN_RL_NOT_MAPPED == lcn)
                {
                    p_meta->eof = true;
                    utf8_name[0] = '\0';
                    return PFS_NTFS_RET_OK;
                }
                if (PFS_NTFS_LCN_HOLE == lcn)
                {
                    p_meta->block_vcn += (int64_t)vcn_per_block;
                    continue;
                }
                if (lcn < 0)
                {
                    p_meta->eof = true;
                    utf8_name[0] = '\0';
                    return PFS_NTFS_RET_OK;
                }
                break;
            }

            p_meta->p_block = (uint8_t *)pfs_malloc(p_idx->block_size);
            if (NULL == p_meta->p_block)
            {
                return PFS_NTFS_RET_ENOMEM;
            }
            int rc = pfs_ntfs_index_read_block(p_sbi, p_idx, p_meta->block_vcn, p_meta->p_block);
            if (PFS_NTFS_RET_OK != rc)
            {
                // 错位/损坏块: 按整块步进继续扫, 勿直接 EOF (否则漏后续叶)
                pfs_free(p_meta->p_block);
                p_meta->p_block = NULL;
                p_meta->block_vcn += (int64_t)vcn_per_block;
                p_meta->entry_ofs = 0;
                continue;
            }
            p_ib = (ntfs_index_block_t *)p_meta->p_block;
            p_ih = &p_ib->index;
            if (0U != (p_ih->flags & (uint8_t)NTFS_INDEX_NODE))
            {
                p_meta->entry_ofs = 0;
            }
            else
            {
                p_meta->entry_ofs = pfs_le32_to_cpu(p_ih->entries_offset);
            }
        }
        else
        {
            p_ib = (ntfs_index_block_t *)p_meta->p_block;
            p_ih = &p_ib->index;
        }

        if (0U == (p_ih->flags & (uint8_t)NTFS_INDEX_NODE))
        {
            found = pfs_ntfs_readdir_scan_ih(p_meta, p_ih, p_d_type, utf8_name, name_cap);
            if (found > 0)
            {
                return PFS_NTFS_RET_OK;
            }
            if (found < 0)
            {
                return found;
            }
        }

        pfs_free(p_meta->p_block);
        p_meta->p_block = NULL;
        p_meta->block_vcn += (int64_t)vcn_per_block;
        p_meta->entry_ofs = 0;
    }

    utf8_name[0] = '\0';
    return PFS_NTFS_RET_OK;
}

// end
