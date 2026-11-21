// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 路径查找 / mkdir / creat (扫描缓存写子集)
#include "yaffs/pfs_yaffs_internal.h"

#include <string.h>
#include <stdbool.h>

static int pfs_yaffs_component(const char **pp, char *out, int out_cap)
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

static pfs_yaffs_obj_t *pfs_yaffs_child_by_name(pfs_yaffs_sbi_t *p_sbi, uint32_t parent_id, const char *name, int nlen)
{
    pfs_yaffs_obj_t *p_obj;

    for (p_obj = p_sbi->obj_list; NULL != p_obj; p_obj = p_obj->next)
    {
        if (!p_obj->has_hdr || !pfs_yaffs_obj_visible(p_obj))
        {
            continue;
        }
        if (p_obj->parent_id != parent_id)
        {
            continue;
        }
        if (PFS_YAFFS_OBJECTID_ROOT == p_obj->obj_id)
        {
            continue;
        }
        if (0 == nlen)
        {
            continue;
        }
        if (0 == strncmp(p_obj->name, name, (size_t)nlen) && '\0' == p_obj->name[nlen])
        {
            return p_obj;
        }
    }
    return NULL;
}

int pfs_yaffs_lookup_path(pfs_yaffs_sbi_t *p_sbi, const char *path, uint32_t *p_obj_id, bool *p_is_dir)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    pfs_yaffs_obj_t *p_obj;
    const char *p;
    char name[PFS_YAFFS_MAX_NAME_LENGTH + 1];
    int nlen;
    uint32_t obj_id;
    bool is_dir;

    obj_id = PFS_YAFFS_OBJECTID_ROOT;
    p = path;
    while ('/' == *p)
    {
        p++;
    }

    if ('\0' == *p)
    {
        if (NULL != p_obj_id)
        {
            *p_obj_id = obj_id;
        }
        if (NULL != p_is_dir)
        {
            *p_is_dir = true;
        }
        return PFS_OK;
    }

    while ('\0' != *p)
    {
        nlen = pfs_yaffs_component(&p, name, (int)sizeof(name));
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
            if (PFS_YAFFS_OBJECTID_ROOT != obj_id)
            {
                return PFS_ENOENT;
            }
            continue;
        }

        p_obj = pfs_yaffs_obj_find(p_sbi, obj_id);
        if (NULL == p_obj || !pfs_yaffs_obj_is_dir(p_obj))
        {
            return PFS_ENOENT;
        }
        p_obj = pfs_yaffs_child_by_name(p_sbi, obj_id, name, nlen);
        if (NULL == p_obj)
        {
            return PFS_ENOENT;
        }
        obj_id = p_obj->obj_id;
    }

    p_obj = pfs_yaffs_obj_find(p_sbi, obj_id);
    if (NULL == p_obj || !p_obj->has_hdr)
    {
        return PFS_ENOENT;
    }
    is_dir = pfs_yaffs_obj_is_dir(p_obj);

    if (NULL != p_obj_id)
    {
        *p_obj_id = obj_id;
    }
    if (NULL != p_is_dir)
    {
        *p_is_dir = is_dir;
    }
    return PFS_OK;
}

int pfs_yaffs_access(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode)
{
    uint32_t obj_id;
    bool is_dir;

    (void)mode;
    int rc = pfs_yaffs_lookup_path(p_sbi, path, &obj_id, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    (void)obj_id;
    (void)is_dir;
    return PFS_OK;
}

static int pfs_yaffs_split_parent(const char *path, char *parent, int parent_cap, char *leaf, int leaf_cap)
{
    const char *p;
    const char *slash;

    p = path;
    while ('/' == *p)
    {
        p++;
    }
    if ('\0' == *p)
    {
        return PFS_EINVAL;
    }

    int plen;
    int llen;

    slash = strrchr(p, '/');
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
    plen = (int)(slash - path);
    if (0 == plen)
    {
        plen = 1;
    }
    llen = (int)strlen(slash + 1);
    if (llen <= 0 || plen >= parent_cap || llen >= leaf_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(parent, path, (size_t)plen);
    parent[plen] = '\0';
    memcpy(leaf, slash + 1, (size_t)llen + 1);
    return PFS_OK;
}

int pfs_yaffs_mkdir(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_YAFFS_MAX_NAME_LENGTH + 1];
    uint32_t pobj;
    uint32_t nobj;
    uint32_t imode;
    bool is_dir;
    int nlen;

    int rc = pfs_yaffs_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    nlen = (int)strlen(leaf);
    rc = pfs_yaffs_lookup_path(p_sbi, parent, &pobj, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    if (NULL != pfs_yaffs_child_by_name(p_sbi, pobj, leaf, nlen))
    {
        return PFS_EEXIST;
    }

    nobj = p_sbi->next_obj_id++;
    imode = PFS_YAFFS_S_IFDIR | ((0 != (mode & 0777)) ? (uint32_t)(mode & 0777) : 0755u);
    rc = pfs_yaffs_emit_obj_hdr(p_sbi, nobj, PFS_YAFFS_OBJECT_TYPE_DIRECTORY, pobj, imode, 0, leaf);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_yaffs_sync_media(p_sbi);
}

int pfs_yaffs_creat(pfs_yaffs_sbi_t *p_sbi, const char *path, uint32_t *p_obj_id)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (!pfs_yaffs_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    char parent[512];
    char leaf[PFS_YAFFS_MAX_NAME_LENGTH + 1];
    uint32_t pobj;
    uint32_t nobj;
    bool is_dir;
    int nlen;

    int rc = pfs_yaffs_split_parent(path, parent, (int)sizeof(parent), leaf, (int)sizeof(leaf));
    if (PFS_OK != rc)
    {
        return rc;
    }
    nlen = (int)strlen(leaf);
    rc = pfs_yaffs_lookup_path(p_sbi, parent, &pobj, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!is_dir)
    {
        return PFS_EINVAL;
    }
    if (NULL != pfs_yaffs_child_by_name(p_sbi, pobj, leaf, nlen))
    {
        return PFS_EEXIST;
    }

    nobj = p_sbi->next_obj_id++;
    rc = pfs_yaffs_emit_obj_hdr(p_sbi, nobj, PFS_YAFFS_OBJECT_TYPE_FILE, pobj, PFS_YAFFS_S_IFREG | 0644, 0, leaf);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (NULL != p_obj_id)
    {
        *p_obj_id = nobj;
    }
    return pfs_yaffs_sync_media(p_sbi);
}

int pfs_yaffs_rmdir(pfs_yaffs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

int pfs_yaffs_rename(pfs_yaffs_sbi_t *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_EPERM;
}

int pfs_yaffs_unlink(pfs_yaffs_sbi_t *p_sbi, const char *path)
{
    (void)p_sbi;
    (void)path;
    return PFS_EPERM;
}

// end
