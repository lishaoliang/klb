// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 路径查找 / mkdir / creat (扫描缓存写子集)
#include "jffs2/pfs_jffs2_internal.h"

#include <string.h>
#include <stdbool.h>

static int pfs_jffs2_component(const char **pp, char *out, int out_cap)
{
    const char *p = *pp;

    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        *pp = p;
        return 0;
    }
    int n = 0;
    while ('\0' != *p && '/' != *p)
    {
        if (n + 1 >= out_cap)
        {
            return -1;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    *pp = p;
    return n;
}

static pfs_jffs2_fd_t *pfs_jffs2_fd_find(pfs_jffs2_ic_t *p_ic, const char *name, int nlen)
{
    if (NULL == p_ic || NULL == name || nlen <= 0)
    {
        return NULL;
    }

    for (pfs_jffs2_fd_t *p_fd = p_ic->dents; NULL != p_fd; p_fd = p_fd->next)
    {
        if (0 == p_fd->ino)
        {
            continue;
        }
        if ((int)p_fd->nsize == nlen && 0 == memcmp(p_fd->name, name, (size_t)nlen))
        {
            return p_fd;
        }
    }
    return NULL;
}

int pfs_jffs2_lookup_path(pfs_jffs2_sbi_t *p_sbi, const char *path, uint32_t *p_ino, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    uint32_t ino = PFS_JFFS2_ROOT_INO;
    const char *p = path;

    while ('/' == *p)
    {
        p++;
    }

    if ('\0' == *p)
    {
        (void)pfs_jffs2_ic_find(p_sbi, ino);
        if (NULL != p_ino)
        {
            *p_ino = ino;
        }
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    char name[PFS_JFFS2_MAX_NAME_LEN + 1];

    while ('\0' != *p)
    {
        int nlen = pfs_jffs2_component(&p, name, (int)sizeof(name));
        if (nlen < 0)
        {
            return PFS_ENAMETOOLONG;
        }
        if (0 == nlen)
        {
            break;
        }
        if (1 == nlen && '.' == name[0])
        {
            continue;
        }
        if (2 == nlen && '.' == name[0] && '.' == name[1])
        {
            // 无父链缓存; 根上 .. 仍为根
            if (PFS_JFFS2_ROOT_INO != ino)
            {
                return PFS_ENOENT;
            }
            continue;
        }

        pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, ino);

        if (NULL == p_ic || !pfs_jffs2_ic_is_dir(p_ic))
        {
            return PFS_ENOENT;
        }

        pfs_jffs2_fd_t *p_fd = pfs_jffs2_fd_find(p_ic, name, nlen);

        if (NULL == p_fd || 0 == p_fd->ino)
        {
            return PFS_ENOENT;
        }
        ino = p_fd->ino;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, ino);
    bool is_dir;

    if (NULL == p_ic)
    {
        // 仅有 dirent 指向, 尚无 inode 节点: 当普通文件空
        is_dir = false;
    }
    else
    {
        is_dir = pfs_jffs2_ic_is_dir(p_ic);
        if (!is_dir && NULL != p_ic->dents)
        {
            is_dir = true;
        }
    }

    if (NULL != p_ino)
    {
        *p_ino = ino;
    }
    if (NULL != p_is_dir)
    {
        *p_is_dir = is_dir;
    }
    return PFS_OK;
}

int pfs_jffs2_access(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode)
{
    (void)mode;

    uint32_t ino;
    bool is_dir;

    int rc = pfs_jffs2_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)ino;
    (void)is_dir;
    return PFS_OK;
}

static int pfs_jffs2_split_parent(const char *path, char *parent, int parent_cap, char *leaf, int leaf_cap)
{
    const char *p = path;

    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_EINVAL;
    }

    const char *slash = strrchr(p, '/');

    if (NULL == slash)
    {
        if (parent_cap < 2 || leaf_cap <= (int)strlen(p))
        {
            return PFS_ENAMETOOLONG;
        }
        parent[0] = '/';
        parent[1] = '\0';
        memcpy(leaf, p, (size_t)strlen(p) + 1);
        return PFS_OK;
    }

    int plen = (int)(slash - path);
    int llen = (int)strlen(slash + 1);

    if (0 == plen)
    {
        plen = 1;
    }
    if (llen <= 0 || plen >= parent_cap || llen >= leaf_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(parent, path, (size_t)plen);
    parent[plen] = '\0';
    memcpy(leaf, slash + 1, (size_t)llen + 1);
    return PFS_OK;
}

int pfs_jffs2_mkdir(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_JFFS2_MAX_NAME_LEN + 1];

    int rc = pfs_jffs2_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }

    int nlen = (int)strlen(leaf);
    uint32_t pino;
    bool is_dir;

    rc = pfs_jffs2_lookup_path(p_sbi, parent, &pino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_ic_t *p_pic = pfs_jffs2_ic_find(p_sbi, pino);

    if (NULL == p_pic)
    {
        return PFS_ENOENT;
    }
    if (NULL != pfs_jffs2_fd_find(p_pic, leaf, nlen))
    {
        return PFS_EEXIST;
    }

    uint32_t nino = p_sbi->next_ino++;
    uint32_t imode = PFS_JFFS2_S_IFDIR | ((0 != (mode & 0777)) ? (uint32_t)(mode & 0777) : 0755u);
    uint32_t ver = 1;

    rc = pfs_jffs2_emit_inode(p_sbi, nino, ver, imode, 0, 0, 0, NULL, PFS_JFFS2_COMPR_NONE);
    if (PFS_OK != rc)
    {
        return rc;
    }

    pfs_jffs2_ic_t *p_nic = pfs_jffs2_ic_find(p_sbi, nino);

    if (NULL != p_nic)
    {
        p_nic->highest_version = ver;
    }
    rc = pfs_jffs2_emit_dirent(p_sbi, pino, 1, nino, PFS_JFFS2_DT_DIR, leaf, nlen);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_jffs2_sync_media(p_sbi);
}

int pfs_jffs2_creat(pfs_jffs2_sbi_t *p_sbi, const char *path, uint32_t perm, uint32_t *p_ino)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_JFFS2_MAX_NAME_LEN + 1];

    int rc = pfs_jffs2_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }

    int nlen = (int)strlen(leaf);
    uint32_t pino;
    bool is_dir;

    rc = pfs_jffs2_lookup_path(p_sbi, parent, &pino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_ic_t *p_pic = pfs_jffs2_ic_find(p_sbi, pino);

    if (NULL == p_pic)
    {
        return PFS_ENOENT;
    }
    if (NULL != pfs_jffs2_fd_find(p_pic, leaf, nlen))
    {
        return PFS_EEXIST;
    }

    uint32_t nino = p_sbi->next_ino++;
    uint32_t ver = 1;
    uint32_t imode = PFS_JFFS2_S_IFREG | ((0 != (perm & 0777u)) ? (perm & 0777u) : 0644u);

    rc = pfs_jffs2_emit_inode(p_sbi, nino, ver, imode, 0, 0, 0, NULL, PFS_JFFS2_COMPR_NONE);
    if (PFS_OK != rc)
    {
        return rc;
    }

    pfs_jffs2_ic_t *p_nic = pfs_jffs2_ic_find(p_sbi, nino);

    if (NULL != p_nic)
    {
        p_nic->highest_version = ver;
    }
    rc = pfs_jffs2_emit_dirent(p_sbi, pino, 1, nino, PFS_JFFS2_DT_REG, leaf, nlen);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (NULL != p_ino)
    {
        *p_ino = nino;
    }
    return pfs_jffs2_sync_media(p_sbi);
}

static bool pfs_jffs2_ic_dir_empty(const pfs_jffs2_ic_t *p_ic)
{
    if (NULL == p_ic)
    {
        return true;
    }

    for (const pfs_jffs2_fd_t *p_fd = p_ic->dents; NULL != p_fd; p_fd = p_fd->next)
    {
        if (0 != p_fd->ino)
        {
            return false;
        }
    }
    return true;
}

static int pfs_jffs2_unlink_dirent(pfs_jffs2_sbi_t *p_sbi, uint32_t pino, const char *leaf, int nlen)
{
    pfs_jffs2_ic_t *p_pic = pfs_jffs2_ic_find(p_sbi, pino);

    if (NULL == p_pic)
    {
        return PFS_ENOENT;
    }

    pfs_jffs2_fd_t *p_fd = pfs_jffs2_fd_find(p_pic, leaf, nlen);

    if (NULL == p_fd)
    {
        return PFS_ENOENT;
    }

    uint32_t ver = p_fd->version + 1u;

    int rc = pfs_jffs2_emit_dirent(p_sbi, pino, ver, 0, p_fd->type, leaf, nlen);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_jffs2_sync_media(p_sbi);
}

int pfs_jffs2_rmdir(pfs_jffs2_sbi_t *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }
    if (0 == strcmp(path, "/") || 0 == strcmp(path, ""))
    {
        return PFS_EINVAL;
    }

    char parent[512];
    char leaf[PFS_JFFS2_MAX_NAME_LEN + 1];

    int rc = pfs_jffs2_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }

    int nlen = (int)strlen(leaf);
    uint32_t ino;
    bool is_dir;

    rc = pfs_jffs2_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_ENOTDIR;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, ino);

    if (!pfs_jffs2_ic_dir_empty(p_ic))
    {
        return PFS_ENOTEMPTY;
    }

    uint32_t pino;

    rc = pfs_jffs2_lookup_path(p_sbi, parent, &pino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    return pfs_jffs2_unlink_dirent(p_sbi, pino, leaf, nlen);
}

int pfs_jffs2_rename(pfs_jffs2_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_jffs2_unlink(pfs_jffs2_sbi_t *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }
    if (0 == strcmp(path, "/") || 0 == strcmp(path, ""))
    {
        return PFS_EINVAL;
    }

    char parent[512];
    char leaf[PFS_JFFS2_MAX_NAME_LEN + 1];

    int rc = pfs_jffs2_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }

    int nlen = (int)strlen(leaf);
    uint32_t ino;
    bool is_dir;

    rc = pfs_jffs2_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EISDIR;
    }

    uint32_t pino;

    rc = pfs_jffs2_lookup_path(p_sbi, parent, &pino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    return pfs_jffs2_unlink_dirent(p_sbi, pino, leaf, nlen);
}

// end
