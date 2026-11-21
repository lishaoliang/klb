// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 目录流 (. / .. + 缓存 dent)
#include "ubifs/pfs_ubifs_internal.h"

#include <string.h>
#include <stdbool.h>

int pfs_ubifs_dir_open(pfs_ubifs_sbi_t *p_sbi, const char *path, pfs_ubifs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));
    uint64_t inum;
    bool is_dir;
    int rc = pfs_ubifs_lookup_path(p_sbi, path, &inum, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    p_dm->inum = inum;
    p_dm->invent = 0;
    p_dm->dent_idx = 0;
    return PFS_OK;
}

void pfs_ubifs_dir_close(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_ubifs_dir_readdir(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    size_t ncopy;

    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_ubifs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));
    if (0 == p_dm->invent)
    {
        p_ent->d_type = PFS_DT_DIR;
        p_ent->d_name[0] = '.';
        p_ent->d_name[1] = '\0';
        p_dm->invent = 1;
        return PFS_OK;
    }
    if (1 == p_dm->invent)
    {
        p_ent->d_type = PFS_DT_DIR;
        p_ent->d_name[0] = '.';
        p_ent->d_name[1] = '.';
        p_ent->d_name[2] = '\0';
        p_dm->invent = 2;
        p_dm->dent_idx = 0;
        return PFS_OK;
    }

    for (int i = p_dm->dent_idx; i < PFS_UBIFS_CACHE_DENT_MAX; i++)
    {
        if (!p_sbi->p_dcs[i].used || p_sbi->p_dcs[i].parent != p_dm->inum)
        {
            continue;
        }
        p_dm->dent_idx = i + 1;
        if (PFS_UBIFS_ITYPE_DIR == p_sbi->p_dcs[i].type)
        {
            p_ent->d_type = PFS_DT_DIR;
        }
        else if (PFS_UBIFS_ITYPE_REG == p_sbi->p_dcs[i].type)
        {
            p_ent->d_type = PFS_DT_REG;
        }
        else
        {
            p_ent->d_type = PFS_DT_UNKNOWN;
        }
        ncopy = (size_t)p_sbi->p_dcs[i].nlen;
        if (ncopy > sizeof(p_ent->d_name) - 1)
        {
            ncopy = sizeof(p_ent->d_name) - 1;
        }
        memcpy(p_ent->d_name, p_sbi->p_dcs[i].name, ncopy);
        p_ent->d_name[ncopy] = '\0';
        return PFS_OK;
    }
    return PFS_ENOENT;
}

// end
