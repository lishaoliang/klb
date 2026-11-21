// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 目录流 (扫描 dents; . / .. 由 vfs 合成)
#include "jffs2/pfs_jffs2_internal.h"

#include <string.h>
#include <stdbool.h>

static uint8_t pfs_jffs2_map_dtype(uint8_t jtype, const pfs_jffs2_ic_t *p_child)
{
    if (PFS_JFFS2_DT_DIR == jtype)
    {
        return PFS_DT_DIR;
    }
    if (PFS_JFFS2_DT_REG == jtype)
    {
        return PFS_DT_REG;
    }
    if (NULL != p_child && pfs_jffs2_ic_is_dir(p_child))
    {
        return PFS_DT_DIR;
    }
    if (0 == jtype)
    {
        return PFS_DT_REG;
    }
    return PFS_DT_UNKNOWN;
}

int pfs_jffs2_dir_open(pfs_jffs2_sbi_t *p_sbi, const char *path, pfs_jffs2_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));
    uint32_t ino;
    bool is_dir;
    int rc = pfs_jffs2_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, ino);
    p_dm->ino = ino;
    p_dm->invent = 0;
    p_dm->p_next = (NULL != p_ic) ? p_ic->dents : NULL;
    return PFS_OK;
}

void pfs_jffs2_dir_close(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_jffs2_dir_readdir(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));

    while (NULL != p_dm->p_next)
    {
        pfs_jffs2_fd_t *p_fd = p_dm->p_next;
        p_dm->p_next = p_fd->next;
        if (0 == p_fd->ino)
        {
            continue;
        }
        pfs_jffs2_ic_t *p_child = pfs_jffs2_ic_find(p_sbi, p_fd->ino);
        p_ent->d_type = pfs_jffs2_map_dtype(p_fd->type, p_child);
        size_t ncopy = (size_t)p_fd->nsize;
        if (ncopy > sizeof(p_ent->d_name) - 1)
        {
            ncopy = sizeof(p_ent->d_name) - 1;
        }
        memcpy(p_ent->d_name, p_fd->name, ncopy);
        p_ent->d_name[ncopy] = '\0';
        return 1;
    }
    return 0;
}

// end
