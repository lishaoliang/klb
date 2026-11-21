// Doc Encode : UTF-8 BOM, Unix(LF)
// 路径/组件名校验实现
#include "base/pfs_path_check.h"
#include <stddef.h>
#include <stdbool.h>

static bool pfs_path_check_ok(const char *path)
{
    if ((NULL == path) || ('\0' == path[0]))
    {
        return false;
    }
    return true;
}

static bool pfs_path_component_has_bad_char(const char *s, int len)
{
    for (int i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)s[i];

        if (c < 0x20u)
        {
            return true;
        }
        // Windows / FAT/exFAT 非法名: " * / : < > ? \ |
        if ('"' == c || '*' == c || '/' == c || ':' == c ||
                '<' == c || '>' == c || '?' == c || '\\' == c || '|' == c)
        {
            return true;
        }
    }
    return false;
}

static int pfs_path_foreach_component(const char *path,
        int (*fn)(const char *start, int len, void *ctx), void *ctx)
{
    if (!pfs_path_check_ok(path))
    {
        return PFS_EINVAL;
    }

    const char *p = path;
    while ('/' == *p)
    {
        p++;
    }
    while ('\0' != *p)
    {
        const char *start = p;
        int len = 0;

        while ('\0' != *p && '/' != *p)
        {
            p++;
            len++;
        }
        if (len > 0)
        {
            int rc = fn(start, len, ctx);
            if (PFS_OK != rc)
            {
                return rc;
            }
        }
        while ('/' == *p)
        {
            p++;
        }
    }
    return PFS_OK;
}

static int pfs_path_check_bad_char_cb(const char *start, int len, void *ctx)
{
    if (pfs_path_component_has_bad_char(start, len))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

static int pfs_path_check_len_cb(const char *start, int len, void *ctx)
{
    if (len > PFS_NAME_MAX)
    {
        return PFS_ENAMETOOLONG;
    }
    return PFS_OK;
}

static int pfs_path_check_colon_cb(const char *start, int len, void *ctx)
{
    for (int i = 0; i < len; i++)
    {
        if (':' == start[i])
        {
            return PFS_EINVAL;
        }
    }
    return PFS_OK;
}

int pfs_path_check_win_names(const char *path)
{
    return pfs_path_foreach_component(path, pfs_path_check_bad_char_cb, NULL);
}

int pfs_path_check_name_max(const char *path)
{
    return pfs_path_foreach_component(path, pfs_path_check_len_cb, NULL);
}

int pfs_path_check_win_fat(const char *path)
{
    int rc = pfs_path_check_win_names(path);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_path_check_name_max(path);
}

int pfs_path_check_win_ntfs(const char *path)
{
    int rc = pfs_path_check_win_names(path);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_path_foreach_component(path, pfs_path_check_colon_cb, NULL);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_path_check_name_max(path);
}

// end
