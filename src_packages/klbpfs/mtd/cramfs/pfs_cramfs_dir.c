// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs 目录流 (盘上无 . / ..; 连续 inode+name)
#include "cramfs/pfs_cramfs_internal.h"

#include <string.h>
#include <stdbool.h>

int pfs_cramfs_dir_open(pfs_cramfs_sbi_t *p_sbi, const char *path, pfs_cramfs_dir_meta_t *p_dm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_cramfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_dm, 0, sizeof(*p_dm));
    pfs_cramfs_inode_t ino = {0};
    bool is_dir = false;
    int rc = pfs_cramfs_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    p_dm->ino = ino;
    p_dm->dir_off = pfs_cramfs_inode_byte_off(&ino);
    p_dm->dir_end = p_dm->dir_off + ino.size;
    p_dm->cur = p_dm->dir_off;
    return PFS_OK;
}

void pfs_cramfs_dir_close(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_dir_meta_t *p_dm)
{
    (void)p_sbi;
    if (NULL != p_dm)
    {
        memset(p_dm, 0, sizeof(*p_dm));
    }
}

int pfs_cramfs_dir_readdir(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dm || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (!pfs_cramfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_ent, 0, sizeof(*p_ent));
    if (p_dm->cur + 12 > p_dm->dir_end)
    {
        return PFS_ENOENT;
    }

    pfs_cramfs_inode_t ino;
    char name[PFS_CRAMFS_MAXPATHLEN + 1];
    int nlen;
    unsigned int padded;
    size_t ncopy;
    int rc = pfs_cramfs_read_inode_at(p_sbi, p_dm->cur, &ino, name, (int)sizeof(name), &nlen);
    if (PFS_OK != rc)
    {
        return rc;
    }
    padded = pfs_cramfs_name_padded(&ino);
    p_dm->cur += 12u + padded;

    if (PFS_CRAMFS_S_IFDIR == (ino.mode & PFS_CRAMFS_S_IFMT))
    {
        p_ent->d_type = PFS_DT_DIR;
    }
    else if (PFS_CRAMFS_S_IFREG == (ino.mode & PFS_CRAMFS_S_IFMT))
    {
        p_ent->d_type = PFS_DT_REG;
    }
    else
    {
        p_ent->d_type = PFS_DT_UNKNOWN;
    }
    ncopy = (size_t)nlen;
    if (ncopy > sizeof(p_ent->d_name) - 1)
    {
        ncopy = sizeof(p_ent->d_name) - 1;
    }
    if (ncopy > 0)
    {
        memcpy(p_ent->d_name, name, ncopy);
    }
    p_ent->d_name[ncopy] = '\0';
    return PFS_OK;
}

// end
