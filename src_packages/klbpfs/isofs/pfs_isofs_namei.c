// Doc Encode : UTF-8 BOM, Unix(LF)
// ISO 9660 路径查找 (只读; 支持 ..)
#include "isofs/pfs_isofs_internal.h"

#include <string.h>

int pfs_isofs_lookup_path(pfs_isofs_sbi_t *p_sbi, const char *path, pfs_isofs_inode_t *p_out)
{
    if (NULL == p_sbi || NULL == path || NULL == p_out)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (!pfs_isofs_sbi_is_mounted(p_sbi))
    {
        return PFS_ISOFS_RET_EINVAL;
    }

    if ('\0' == path[0] || (('/' == path[0] || '\\' == path[0]) && '\0' == path[1]))
    {
        *p_out = p_sbi->root;
        return PFS_ISOFS_RET_OK;
    }

    pfs_isofs_inode_t cur = p_sbi->root;
    const char *p = path;
    char comp[PFS_NAME_MAX + 1];

    while ('/' == *p || '\\' == *p)
    {
        p++;
    }

    while ('\0' != *p)
    {
        int ci = 0;
        while ('\0' != *p && '/' != *p && '\\' != *p)
        {
            if (ci >= PFS_NAME_MAX)
            {
                return PFS_ISOFS_RET_ENAMETOOLONG;
            }
            comp[ci] = *p;
            ci++;
            p++;
        }
        comp[ci] = '\0';
        while ('/' == *p || '\\' == *p)
        {
            p++;
        }
        if (0 == ci)
        {
            continue;
        }
        if (0 == strcmp(comp, "."))
        {
            continue;
        }
        if (0 == strcmp(comp, ".."))
        {
            int rc = pfs_isofs_parent_dir(p_sbi, &cur, &cur);
            if (PFS_ISOFS_RET_OK != rc)
            {
                return rc;
            }
            continue;
        }

        int rc = pfs_isofs_find_in_dir(p_sbi, &cur, comp, &cur);
        if (PFS_ISOFS_RET_OK != rc)
        {
            return rc;
        }
        if ('\0' != *p && PFS_ISOFS_TYPE_DIR != cur.type)
        {
            return PFS_ISOFS_RET_ENOENT;
        }
    }

    *p_out = cur;
    return PFS_ISOFS_RET_OK;
}

// end
