// Doc Encode : UTF-8 BOM, Unix(LF)
// 目录项 (linux-7.1 fs/exfat/dir.c)
#include "exfat/pfs_exfat_internal.h"

#include <string.h>

static int pfs_exfat_read_dentry_local(pfs_exfat_sbi_t *p_sbi, uint32_t clu, uint32_t local,
        exfat_dentry_t *p_out)
{
    if (NULL == p_sbi || NULL == p_out)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (local >= p_bs->dentries_per_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_cache_cluster_slot_t *p_slot = pfs_exfat_dir_cache_get(&p_sbi->dir_cache, clu);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_EXFAT_RET_EIO;
    }

    const exfat_dentry_t *p_ents = (const exfat_dentry_t *)p_slot->p_data;
    memcpy(p_out, &p_ents[local], sizeof(*p_out));
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_read_dentry_at(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, exfat_dentry_t *p_out)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_out)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    pfs_exfat_chain_t clu;
    pfs_exfat_chain_dup(&clu, p_dir);

    uint32_t byte_off = (uint32_t)PFS_EXFAT_DEN_TO_B(abs_entry);
    uint32_t clu_off = PFS_EXFAT_B_TO_CLU(byte_off, p_bs);
    uint32_t clu_id = clu.dir;
    int rc = pfs_exfat_cluster_walk(p_sbi, &clu_id, clu_off, clu.flags);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_EOF_CLUSTER == clu_id)
    {
        return PFS_EXFAT_RET_EIO;
    }

    byte_off &= (p_bs->cluster_size - 1U);
    uint32_t local = byte_off >> EXFAT_DENTRY_SIZE_BITS;
    return pfs_exfat_read_dentry_local(p_sbi, clu_id, local, p_out);
}

static int pfs_exfat_flush_dir_cluster(pfs_exfat_sbi_t *p_sbi, uint32_t clu_id,
        const uint8_t *p_data)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    int64_t off = pfs_exfat_cluster_to_byte(p_sbi, clu_id);
    int rc = pfs_blkio_pwrite(p_sbi->p_blkio, p_data, (int)p_bs->cluster_size, off);
    if (0 > rc || rc != (int)p_bs->cluster_size)
    {
        return PFS_EXFAT_RET_EIO;
    }
    pfs_exfat_dir_cache_invalidate(&p_sbi->dir_cache, clu_id);
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_write_dentry_at(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, const exfat_dentry_t *p_in)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_in || NULL == p_sbi->p_blkio)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    pfs_exfat_chain_t clu;
    pfs_exfat_chain_dup(&clu, p_dir);

    uint32_t byte_off = (uint32_t)PFS_EXFAT_DEN_TO_B(abs_entry);
    uint32_t clu_off = PFS_EXFAT_B_TO_CLU(byte_off, p_bs);
    uint32_t clu_id = clu.dir;
    int rc = pfs_exfat_cluster_walk(p_sbi, &clu_id, clu_off, clu.flags);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_EOF_CLUSTER == clu_id || !pfs_exfat_is_valid_cluster(p_sbi, clu_id))
    {
        return PFS_EXFAT_RET_EIO;
    }

    byte_off &= (p_bs->cluster_size - 1U);
    uint32_t local = byte_off >> EXFAT_DENTRY_SIZE_BITS;
    if (local >= p_bs->dentries_per_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_cache_cluster_slot_t *p_slot = pfs_exfat_dir_cache_get(&p_sbi->dir_cache, clu_id);
    if (NULL == p_slot || NULL == p_slot->p_data)
    {
        return PFS_EXFAT_RET_EIO;
    }
    exfat_dentry_t *p_ents = (exfat_dentry_t *)p_slot->p_data;
    memcpy(&p_ents[local], p_in, sizeof(*p_in));
    return pfs_exfat_flush_dir_cluster(p_sbi, clu_id, p_slot->p_data);
}

static int pfs_exfat_extract_uni_name(const exfat_dentry_t *p_ep, uint16_t *p_uniname)
{
    int len = 0;

    for (int i = 0; i < EXFAT_FILE_NAME_LEN; i++)
    {
        uint16_t ch = pfs_le16_to_cpu(p_ep->dentry.name.unicode_0_14[i]);

        p_uniname[i] = ch;
        if (0x0 == ch)
        {
            return len;
        }
        len++;
    }

    p_uniname[EXFAT_FILE_NAME_LEN] = 0x0;
    return len;
}

unsigned int pfs_exfat_get_entry_type(const exfat_dentry_t *p_entry)
{
    if (NULL == p_entry)
    {
        return PFS_EXFAT_TYPE_INVALID;
    }

    uint8_t type = p_entry->type;
    if (EXFAT_UNUSED == type)
    {
        return PFS_EXFAT_TYPE_UNUSED;
    }
    if (IS_EXFAT_DELETED(type))
    {
        return PFS_EXFAT_TYPE_DELETED;
    }
    if (EXFAT_INVAL == type)
    {
        return PFS_EXFAT_TYPE_INVALID;
    }
    if (EXFAT_BITMAP == type)
    {
        return PFS_EXFAT_TYPE_BITMAP;
    }
    if (EXFAT_UPCASE == type)
    {
        return PFS_EXFAT_TYPE_UPCASE;
    }
    if (EXFAT_VOLUME == type)
    {
        return PFS_EXFAT_TYPE_VOLUME;
    }
    if (EXFAT_FILE == type)
    {
        // attr 盘上为小端
        if (EXFAT_ATTR_SUBDIR & pfs_le16_to_cpu(p_entry->dentry.file.attr))
        {
            return PFS_EXFAT_TYPE_DIR;
        }
        return PFS_EXFAT_TYPE_FILE;
    }
    if (EXFAT_STREAM == type)
    {
        return PFS_EXFAT_TYPE_STREAM;
    }
    if (EXFAT_NAME == type)
    {
        return PFS_EXFAT_TYPE_EXTEND;
    }
    return PFS_EXFAT_TYPE_INVALID;
}

int pfs_exfat_get_uniname_from_ext_entry(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_clu,
        int local_entry, uint16_t *uniname)
{
    if (NULL == p_sbi || NULL == p_clu || NULL == uniname)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    exfat_dentry_t file_ent;
    int rc = pfs_exfat_read_dentry_local(p_sbi, p_clu->dir, (uint32_t)local_entry, &file_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    uint32_t num_ext = file_ent.dentry.file.num_ext;
    uint16_t *p_uni = uniname;
    uint32_t uni_len = 0;
    uint32_t clu_id = p_clu->dir;
    uint32_t idx = (uint32_t)local_entry;

    // 对齐 linux: 从 FIRST_FILENAME(2) 起拼名; stream(1) 跳过; 遇非 NAME 结束
    for (uint32_t order = PFS_EXFAT_ES_IDX_FIRST_FILENAME; order <= num_ext; order++)
    {
        exfat_dentry_t ext_ent;
        idx = (uint32_t)local_entry + order;
        clu_id = p_clu->dir;
        while (idx >= p_bs->dentries_per_clu)
        {
            idx -= p_bs->dentries_per_clu;
            rc = pfs_exfat_cluster_walk(p_sbi, &clu_id, 1, p_clu->flags);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
            if (EXFAT_EOF_CLUSTER == clu_id)
            {
                return PFS_EXFAT_RET_EIO;
            }
        }

        rc = pfs_exfat_read_dentry_local(p_sbi, clu_id, idx, &ext_ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }

        unsigned int type = pfs_exfat_get_entry_type(&ext_ent);
        if (PFS_EXFAT_TYPE_EXTEND != type)
        {
            break;
        }

        int len = pfs_exfat_extract_uni_name(&ext_ent, p_uni);
        uni_len += (uint32_t)len;
        if (len != EXFAT_FILE_NAME_LEN || uni_len >= PFS_EXFAT_MAX_NAME_LENGTH)
        {
            break;
        }
        p_uni += EXFAT_FILE_NAME_LEN;
    }

    if (uni_len > PFS_EXFAT_MAX_NAME_LENGTH)
    {
        uni_len = PFS_EXFAT_MAX_NAME_LENGTH;
    }
    // 须写在已收集长度之后; 不可 *p_uni=0 (短名时 p_uni 仍指向首字符, 会清空整名)
    uniname[uni_len] = 0x0;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_load_dir_entry_info(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        int entry, pfs_exfat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_info)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    exfat_dentry_t file_ent;
    int rc = pfs_exfat_read_dentry_at(p_sbi, p_dir, (uint32_t)entry, &file_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    exfat_dentry_t stream_ent;
    rc = pfs_exfat_read_dentry_at(p_sbi, p_dir, (uint32_t)entry + 1U, &stream_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    p_info->dir = *p_dir;
    p_info->entry = entry;
    p_info->type = pfs_exfat_get_entry_type(&file_ent);
    p_info->attr = pfs_le16_to_cpu(file_ent.dentry.file.attr);
    p_info->valid_size = (int64_t)pfs_le64_to_cpu(stream_ent.dentry.stream.valid_size);
    p_info->size = (int64_t)pfs_le64_to_cpu(stream_ent.dentry.stream.size);
    p_info->start_clu = pfs_le32_to_cpu(stream_ent.dentry.stream.start_clu);

    if (!pfs_exfat_is_valid_cluster(p_sbi, p_info->start_clu) && p_info->size > 0)
    {
        p_info->size = 0;
        p_info->valid_size = 0;
    }

    if (p_info->valid_size > p_info->size)
    {
        p_info->valid_size = p_info->size;
    }

    if (0 == p_info->size)
    {
        p_info->flags = EXFAT_ALLOC_NO_FAT_CHAIN;
        p_info->start_clu = EXFAT_EOF_CLUSTER;
    }
    else
    {
        p_info->flags = stream_ent.dentry.stream.flags;
    }

    if (p_info->valid_size < 0)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    if (PFS_EXFAT_B_TO_CLU((uint64_t)p_info->size, p_bs) > p_bs->num_clusters)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    p_info->num_subdirs = 0;
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_readdir(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir_chain,
        uint8_t dir_flags_unused, int64_t *p_cpos, pfs_exfat_dir_entry_t *p_de,
        char *utf8_name, int utf8_cap)
{
    (void)dir_flags_unused;

    if (NULL == p_sbi || NULL == p_dir_chain || NULL == p_cpos || NULL == p_de ||
            NULL == utf8_name || 0 >= utf8_cap)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    uint32_t dentries_per_clu = p_bs->dentries_per_clu;
    uint32_t max_dentries = PFS_EXFAT_CLU_TO_DEN(p_bs->num_clusters, p_bs);
    if (max_dentries > PFS_EXFAT_MAX_DENTRIES)
    {
        max_dentries = PFS_EXFAT_MAX_DENTRIES;
    }

    uint32_t dentry = PFS_EXFAT_B_TO_DEN(*p_cpos);
    pfs_exfat_chain_t clu;
    pfs_exfat_chain_dup(&clu, p_dir_chain);

    uint32_t clu_offset = PFS_EXFAT_DEN_TO_CLU(dentry, p_bs);
    int rc = pfs_exfat_chain_advance(p_sbi, &clu, clu_offset);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    utf8_name[0] = '\0';

    exfat_dentry_t ent;
    uint16_t uni_buf[PFS_EXFAT_MAX_NAME_LENGTH + 3];

    while (EXFAT_EOF_CLUSTER != clu.dir && dentry < max_dentries)
    {
        uint32_t i = dentry & (dentries_per_clu - 1U);

        for (; i < dentries_per_clu; i++, dentry++)
        {
            rc = pfs_exfat_read_dentry_local(p_sbi, clu.dir, i, &ent);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }

            unsigned int type = pfs_exfat_get_entry_type(&ent);
            if (PFS_EXFAT_TYPE_UNUSED == type)
            {
                *p_cpos = (int64_t)PFS_EXFAT_DEN_TO_B(dentry);
                return PFS_EXFAT_RET_OK;
            }

            if (PFS_EXFAT_TYPE_FILE != type && PFS_EXFAT_TYPE_DIR != type)
            {
                continue;
            }

            uint32_t num_ext = ent.dentry.file.num_ext;
            p_de->attr = pfs_le16_to_cpu(ent.dentry.file.attr);
            p_de->type = type;

            rc = pfs_exfat_get_uniname_from_ext_entry(p_sbi, &clu, (int)i, uni_buf);
            if (PFS_EXFAT_RET_OK != rc)
            {
                continue;
            }

            rc = pfs_exfat_utf16_to_utf8(uni_buf, PFS_EXFAT_MAX_NAME_LENGTH, utf8_name, utf8_cap);
            if (0 > rc)
            {
                continue;
            }

            rc = pfs_exfat_load_dir_entry_info(p_sbi, p_dir_chain, (int)dentry, p_de);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }

            p_de->dir = clu;
            p_de->entry = (int)i;

            *p_cpos = (int64_t)PFS_EXFAT_DEN_TO_B(dentry + 1U + num_ext);
            return PFS_EXFAT_RET_OK;
        }

        rc = pfs_exfat_chain_advance(p_sbi, &clu, 1);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }

    utf8_name[0] = '\0';
    *p_cpos = (int64_t)PFS_EXFAT_DEN_TO_B(dentry);
    return PFS_EXFAT_RET_OK;
}

// end
