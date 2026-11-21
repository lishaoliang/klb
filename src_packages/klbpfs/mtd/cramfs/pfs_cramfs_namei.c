// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs 路径查找 (只读; 目录线性扫, 无 . / ..)
#include "cramfs/pfs_cramfs_internal.h"

#include <string.h>
#include <stdbool.h>

static int pfs_cramfs_component(const char **pp, char *out, int out_cap)
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
        if (out_cap <= n + 1)
        {
            return -1;
        }
        out[n++] = *p++;
    }
    out[n] = '\0';
    *pp = p;
    return n;
}

static int pfs_cramfs_lookup_one(pfs_cramfs_sbi_t *p_sbi, const pfs_cramfs_inode_t *p_dir,
        const char *name, int nlen, pfs_cramfs_inode_t *p_out)
{
    if (PFS_CRAMFS_S_IFDIR != (p_dir->mode & PFS_CRAMFS_S_IFMT))
    {
        return PFS_ENOENT;
    }
    if (0 == p_dir->size || 0 == p_dir->offset)
    {
        return PFS_ENOENT;
    }

    uint32_t cur = pfs_cramfs_inode_byte_off(p_dir);
    uint32_t end = cur + p_dir->size;
    pfs_cramfs_inode_t ino;
    char ename[PFS_CRAMFS_MAXPATHLEN + 1];
    int enlen;
    unsigned int padded;

    while (cur + 12 <= end)
    {
        int rc = pfs_cramfs_read_inode_at(p_sbi, cur, &ino, ename, (int)sizeof(ename), &enlen);
        if (PFS_OK != rc)
        {
            return rc;
        }
        padded = pfs_cramfs_name_padded(&ino);
        if (enlen == nlen && 0 == memcmp(ename, name, (size_t)nlen))
        {
            *p_out = ino;
            return PFS_OK;
        }
        if (0 != (p_sbi->flags & PFS_CRAMFS_FLAG_SORTED_DIRS) && 0 < enlen && 0 < nlen)
        {
            if ((unsigned char)ename[0] > (unsigned char)name[0])
            {
                return PFS_ENOENT;
            }
        }
        cur += 12u + padded;
        if (0 != (cur & 3u))
        {
            return PFS_EBADMSG;
        }
    }
    return PFS_ENOENT;
}

int pfs_cramfs_lookup_path(pfs_cramfs_sbi_t *p_sbi, const char *path, pfs_cramfs_inode_t *p_ino, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path || NULL == p_ino)
    {
        return PFS_EINVAL;
    }
    if (!pfs_cramfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    pfs_cramfs_inode_t cur = p_sbi->root;
    const char *p = path;
    char name[PFS_CRAMFS_MAXPATHLEN + 1];
    while ('/' == *p)
    {
        p++;
    }

    if ('\0' == *p)
    {
        *p_ino = cur;
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    while ('\0' != *p)
    {
        int nlen = pfs_cramfs_component(&p, name, (int)sizeof(name));
        if (0 > nlen)
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
            // 无父链; 仅根上允许
            if (0 != memcmp(&cur, &p_sbi->root, sizeof(cur)))
            {
                return PFS_ENOENT;
            }
            continue;
        }
        int rc = pfs_cramfs_lookup_one(p_sbi, &cur, name, nlen, &cur);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }

    bool is_dir = (PFS_CRAMFS_S_IFDIR == (cur.mode & PFS_CRAMFS_S_IFMT));
    *p_ino = cur;
    if (NULL != p_is_dir)
    {
        *p_is_dir = is_dir;
    }
    return PFS_OK;
}

int pfs_cramfs_access(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode)
{
    pfs_cramfs_inode_t ino;
    bool is_dir;

    (void)mode;
    int rc = pfs_cramfs_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)ino;
    (void)is_dir;
    return PFS_OK;
}

int pfs_cramfs_mkdir(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)p_sbi;
    (void)path;
    (void)mode;
    return PFS_EPERM;
}

int pfs_cramfs_rmdir(pfs_cramfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_cramfs_rename(pfs_cramfs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_cramfs_unlink(pfs_cramfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
