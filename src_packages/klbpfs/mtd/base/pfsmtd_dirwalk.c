// Doc Encode : UTF-8 BOM, Unix(LF)
// 宿主目录遍历 (供各目标 FS mkvol 灌树)
#include "base/pfsmtd_dirwalk.h"
#include "base/pfsmtd_mem.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#endif

static int pfsmtd_dirwalk_join(char *out, int out_cap, const char *a, const char *b)
{
    if (NULL == out || out_cap <= 0 || NULL == a || NULL == b)
    {
        return PFS_EINVAL;
    }
    int la = (int)strlen(a);
    int lb = (int)strlen(b);
    bool need_slash = false;
    if (la > 0)
    {
#ifdef _WIN32
        if ('/' != a[la - 1] && '\\' != a[la - 1])
        {
            need_slash = true;
        }
#else
        if ('/' != a[la - 1])
        {
            need_slash = true;
        }
#endif
    }
    if (la + need_slash + lb + 1 > out_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(out, a, (size_t)la);
    if (need_slash)
    {
#ifdef _WIN32
        out[la++] = '\\';
#else
        out[la++] = '/';
#endif
    }
    memcpy(out + la, b, (size_t)lb + 1);
    return PFS_OK;
}

static int pfsmtd_dirwalk_vfs_join(char *out, int out_cap, const char *parent_rel, const char *name)
{
    if (NULL == out || out_cap < 2 || NULL == parent_rel || NULL == name)
    {
        return PFS_EINVAL;
    }
    if ('\0' == parent_rel[0] || (1 == (int)strlen(parent_rel) && '/' == parent_rel[0]))
    {
        int ln = (int)strlen(name);
        if (ln + 2 > out_cap)
        {
            return PFS_ENAMETOOLONG;
        }
        out[0] = '/';
        memcpy(out + 1, name, (size_t)ln + 1);
        return PFS_OK;
    }
    int lp = (int)strlen(parent_rel);
    int ln = (int)strlen(name);
    if (lp + 1 + ln + 1 > out_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    memcpy(out, parent_rel, (size_t)lp);
    out[lp] = '/';
    memcpy(out + lp + 1, name, (size_t)ln + 1);
    return PFS_OK;
}

#ifdef _WIN32
static int pfsmtd_utf8_to_wide(const char *utf8, wchar_t **p_out)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0)
    {
        return PFS_EINVAL;
    }
    wchar_t *w = (wchar_t *)pfsmtd_malloc((size_t)n * sizeof(wchar_t));
    if (NULL == w)
    {
        return PFS_ENOMEM;
    }
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w, n) <= 0)
    {
        pfsmtd_free(w);
        return PFS_EINVAL;
    }
    *p_out = w;
    return PFS_OK;
}

static int pfsmtd_wide_to_utf8(const wchar_t *w, char *out, int out_cap)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, out_cap, NULL, NULL);
    if (n <= 0 || n > out_cap)
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

static int pfsmtd_dirwalk_rec(const char *host_dir, const char *vfs_rel, pfsmtd_dirwalk_fn fn, void *ctx)
{
    wchar_t *p_wdir;
    int rc = pfsmtd_utf8_to_wide(host_dir, &p_wdir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    wchar_t pattern[PFSMTD_DIRWALK_PATH_MAX + 8];
    (void)_snwprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, L"%s\\*", p_wdir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = L'\0';
    pfsmtd_free(p_wdir);

    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern, &fd);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_ENOENT;
    }
    do
    {
        char name_utf8[PFSMTD_DIRWALK_NAME_MAX + 1];
        char child_host[PFSMTD_DIRWALK_PATH_MAX];
        char child_vfs[PFSMTD_DIRWALK_PATH_MAX];
        bool is_dir;

        if (0 == wcscmp(fd.cFileName, L".") || 0 == wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;
        }
        rc = pfsmtd_wide_to_utf8(fd.cFileName, name_utf8, (int)sizeof(name_utf8));
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        is_dir = (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
        rc = pfsmtd_dirwalk_join(child_host, (int)sizeof(child_host), host_dir, name_utf8);
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        rc = pfsmtd_dirwalk_vfs_join(child_vfs, (int)sizeof(child_vfs), vfs_rel, name_utf8);
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        rc = fn(ctx, child_vfs, child_host, is_dir);
        if (PFS_OK != rc)
        {
            FindClose(h);
            return rc;
        }
        if (is_dir)
        {
            rc = pfsmtd_dirwalk_rec(child_host, child_vfs, fn, ctx);
            if (PFS_OK != rc)
            {
                FindClose(h);
                return rc;
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return PFS_OK;
}
#else
static int pfsmtd_dirwalk_rec(const char *host_dir, const char *vfs_rel, pfsmtd_dirwalk_fn fn, void *ctx)
{
    DIR *p_d;
    struct dirent *p_ent;

    p_d = opendir(host_dir);
    if (NULL == p_d)
    {
        return PFS_ENOENT;
    }
    while (NULL != (p_ent = readdir(p_d)))
    {
        char child_host[PFSMTD_DIRWALK_PATH_MAX];
        char child_vfs[PFSMTD_DIRWALK_PATH_MAX];

        if (0 == strcmp(p_ent->d_name, ".") || 0 == strcmp(p_ent->d_name, ".."))
        {
            continue;
        }
        if ((int)strlen(p_ent->d_name) > PFSMTD_DIRWALK_NAME_MAX)
        {
            closedir(p_d);
            return PFS_ENAMETOOLONG;
        }
        int rc = pfsmtd_dirwalk_join(child_host, (int)sizeof(child_host), host_dir, p_ent->d_name);
        if (PFS_OK != rc)
        {
            closedir(p_d);
            return rc;
        }
        struct stat st;
        if (0 != lstat(child_host, &st))
        {
            closedir(p_d);
            return PFS_EIO;
        }
        if (S_ISLNK(st.st_mode))
        {
            continue;
        }
        bool is_dir = S_ISDIR(st.st_mode);
        if (!is_dir && !S_ISREG(st.st_mode))
        {
            continue;
        }
        rc = pfsmtd_dirwalk_vfs_join(child_vfs, (int)sizeof(child_vfs), vfs_rel, p_ent->d_name);
        if (PFS_OK != rc)
        {
            closedir(p_d);
            return rc;
        }
        rc = fn(ctx, child_vfs, child_host, is_dir);
        if (PFS_OK != rc)
        {
            closedir(p_d);
            return rc;
        }
        if (is_dir)
        {
            rc = pfsmtd_dirwalk_rec(child_host, child_vfs, fn, ctx);
            if (PFS_OK != rc)
            {
                closedir(p_d);
                return rc;
            }
        }
    }
    closedir(p_d);
    return PFS_OK;
}
#endif

int pfsmtd_dirwalk(const char *src_dir, pfsmtd_dirwalk_fn fn, void *ctx)
{
    if (NULL == src_dir || '\0' == src_dir[0] || NULL == fn)
    {
        return PFS_EINVAL;
    }
    return pfsmtd_dirwalk_rec(src_dir, "/", fn, ctx);
}

int pfsmtd_host_read_file(const char *host_path, uint8_t **p_buf, uint32_t *p_size)
{
    if (NULL == host_path || NULL == p_buf || NULL == p_size)
    {
        return PFS_EINVAL;
    }
    *p_buf = NULL;
    *p_size = 0;

    FILE *fp;
#ifdef _WIN32
    {
        wchar_t *wpath;
        int rc = pfsmtd_utf8_to_wide(host_path, &wpath);
        if (PFS_OK != rc)
        {
            return rc;
        }
        fp = _wfopen(wpath, L"rb");
        pfsmtd_free(wpath);
    }
#else
    fp = fopen(host_path, "rb");
#endif
    if (NULL == fp)
    {
        return PFS_ENOENT;
    }
    if (0 != fseek(fp, 0, SEEK_END))
    {
        fclose(fp);
        return PFS_EIO;
    }
    long sz = ftell(fp);
    if (sz < 0 || sz > 0x7fffffffL)
    {
        fclose(fp);
        return PFS_EIO;
    }
    if (0 != fseek(fp, 0, SEEK_SET))
    {
        fclose(fp);
        return PFS_EIO;
    }
    uint8_t *buf = (uint8_t *)pfsmtd_malloc((size_t)sz + 1u);
    if (NULL == buf)
    {
        fclose(fp);
        return PFS_ENOMEM;
    }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    if (n != (size_t)sz)
    {
        pfsmtd_free(buf);
        return PFS_EIO;
    }
    *p_buf = buf;
    *p_size = (uint32_t)sz;
    return PFS_OK;
}

// end
