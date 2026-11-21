// Doc Encode : UTF-8 BOM, Unix(LF)
// 路径与 lookup/create/mkdir/unlink/rmdir/rename (linux-7.1 fs/exfat/namei.c)
#include "exfat/pfs_exfat_internal.h"
#include "pfs_compiler.h"

#include <stdbool.h>
#include <string.h>

static void pfs_exfat_fill_root_info(pfs_exfat_sbi_t *p_sbi, pfs_exfat_dir_entry_t *p_info)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    memset(p_info, 0, sizeof(*p_info));
    pfs_exfat_chain_set(&p_info->dir, p_bs->root_dir_clu, p_bs->num_clusters, EXFAT_ALLOC_FAT_CHAIN);
    p_info->type = PFS_EXFAT_TYPE_DIR;
    p_info->start_clu = p_bs->root_dir_clu;
    p_info->flags = EXFAT_ALLOC_FAT_CHAIN;
    p_info->attr = EXFAT_ATTR_SUBDIR;
    p_info->size = 0;
    p_info->valid_size = 0;
}

static bool pfs_exfat_name_matches(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, const pfs_exfat_uni_name_t *p_uni, uint16_t *p_tmp_name)
{
    exfat_dentry_t stream_ent = {0};
    int rc = pfs_exfat_read_dentry_at(p_sbi, p_dir, abs_entry + 1U, &stream_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return false;
    }
    if (PFS_EXFAT_TYPE_STREAM != pfs_exfat_get_entry_type(&stream_ent))
    {
        return false;
    }

    uint16_t name_hash = pfs_le16_to_cpu(stream_ent.dentry.stream.name_hash);
    uint8_t name_len = stream_ent.dentry.stream.name_len;
    if (p_uni->name_hash != name_hash || p_uni->name_len != name_len)
    {
        return false;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    pfs_exfat_chain_t at_clu;
    pfs_exfat_chain_dup(&at_clu, p_dir);
    rc = pfs_exfat_chain_advance(p_sbi, &at_clu, PFS_EXFAT_DEN_TO_CLU(abs_entry, p_bs));
    if (PFS_EXFAT_RET_OK != rc)
    {
        return false;
    }
    uint32_t byte_off = (uint32_t)PFS_EXFAT_DEN_TO_B(abs_entry) & (p_bs->cluster_size - 1U);
    uint32_t local = byte_off >> EXFAT_DENTRY_SIZE_BITS;

    rc = pfs_exfat_get_uniname_from_ext_entry(p_sbi, &at_clu, (int)local, p_tmp_name);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return false;
    }

    if (0 != pfs_exfat_uniname_ncmp(p_sbi, p_tmp_name, p_uni->name, name_len))
    {
        return false;
    }

    return true;
}

int pfs_exfat_find_dir_entry(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        const pfs_exfat_uni_name_t *p_uni, pfs_exfat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_uni || NULL == p_info)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (0 == p_uni->name_len)
    {
        return PFS_EXFAT_RET_ENOENT;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    uint32_t max_dentries = PFS_EXFAT_CLU_TO_DEN(p_bs->num_clusters, p_bs);
    if (max_dentries > PFS_EXFAT_MAX_DENTRIES)
    {
        max_dentries = PFS_EXFAT_MAX_DENTRIES;
    }

    uint16_t tmp_name[PFS_EXFAT_MAX_NAME_LENGTH + 3];
    for (uint32_t dentry = 0; dentry < max_dentries; )
    {
        exfat_dentry_t ent;

        int rc = pfs_exfat_read_dentry_at(p_sbi, p_dir, dentry, &ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }

        unsigned int type = pfs_exfat_get_entry_type(&ent);
        if (PFS_EXFAT_TYPE_UNUSED == type)
        {
            return PFS_EXFAT_RET_ENOENT;
        }

        if (PFS_EXFAT_TYPE_FILE != type && PFS_EXFAT_TYPE_DIR != type)
        {
            dentry++;
            continue;
        }

        uint32_t num_ext = ent.dentry.file.num_ext;
        if (!pfs_exfat_name_matches(p_sbi, p_dir, dentry, p_uni, tmp_name))
        {
            dentry += 1U + num_ext;
            continue;
        }

        // 保持父目录完整簇链 + 绝对项索引 (与 create/load 一致, 便于 rename/回写)
        return pfs_exfat_load_dir_entry_info(p_sbi, p_dir, (int)dentry, p_info);
    }

    return PFS_EXFAT_RET_ENOENT;
}

int pfs_exfat_lookup_path(pfs_exfat_sbi_t *p_sbi, const char *path, pfs_exfat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == path || NULL == p_info)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    pfs_exfat_chain_t dir_chain;
    pfs_exfat_dir_entry_t cur;
    pfs_exfat_uni_name_t uni;
    char comp[PFS_EXFAT_MAX_NAME_LENGTH + 1];
    const char *p = path;
    bool have_cur = false;

    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        pfs_exfat_fill_root_info(p_sbi, p_info);
        return PFS_EXFAT_RET_OK;
    }

    pfs_exfat_chain_set(&dir_chain, p_bs->root_dir_clu, p_bs->num_clusters, EXFAT_ALLOC_FAT_CHAIN);

    while ('\0' != *p)
    {
        int comp_len = 0;
        while ('\0' != *p && '/' != *p)
        {
            if (comp_len >= PFS_EXFAT_MAX_NAME_LENGTH)
            {
                return PFS_EXFAT_RET_ENAMETOOLONG;
            }
            comp[comp_len++] = *p++;
        }
        comp[comp_len] = '\0';

        while ('/' == *p)
        {
            p++;
        }

        bool is_last = ('\0' == *p);

        if (0 == comp_len || (1 == comp_len && '.' == comp[0]))
        {
            if (is_last)
            {
                if (have_cur)
                {
                    *p_info = cur;
                }
                else
                {
                    pfs_exfat_fill_root_info(p_sbi, p_info);
                }
                return PFS_EXFAT_RET_OK;
            }
            continue;
        }
        if (2 == comp_len && '.' == comp[0] && '.' == comp[1])
        {
            return PFS_EXFAT_RET_EOPNOTSUPP;
        }

        int rc = pfs_exfat_utf8_to_uniname(p_sbi, comp, comp_len, &uni);
        if (0 > rc)
        {
            return rc;
        }

        rc = pfs_exfat_find_dir_entry(p_sbi, &dir_chain, &uni, &cur);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        have_cur = true;

        if (is_last)
        {
            *p_info = cur;
            return PFS_EXFAT_RET_OK;
        }

        if (PFS_EXFAT_TYPE_DIR != cur.type)
        {
            return PFS_EXFAT_RET_ENOENT;
        }

        if (0 == cur.size)
        {
            pfs_exfat_chain_set(&dir_chain, cur.start_clu, 0, cur.flags);
        }
        else
        {
            pfs_exfat_chain_set(&dir_chain, cur.start_clu,
                    PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)cur.size, p_bs), cur.flags);
        }
    }

    pfs_exfat_fill_root_info(p_sbi, p_info);
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_calc_num_entries(const pfs_exfat_uni_name_t *p_uni)
{
    if (NULL == p_uni || 0 == p_uni->name_len)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return 2 + (int)PFS_EXFAT_FILENAME_ENTRY_NUM(p_uni->name_len);
}

// 在已分配目录簇内找连续空槽; 不扩展目录 (满则 ENOSPC)
// skip_start/skip_count: 保留区间 (如 rename 源项), skip_count==0 表示无保留
static int pfs_exfat_search_empty_slot(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        int num_entries, uint32_t skip_start, uint32_t skip_count, uint32_t *p_abs_entry)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == p_abs_entry || num_entries <= 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (0 == p_dir->size)
    {
        return PFS_EXFAT_RET_ENOSPC;
    }
    uint32_t total = PFS_EXFAT_CLU_TO_DEN(p_dir->size, p_bs);
    if (total > PFS_EXFAT_MAX_DENTRIES)
    {
        total = PFS_EXFAT_MAX_DENTRIES;
    }

    uint32_t run_start = 0;
    int run_len = 0;
    for (uint32_t dentry = 0; dentry < total; dentry++)
    {
        exfat_dentry_t ent;
        int in_skip = 0;
        if (skip_count > 0 && dentry >= skip_start && dentry < skip_start + skip_count)
        {
            in_skip = 1;
        }

        if (in_skip)
        {
            run_len = 0;
            continue;
        }

        int rc = pfs_exfat_read_dentry_at(p_sbi, p_dir, dentry, &ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        unsigned int type = pfs_exfat_get_entry_type(&ent);
        if (PFS_EXFAT_TYPE_UNUSED == type)
        {
            if (0 == run_len)
            {
                run_start = dentry;
            }
            if ((int)(total - run_start) >= num_entries)
            {
                *p_abs_entry = run_start;
                return PFS_EXFAT_RET_OK;
            }
            return PFS_EXFAT_RET_ENOSPC;
        }
        if (PFS_EXFAT_TYPE_DELETED == type)
        {
            if (0 == run_len)
            {
                run_start = dentry;
            }
            run_len++;
            if (run_len >= num_entries)
            {
                *p_abs_entry = run_start;
                return PFS_EXFAT_RET_OK;
            }
            continue;
        }
        run_len = 0;
        if (PFS_EXFAT_TYPE_FILE == type || PFS_EXFAT_TYPE_DIR == type)
        {
            dentry += (uint32_t)ent.dentry.file.num_ext;
        }
    }
    return PFS_EXFAT_RET_ENOSPC;
}

static void pfs_exfat_init_name_entry(exfat_dentry_t *p_ep, const uint16_t *uniname)
{

    memset(p_ep, 0, sizeof(*p_ep));
    p_ep->type = EXFAT_NAME;
    for (int i = 0; i < EXFAT_FILE_NAME_LEN; i++)
    {
        if (0 != uniname[i])
        {
            p_ep->dentry.name.unicode_0_14[i] = pfs_cpu_to_le16(uniname[i]);
        }
        else
        {
            p_ep->dentry.name.unicode_0_14[i] = 0;
        }
    }
}

static int pfs_exfat_write_empty_file_set(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, int num_entries, const pfs_exfat_uni_name_t *p_uni)
{
    exfat_dentry_t ents[2 + ((PFS_EXFAT_MAX_NAME_LENGTH + EXFAT_FILE_NAME_LEN - 1) /
            EXFAT_FILE_NAME_LEN)];
    const uint16_t *uniname;
    uint16_t chksum;
    int chksum_type;

    if (num_entries < 3 || num_entries > (int)pfs_array_size(ents))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    memset(ents, 0, sizeof(ents[0]) * (size_t)num_entries);

    ents[0].type = EXFAT_FILE;
    ents[0].dentry.file.num_ext = (uint8_t)(num_entries - 1);
    ents[0].dentry.file.attr = pfs_cpu_to_le16(EXFAT_ATTR_ARCHIVE);

    ents[1].type = EXFAT_STREAM;
    ents[1].dentry.stream.flags = EXFAT_ALLOC_FAT_CHAIN;
    ents[1].dentry.stream.name_len = p_uni->name_len;
    ents[1].dentry.stream.name_hash = pfs_cpu_to_le16(p_uni->name_hash);
    ents[1].dentry.stream.start_clu = pfs_cpu_to_le32(EXFAT_FREE_CLUSTER);
    ents[1].dentry.stream.valid_size = pfs_cpu_to_le64(0);
    ents[1].dentry.stream.size = pfs_cpu_to_le64(0);

    uniname = p_uni->name;
    for (int i = 2; i < num_entries; i++)
    {
        pfs_exfat_init_name_entry(&ents[i], uniname);
        uniname += EXFAT_FILE_NAME_LEN;
    }

    chksum = 0;
    chksum_type = EXFAT_CS_DIR_ENTRY;
    for (int i = 0; i < num_entries; i++)
    {
        chksum = pfs_exfat_calc_chksum16_type(&ents[i], (int)sizeof(ents[i]), chksum, chksum_type);
        chksum_type = EXFAT_CS_DEFAULT;
    }
    ents[0].dentry.file.checksum = pfs_cpu_to_le16(chksum);

    for (int i = 0; i < num_entries; i++)
    {
        int rc = pfs_exfat_write_dentry_at(p_sbi, p_dir, abs_entry + (uint32_t)i, &ents[i]);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

int pfs_exfat_create_file(pfs_exfat_sbi_t *p_sbi, const char *path, pfs_exfat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == path || NULL == p_info)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    pfs_exfat_chain_t parent;
    pfs_exfat_dir_entry_t parent_info;
    pfs_exfat_dir_entry_t exist;
    pfs_exfat_uni_name_t uni;
    char leaf[PFS_EXFAT_MAX_NAME_LENGTH + 1];
    char parent_path[512];
    const char *p = path;
    const char *leaf_start;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    leaf_start = p;
    while ('\0' != *p)
    {
        const char *slash = p;

        while ('\0' != *slash && '/' != *slash)
        {
            slash++;
        }
        if ('\0' == *slash)
        {
            leaf_start = p;
            break;
        }
        p = slash;
        while ('/' == *p)
        {
            p++;
        }
        if ('\0' == *p)
        {
            // 尾部斜杠
            return PFS_EXFAT_RET_EINVAL;
        }
        leaf_start = p;
    }

    int leaf_len = 0;
    while ('\0' != leaf_start[leaf_len] && '/' != leaf_start[leaf_len])
    {
        if (leaf_len >= PFS_EXFAT_MAX_NAME_LENGTH)
        {
            return PFS_EXFAT_RET_ENAMETOOLONG;
        }
        leaf[leaf_len] = leaf_start[leaf_len];
        leaf_len++;
    }
    leaf[leaf_len] = '\0';
    if (0 == leaf_len || (1 == leaf_len && '.' == leaf[0]) ||
            (2 == leaf_len && '.' == leaf[0] && '.' == leaf[1]))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    int parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && '/' == path[parent_len - 1])
    {
        parent_len--;
    }
    if (parent_len <= 0)
    {
        pfs_exfat_fill_root_info(p_sbi, &parent_info);
    }
    else
    {
        if (parent_len >= (int)sizeof(parent_path))
        {
            return PFS_EXFAT_RET_ENAMETOOLONG;
        }
        memcpy(parent_path, path, (size_t)parent_len);
        parent_path[parent_len] = '\0';
        int rc = pfs_exfat_lookup_path(p_sbi, parent_path, &parent_info);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (PFS_EXFAT_TYPE_DIR != parent_info.type)
        {
            return PFS_EXFAT_RET_ENOENT;
        }
    }

    if (0 == parent_info.size && parent_info.start_clu == p_bs->root_dir_clu)
    {
        pfs_exfat_chain_set(&parent, p_bs->root_dir_clu, p_bs->num_clusters, EXFAT_ALLOC_FAT_CHAIN);
    }
    else if (0 == parent_info.size)
    {
        return PFS_EXFAT_RET_ENOSPC;
    }
    else
    {
        pfs_exfat_chain_set(&parent, parent_info.start_clu,
                PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)parent_info.size, p_bs), parent_info.flags);
    }

    int rc = pfs_exfat_utf8_to_uniname(p_sbi, leaf, leaf_len, &uni);
    if (0 > rc)
    {
        return rc;
    }

    rc = pfs_exfat_find_dir_entry(p_sbi, &parent, &uni, &exist);
    if (PFS_EXFAT_RET_OK == rc)
    {
        return PFS_EXFAT_RET_EEXIST;
    }
    if (PFS_EXFAT_RET_ENOENT != rc)
    {
        return rc;
    }

    int num_entries = pfs_exfat_calc_num_entries(&uni);
    if (num_entries < 0)
    {
        return num_entries;
    }

    uint32_t abs_entry = 0;
    rc = pfs_exfat_search_empty_slot(p_sbi, &parent, num_entries, 0, 0, &abs_entry);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_exfat_write_empty_file_set(p_sbi, &parent, abs_entry, num_entries, &uni);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    return pfs_exfat_load_dir_entry_info(p_sbi, &parent, (int)abs_entry, p_info);
}

int pfs_exfat_update_stream_meta(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_loc,
        uint32_t start_clu, uint8_t flags, int64_t size, int64_t valid_size)
{
    if (NULL == p_sbi || NULL == p_loc || size < 0 || valid_size < 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }
    if (valid_size > size)
    {
        valid_size = size;
    }

    exfat_dentry_t file_ent;
    int rc = pfs_exfat_read_dentry_at(p_sbi, &p_loc->dir, (uint32_t)p_loc->entry, &file_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    exfat_dentry_t stream_ent;
    rc = pfs_exfat_read_dentry_at(p_sbi, &p_loc->dir, (uint32_t)p_loc->entry + 1U, &stream_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_FILE != file_ent.type || EXFAT_STREAM != stream_ent.type)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    int num_ext = (int)file_ent.dentry.file.num_ext;
    if (num_ext < 2)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }

    stream_ent.dentry.stream.flags = flags;
    stream_ent.dentry.stream.start_clu = pfs_cpu_to_le32(start_clu);
    stream_ent.dentry.stream.valid_size = pfs_cpu_to_le64((uint64_t)valid_size);
    stream_ent.dentry.stream.size = pfs_cpu_to_le64((uint64_t)size);

    uint16_t chksum = 0;
    int chksum_type = EXFAT_CS_DIR_ENTRY;
    chksum = pfs_exfat_calc_chksum16_type(&file_ent, (int)sizeof(file_ent), chksum, chksum_type);
    chksum_type = EXFAT_CS_DEFAULT;
    chksum = pfs_exfat_calc_chksum16_type(&stream_ent, (int)sizeof(stream_ent), chksum, chksum_type);
    for (int i = 2; i <= num_ext; i++)
    {
        exfat_dentry_t name_ent;
        rc = pfs_exfat_read_dentry_at(p_sbi, &p_loc->dir, (uint32_t)p_loc->entry + (uint32_t)i, &name_ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        chksum = pfs_exfat_calc_chksum16_type(&name_ent, (int)sizeof(name_ent), chksum, chksum_type);
    }
    file_ent.dentry.file.checksum = pfs_cpu_to_le16(chksum);

    rc = pfs_exfat_write_dentry_at(p_sbi, &p_loc->dir, (uint32_t)p_loc->entry, &file_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    return pfs_exfat_write_dentry_at(p_sbi, &p_loc->dir, (uint32_t)p_loc->entry + 1U, &stream_ent);
}

int pfs_exfat_remove_entry_set(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_info)
{
    if (NULL == p_sbi || NULL == p_info)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    exfat_dentry_t ent;
    int rc = pfs_exfat_read_dentry_at(p_sbi, &p_info->dir, (uint32_t)p_info->entry, &ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_FILE != ent.type)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }
    int num_ext = (int)ent.dentry.file.num_ext;
    for (int i = 0; i <= num_ext; i++)
    {
        rc = pfs_exfat_read_dentry_at(p_sbi, &p_info->dir, (uint32_t)p_info->entry + (uint32_t)i, &ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        ent.type = (uint8_t)(ent.type & 0x7Fu);
        rc = pfs_exfat_write_dentry_at(p_sbi, &p_info->dir, (uint32_t)p_info->entry + (uint32_t)i, &ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_write_empty_dir_set(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, int num_entries, const pfs_exfat_uni_name_t *p_uni,
        uint32_t dir_clu, uint64_t dir_size)
{
    exfat_dentry_t ents[2 + ((PFS_EXFAT_MAX_NAME_LENGTH + EXFAT_FILE_NAME_LEN - 1) /
            EXFAT_FILE_NAME_LEN)];
    const uint16_t *uniname;
    uint16_t chksum;
    int chksum_type;

    if (num_entries < 3 || num_entries > (int)pfs_array_size(ents))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    memset(ents, 0, sizeof(ents[0]) * (size_t)num_entries);

    ents[0].type = EXFAT_FILE;
    ents[0].dentry.file.num_ext = (uint8_t)(num_entries - 1);
    ents[0].dentry.file.attr = pfs_cpu_to_le16(EXFAT_ATTR_SUBDIR);

    ents[1].type = EXFAT_STREAM;
    ents[1].dentry.stream.flags = EXFAT_ALLOC_FAT_CHAIN;
    ents[1].dentry.stream.name_len = p_uni->name_len;
    ents[1].dentry.stream.name_hash = pfs_cpu_to_le16(p_uni->name_hash);
    ents[1].dentry.stream.start_clu = pfs_cpu_to_le32(dir_clu);
    ents[1].dentry.stream.valid_size = pfs_cpu_to_le64(dir_size);
    ents[1].dentry.stream.size = pfs_cpu_to_le64(dir_size);

    uniname = p_uni->name;
    for (int i = 2; i < num_entries; i++)
    {
        pfs_exfat_init_name_entry(&ents[i], uniname);
        uniname += EXFAT_FILE_NAME_LEN;
    }

    chksum = 0;
    chksum_type = EXFAT_CS_DIR_ENTRY;
    for (int i = 0; i < num_entries; i++)
    {
        chksum = pfs_exfat_calc_chksum16_type(&ents[i], (int)sizeof(ents[i]), chksum, chksum_type);
        chksum_type = EXFAT_CS_DEFAULT;
    }
    ents[0].dentry.file.checksum = pfs_cpu_to_le16(chksum);

    for (int i = 0; i < num_entries; i++)
    {
        int rc = pfs_exfat_write_dentry_at(p_sbi, p_dir, abs_entry + (uint32_t)i, &ents[i]);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_path_parent_leaf(pfs_exfat_sbi_t *p_sbi, const char *path,
        pfs_exfat_dir_entry_t *p_parent, char *leaf, int leaf_cap, int *p_leaf_len)
{
    if (NULL == p_sbi || NULL == path || NULL == p_parent || NULL == leaf || leaf_cap <= 1 ||
            NULL == p_leaf_len)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_dir_entry_t parent_info;
    char parent_path[512];
    const char *p = path;
    const char *leaf_start;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    leaf_start = p;
    while ('\0' != *p)
    {
        const char *slash = p;

        while ('\0' != *slash && '/' != *slash)
        {
            slash++;
        }
        if ('\0' == *slash)
        {
            leaf_start = p;
            break;
        }
        p = slash;
        while ('/' == *p)
        {
            p++;
        }
        if ('\0' == *p)
        {
            return PFS_EXFAT_RET_EINVAL;
        }
        leaf_start = p;
    }

    int leaf_len = 0;
    while ('\0' != leaf_start[leaf_len] && '/' != leaf_start[leaf_len])
    {
        if (leaf_len + 1 >= leaf_cap)
        {
            return PFS_EXFAT_RET_ENAMETOOLONG;
        }
        leaf[leaf_len] = leaf_start[leaf_len];
        leaf_len++;
    }
    leaf[leaf_len] = '\0';
    if (0 == leaf_len || (1 == leaf_len && '.' == leaf[0]) ||
            (2 == leaf_len && '.' == leaf[0] && '.' == leaf[1]))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    int parent_len = (int)(leaf_start - path);
    while (parent_len > 0 && '/' == path[parent_len - 1])
    {
        parent_len--;
    }
    if (parent_len <= 0)
    {
        pfs_exfat_fill_root_info(p_sbi, &parent_info);
    }
    else
    {
        if (parent_len >= (int)sizeof(parent_path))
        {
            return PFS_EXFAT_RET_ENAMETOOLONG;
        }
        memcpy(parent_path, path, (size_t)parent_len);
        parent_path[parent_len] = '\0';
        int rc = pfs_exfat_lookup_path(p_sbi, parent_path, &parent_info);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        if (PFS_EXFAT_TYPE_DIR != parent_info.type)
        {
            return PFS_EXFAT_RET_ENOENT;
        }
    }

    *p_parent = parent_info;
    *p_leaf_len = leaf_len;
    return PFS_EXFAT_RET_OK;
}

static void pfs_exfat_parent_to_chain(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_parent,
        pfs_exfat_chain_t *p_chain)
{
    const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_sbi);

    if (0 == p_parent->size && p_parent->start_clu == p_bs->root_dir_clu)
    {
        pfs_exfat_chain_set(p_chain, p_bs->root_dir_clu, p_bs->num_clusters, EXFAT_ALLOC_FAT_CHAIN);
    }
    else
    {
        pfs_exfat_chain_set(p_chain, p_parent->start_clu,
                PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_parent->size, p_bs), p_parent->flags);
    }
}

int pfs_exfat_mkdir(pfs_exfat_sbi_t *p_sbi, const char *path)
{
    const pfs_exfat_bs_t *p_bs;
    pfs_exfat_chain_t parent;
    pfs_exfat_dir_entry_t parent_info;
    pfs_exfat_dir_entry_t exist;
    pfs_exfat_uni_name_t uni;
    char leaf[PFS_EXFAT_MAX_NAME_LENGTH + 1];
    uint32_t abs_entry;
    uint32_t new_clu;
    int num_entries;
    int leaf_len;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    int rc = pfs_exfat_path_parent_leaf(p_sbi, path, &parent_info, leaf, (int)sizeof(leaf), &leaf_len);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (0 == parent_info.size && parent_info.start_clu != p_bs->root_dir_clu)
    {
        return PFS_EXFAT_RET_ENOSPC;
    }
    pfs_exfat_parent_to_chain(p_sbi, &parent_info, &parent);

    memset(&uni, 0, sizeof(uni));
    rc = pfs_exfat_utf8_to_uniname(p_sbi, leaf, leaf_len, &uni);
    if (rc < 0)
    {
        return rc;
    }
    rc = pfs_exfat_find_dir_entry(p_sbi, &parent, &uni, &exist);
    if (PFS_EXFAT_RET_OK == rc)
    {
        return PFS_EXFAT_RET_EEXIST;
    }
    if (PFS_EXFAT_RET_ENOENT != rc)
    {
        return rc;
    }

    num_entries = pfs_exfat_calc_num_entries(&uni);
    if (num_entries < 0)
    {
        return num_entries;
    }
    rc = pfs_exfat_search_empty_slot(p_sbi, &parent, num_entries, 0, 0, &abs_entry);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_exfat_alloc_cluster(p_sbi, &new_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_exfat_zero_cluster(p_sbi, new_clu);
    if (PFS_EXFAT_RET_OK != rc)
    {
        (void)pfs_exfat_free_cluster_chain(p_sbi, new_clu, 1, EXFAT_ALLOC_FAT_CHAIN);
        return rc;
    }

    rc = pfs_exfat_write_empty_dir_set(p_sbi, &parent, abs_entry, num_entries, &uni, new_clu,
            (uint64_t)p_bs->cluster_size);
    if (PFS_EXFAT_RET_OK != rc)
    {
        (void)pfs_exfat_free_cluster_chain(p_sbi, new_clu, 1, EXFAT_ALLOC_FAT_CHAIN);
        return rc;
    }
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_dir_is_empty(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_dir_entry_t *p_dir)
{
    const pfs_exfat_bs_t *p_bs;
    pfs_exfat_chain_t chain;
    uint32_t total;
    unsigned int type;

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (0 == p_dir->size)
    {
        return 1;
    }
    pfs_exfat_chain_set(&chain, p_dir->start_clu,
            PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_dir->size, p_bs), p_dir->flags);
    total = PFS_EXFAT_CLU_TO_DEN(chain.size, p_bs);
    if (total > PFS_EXFAT_MAX_DENTRIES)
    {
        total = PFS_EXFAT_MAX_DENTRIES;
    }
    for (uint32_t dentry = 0; dentry < total; dentry++)
    {
        exfat_dentry_t ent;

        int rc = pfs_exfat_read_dentry_at(p_sbi, &chain, dentry, &ent);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        type = pfs_exfat_get_entry_type(&ent);
        if (PFS_EXFAT_TYPE_UNUSED == type)
        {
            return 1;
        }
        if (PFS_EXFAT_TYPE_DELETED == type)
        {
            continue;
        }
        if (PFS_EXFAT_TYPE_FILE == type || PFS_EXFAT_TYPE_DIR == type)
        {
            return 0;
        }
    }
    return 1;
}

int pfs_exfat_unlink(pfs_exfat_sbi_t *p_sbi, const char *path)
{
    pfs_exfat_dir_entry_t info;
    uint32_t num_clu;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    int rc = pfs_exfat_lookup_path(p_sbi, path, &info);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXFAT_TYPE_FILE != info.type)
    {
        return PFS_EXFAT_RET_EPERM;
    }

    if (info.size > 0 && EXFAT_EOF_CLUSTER != info.start_clu && EXFAT_FREE_CLUSTER != info.start_clu)
    {
        num_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)info.size, PFS_EXFAT_SBI_BS(p_sbi));
        rc = pfs_exfat_free_cluster_chain(p_sbi, info.start_clu, num_clu, info.flags);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return pfs_exfat_remove_entry_set(p_sbi, &info);
}

int pfs_exfat_rmdir(pfs_exfat_sbi_t *p_sbi, const char *path)
{
    const pfs_exfat_bs_t *p_bs;
    pfs_exfat_dir_entry_t info;
    uint32_t num_clu;
    int empty;

    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    int rc = pfs_exfat_lookup_path(p_sbi, path, &info);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXFAT_TYPE_DIR != info.type)
    {
        return PFS_EXFAT_RET_EPERM;
    }
    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    if (info.start_clu == p_bs->root_dir_clu)
    {
        return PFS_EXFAT_RET_EPERM;
    }

    empty = pfs_exfat_dir_is_empty(p_sbi, &info);
    if (empty < 0)
    {
        return empty;
    }
    if (0 == empty)
    {
        return PFS_EXFAT_RET_ENOTEMPTY;
    }

    if (info.size > 0 && EXFAT_EOF_CLUSTER != info.start_clu)
    {
        num_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)info.size, p_bs);
        rc = pfs_exfat_free_cluster_chain(p_sbi, info.start_clu, num_clu, info.flags);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return pfs_exfat_remove_entry_set(p_sbi, &info);
}

static int pfs_exfat_write_renamed_set(pfs_exfat_sbi_t *p_sbi, const pfs_exfat_chain_t *p_dir,
        uint32_t abs_entry, int num_entries, const pfs_exfat_uni_name_t *p_uni,
        const exfat_dentry_t *p_file, const exfat_dentry_t *p_stream)
{
    exfat_dentry_t ents[2 + ((PFS_EXFAT_MAX_NAME_LENGTH + EXFAT_FILE_NAME_LEN - 1) /
            EXFAT_FILE_NAME_LEN)];
    const uint16_t *uniname;
    uint16_t chksum;
    uint16_t attr;
    int chksum_type;

    if (num_entries < 3 || num_entries > (int)pfs_array_size(ents))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    memset(ents, 0, sizeof(ents[0]) * (size_t)num_entries);
    ents[0] = *p_file;
    ents[0].type = EXFAT_FILE;
    ents[0].dentry.file.num_ext = (uint8_t)(num_entries - 1);
    attr = pfs_le16_to_cpu(ents[0].dentry.file.attr);
    if (0 == (attr & EXFAT_ATTR_SUBDIR))
    {
        attr = (uint16_t)(attr | EXFAT_ATTR_ARCHIVE);
        ents[0].dentry.file.attr = pfs_cpu_to_le16(attr);
    }

    ents[1] = *p_stream;
    ents[1].type = EXFAT_STREAM;
    ents[1].dentry.stream.name_len = p_uni->name_len;
    ents[1].dentry.stream.name_hash = pfs_cpu_to_le16(p_uni->name_hash);

    uniname = p_uni->name;
    for (int i = 2; i < num_entries; i++)
    {
        pfs_exfat_init_name_entry(&ents[i], uniname);
        uniname += EXFAT_FILE_NAME_LEN;
    }

    chksum = 0;
    chksum_type = EXFAT_CS_DIR_ENTRY;
    for (int i = 0; i < num_entries; i++)
    {
        chksum = pfs_exfat_calc_chksum16_type(&ents[i], (int)sizeof(ents[i]), chksum, chksum_type);
        chksum_type = EXFAT_CS_DEFAULT;
    }
    ents[0].dentry.file.checksum = pfs_cpu_to_le16(chksum);

    for (int i = 0; i < num_entries; i++)
    {
        int rc = pfs_exfat_write_dentry_at(p_sbi, p_dir, abs_entry + (uint32_t)i, &ents[i]);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_replace_target(pfs_exfat_sbi_t *p_sbi, pfs_exfat_dir_entry_t *p_exist)
{
    const pfs_exfat_bs_t *p_bs;
    uint32_t num_clu;
    int empty;

    if (PFS_EXFAT_TYPE_DIR == p_exist->type)
    {
        empty = pfs_exfat_dir_is_empty(p_sbi, p_exist);
        if (empty < 0)
        {
            return empty;
        }
        if (0 == empty)
        {
            return PFS_EXFAT_RET_ENOTEMPTY;
        }
        if (p_exist->size > 0 && EXFAT_EOF_CLUSTER != p_exist->start_clu)
        {
            p_bs = PFS_EXFAT_SBI_BS(p_sbi);
            num_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_exist->size, p_bs);
            int rc = pfs_exfat_free_cluster_chain(p_sbi, p_exist->start_clu, num_clu, p_exist->flags);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else if (PFS_EXFAT_TYPE_FILE == p_exist->type)
    {
        if (p_exist->size > 0 && EXFAT_EOF_CLUSTER != p_exist->start_clu &&
                EXFAT_FREE_CLUSTER != p_exist->start_clu)
        {
            p_bs = PFS_EXFAT_SBI_BS(p_sbi);
            num_clu = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)p_exist->size, p_bs);
            int rc = pfs_exfat_free_cluster_chain(p_sbi, p_exist->start_clu, num_clu, p_exist->flags);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else
    {
        return PFS_EXFAT_RET_EPERM;
    }
    return pfs_exfat_remove_entry_set(p_sbi, p_exist);
}

int pfs_exfat_rename(pfs_exfat_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    const pfs_exfat_bs_t *p_bs;
    pfs_exfat_dir_entry_t old_info;
    pfs_exfat_dir_entry_t new_parent;
    pfs_exfat_dir_entry_t exist;
    pfs_exfat_chain_t new_chain;
    pfs_exfat_uni_name_t uni;
    exfat_dentry_t file_ent;
    exfat_dentry_t stream_ent;
    char leaf[PFS_EXFAT_MAX_NAME_LENGTH + 1];
    uint32_t abs_entry;
    uint32_t skip_start;
    uint32_t skip_count;
    int num_entries;
    int old_num;
    int leaf_len;
    int same_parent;

    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    if (!pfs_exfat_sbi_can_write(p_sbi))
    {
        return PFS_EXFAT_RET_EPERM;
    }
    if (0 == strcmp(oldpath, newpath))
    {
        return PFS_EXFAT_RET_OK;
    }

    p_bs = PFS_EXFAT_SBI_BS(p_sbi);
    int rc = pfs_exfat_lookup_path(p_sbi, oldpath, &old_info);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (old_info.start_clu == p_bs->root_dir_clu && PFS_EXFAT_TYPE_DIR == old_info.type &&
            0 == old_info.size)
    {
        return PFS_EXFAT_RET_EPERM;
    }

    rc = pfs_exfat_path_parent_leaf(p_sbi, newpath, &new_parent, leaf, (int)sizeof(leaf), &leaf_len);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (0 == new_parent.size && new_parent.start_clu != p_bs->root_dir_clu)
    {
        return PFS_EXFAT_RET_ENOSPC;
    }

    if (PFS_EXFAT_TYPE_DIR == old_info.type && new_parent.start_clu == old_info.start_clu &&
            new_parent.start_clu != p_bs->root_dir_clu)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_parent_to_chain(p_sbi, &new_parent, &new_chain);
    memset(&uni, 0, sizeof(uni));
    rc = pfs_exfat_utf8_to_uniname(p_sbi, leaf, leaf_len, &uni);
    if (rc < 0)
    {
        return rc;
    }

    same_parent = (old_info.dir.dir == new_chain.dir && old_info.dir.flags == new_chain.flags) ? 1 : 0;

    rc = pfs_exfat_find_dir_entry(p_sbi, &new_chain, &uni, &exist);
    if (PFS_EXFAT_RET_OK == rc)
    {
        if (exist.dir.dir == old_info.dir.dir && exist.entry == old_info.entry)
        {
            // 同项大小写变化: 继续重写名项
        }
        else
        {
            if (old_info.type != exist.type)
            {
                return PFS_EXFAT_RET_EEXIST;
            }
            rc = pfs_exfat_replace_target(p_sbi, &exist);
            if (PFS_EXFAT_RET_OK != rc)
            {
                return rc;
            }
        }
    }
    else if (PFS_EXFAT_RET_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_exfat_read_dentry_at(p_sbi, &old_info.dir, (uint32_t)old_info.entry, &file_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_exfat_read_dentry_at(p_sbi, &old_info.dir, (uint32_t)old_info.entry + 1U, &stream_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (EXFAT_FILE != file_ent.type || EXFAT_STREAM != stream_ent.type)
    {
        return PFS_EXFAT_RET_EBADMSG;
    }
    old_num = (int)file_ent.dentry.file.num_ext + 1;

    num_entries = pfs_exfat_calc_num_entries(&uni);
    if (num_entries < 0)
    {
        return num_entries;
    }

    skip_start = 0;
    skip_count = 0;
    if (same_parent)
    {
        skip_start = (uint32_t)old_info.entry;
        skip_count = (uint32_t)old_num;
    }
    rc = pfs_exfat_search_empty_slot(p_sbi, &new_chain, num_entries, skip_start, skip_count, &abs_entry);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_exfat_write_renamed_set(p_sbi, &new_chain, abs_entry, num_entries, &uni, &file_ent,
            &stream_ent);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }

    if (!(same_parent && abs_entry == (uint32_t)old_info.entry))
    {
        rc = pfs_exfat_remove_entry_set(p_sbi, &old_info);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_EXFAT_RET_OK;
}

// end
