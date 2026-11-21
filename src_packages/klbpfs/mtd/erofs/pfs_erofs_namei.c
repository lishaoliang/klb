// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs 路径查找 (只读; 目录块线性扫 dirent)
#include "erofs/pfs_erofs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static int pfs_erofs_component(const char **pp, char *out, int out_cap)
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

static int pfs_erofs_lookup_one(pfs_erofs_sbi_t *p_sbi, const pfs_erofs_inode_t *p_dir, const char *name, int nlen, uint64_t *p_nid, uint8_t *p_ft)
{
    if (NULL == p_sbi || NULL == p_dir || NULL == name || nlen <= 0 || NULL == p_nid)
    {
        return PFS_EINVAL;
    }
    if (PFS_EROFS_S_IFDIR != (p_dir->mode & PFS_EROFS_S_IFMT))
    {
        return PFS_ENOENT;
    }

    uint8_t *blk = (uint8_t *)pfsmtd_malloc((size_t)p_sbi->blksz);

    if (NULL == blk)
    {
        return PFS_ENOMEM;
    }

    for (int64_t pos = 0; pos < (int64_t)p_dir->i_size; pos += (int64_t)p_sbi->blksz)
    {
        unsigned int maxsize = p_sbi->blksz;

        if ((uint64_t)pos + maxsize > p_dir->i_size)
        {
            maxsize = (unsigned int)(p_dir->i_size - (uint64_t)pos);
        }

        int got = 0;
        int rc = pfs_erofs_map_read(p_sbi, p_dir, pos, blk, (int)maxsize, &got);
        if (PFS_OK != rc)
        {
            pfsmtd_free(blk);
            return rc;
        }
        if (got < (int)sizeof(pfs_erofs_dirent_t))
        {
            continue;
        }

        pfs_erofs_dirent_t *de = (pfs_erofs_dirent_t *)blk;
        unsigned int nameoff0 = de[0].nameoff;

        if (0 == nameoff0 || nameoff0 >= maxsize || 0 != (nameoff0 % sizeof(pfs_erofs_dirent_t)))
        {
            pfsmtd_free(blk);
            return PFS_EBADMSG;
        }

        unsigned int ndirents = nameoff0 / (unsigned int)sizeof(pfs_erofs_dirent_t);

        for (unsigned int i = 0; i < ndirents; i++)
        {
            unsigned int nameoff = de[i].nameoff;

            if (nameoff >= maxsize)
            {
                continue;
            }

            unsigned int namelen;

            if (i + 1 < ndirents)
            {
                namelen = de[i + 1].nameoff - nameoff;
            }
            else
            {
                namelen = (unsigned int)strnlen((const char *)(blk + nameoff), maxsize - nameoff);
            }
            if ((int)namelen == nlen && 0 == memcmp(blk + nameoff, name, (size_t)nlen))
            {
                *p_nid = de[i].nid;
                if (NULL != p_ft)
                {
                    *p_ft = de[i].file_type;
                }
                pfsmtd_free(blk);
                return PFS_OK;
            }
        }
    }

    pfsmtd_free(blk);
    return PFS_ENOENT;
}

int pfs_erofs_lookup_path(pfs_erofs_sbi_t *p_sbi, const char *path, uint64_t *p_nid, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_erofs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    uint64_t nid = p_sbi->root_nid;
    const char *p = path;

    while ('/' == *p)
    {
        p++;
    }

    if ('\0' == *p)
    {
        if (NULL != p_nid)
        {
            *p_nid = nid;
        }
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    pfs_erofs_inode_t vi;
    char name[PFS_EROFS_NAME_LEN + 1];

    while ('\0' != *p)
    {
        int nlen = pfs_erofs_component(&p, name, (int)sizeof(name));
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
            // 最小实现: 非根 .. 未跟踪父 nid
            if (nid != p_sbi->root_nid)
            {
                return PFS_ENOENT;
            }
            continue;
        }

        int rc = pfs_erofs_read_inode(p_sbi, nid, &vi);
        if (PFS_OK != rc)
        {
            return rc;
        }
        rc = pfs_erofs_lookup_one(p_sbi, &vi, name, nlen, &nid, NULL);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }

    int rc = pfs_erofs_read_inode(p_sbi, nid, &vi);
    if (PFS_OK != rc)
    {
        return rc;
    }

    bool is_dir = (PFS_EROFS_S_IFDIR == (vi.mode & PFS_EROFS_S_IFMT));
    if (NULL != p_nid)
    {
        *p_nid = nid;
    }
    if (NULL != p_is_dir)
    {
        *p_is_dir = is_dir;
    }
    return PFS_OK;
}

int pfs_erofs_access(pfs_erofs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)mode;

    uint64_t nid;
    bool is_dir;
    int rc = pfs_erofs_lookup_path(p_sbi, path, &nid, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)nid;
    (void)is_dir;
    return PFS_OK;
}

int pfs_erofs_mkdir(pfs_erofs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)p_sbi;
    (void)path;
    (void)mode;
    return PFS_EPERM;
}

int pfs_erofs_rmdir(pfs_erofs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_erofs_rename(pfs_erofs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_erofs_unlink(pfs_erofs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
