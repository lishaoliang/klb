// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs 路径查找 (只读; 目录链 + HRD; . / .. 由镜像提供)
#include "romfs/pfs_romfs_internal.h"

#include <string.h>
#include <stdbool.h>

static int pfs_romfs_component(const char **pp, char *out, int out_cap)
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

static int pfs_romfs_lookup_one(pfs_romfs_sbi_t *p_sbi, const pfs_romfs_fh_t *p_dir,
        const char *name, int nlen, pfs_romfs_fh_t *p_out)
{
    if (PFS_ROMFH_DIR != p_dir->type)
    {
        return PFS_ENOENT;
    }

    uint32_t cur = p_dir->spec & PFS_ROMFS_FH_MASK;
    pfs_romfs_fh_t ent;
    char ename[PFS_ROMFS_MAXFN + 1];
    int enlen;
    while (0 != cur && cur + PFS_ROMFS_FH_SIZE <= p_sbi->img_size)
    {
        int rc = pfs_romfs_read_fh(p_sbi, cur, &ent, ename, (int)sizeof(ename), &enlen);
        if (PFS_OK != rc)
        {
            return rc;
        }
        if (enlen == nlen && 0 == memcmp(ename, name, (size_t)nlen))
        {
            // 跟随硬链接到真实类型
            return pfs_romfs_resolve_fh(p_sbi, cur, p_out);
        }
        cur = ent.next;
    }
    return PFS_ENOENT;
}

int pfs_romfs_lookup_path(pfs_romfs_sbi_t *p_sbi, const char *path, pfs_romfs_fh_t *p_fh, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fh)
    {
        return PFS_EINVAL;
    }
    if (!pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    pfs_romfs_fh_t cur;
    int rc = pfs_romfs_resolve_fh(p_sbi, p_sbi->root_pos, &cur);
    if (PFS_OK != rc)
    {
        return rc;
    }

    const char *p = path;
    char name[PFS_ROMFS_MAXFN + 1];
    while ('/' == *p)
    {
        p++;
    }

    if ('\0' == *p)
    {
        *p_fh = cur;
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    while ('\0' != *p)
    {
        int nlen = pfs_romfs_component(&p, name, (int)sizeof(name));
        if (0 > nlen)
        {
            return PFS_ENAMETOOLONG;
        }
        if (0 == nlen)
        {
            break;
        }
        rc = pfs_romfs_lookup_one(p_sbi, &cur, name, nlen, &cur);
        if (PFS_OK != rc)
        {
            return rc;
        }
    }

    bool is_dir = (PFS_ROMFH_DIR == cur.type);
    *p_fh = cur;
    if (NULL != p_is_dir)
    {
        *p_is_dir = is_dir;
    }
    return PFS_OK;
}

int pfs_romfs_access(pfs_romfs_sbi_t *p_sbi, const char *path, int mode)
{
    pfs_romfs_fh_t fh;
    bool is_dir;

    (void)mode;
    int rc = pfs_romfs_lookup_path(p_sbi, path, &fh, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)fh;
    (void)is_dir;
    return PFS_OK;
}

int pfs_romfs_mkdir(pfs_romfs_sbi_t *p_sbi, const char *path, int mode)
{
    (void)p_sbi;
    (void)path;
    (void)mode;
    return PFS_EPERM;
}

int pfs_romfs_rmdir(pfs_romfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_romfs_rename(pfs_romfs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_romfs_unlink(pfs_romfs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
