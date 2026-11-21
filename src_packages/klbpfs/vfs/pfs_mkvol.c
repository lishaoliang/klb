// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs_mkvol: isofs/udf 一次打包; 可写 FS 为 mkfs + 挂载灌入
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "pfs.h"
#include "pfs_compiler.h"
#include "base/pfs_mem.h"
#include "base/pfs_fs_ops.h"
#include "pfs_blkio.h"
#include "vfs/pfs_ctx.h"
#include "vfs/pfs_file.h"
#include "isofs/pfs_isofs.h"
#include "udf/pfs_udf.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#define PFS_MKVOL_PATH_MAX       1024
#define PFS_MKVOL_COPY_CHUNK     (64 * 1024)

static int pfs_mkvol_host_join(char *out, int out_cap, const char *dir, const char *name)
{
    if (NULL == out || out_cap <= 1 || NULL == dir || NULL == name)
    {
        return PFS_EINVAL;
    }

    int n = (int)strlen(dir);
    if (n <= 0)
    {
        return PFS_EINVAL;
    }
#ifdef _WIN32
    if (n + 1 + (int)strlen(name) + 1 > out_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    if ('\\' == dir[n - 1] || '/' == dir[n - 1])
    {
        (void)snprintf(out, (size_t)out_cap, "%s%s", dir, name);
    }
    else
    {
        (void)snprintf(out, (size_t)out_cap, "%s\\%s", dir, name);
    }
#else
    if (n + 1 + (int)strlen(name) + 1 > out_cap)
    {
        return PFS_ENAMETOOLONG;
    }
    if ('/' == dir[n - 1])
    {
        (void)snprintf(out, (size_t)out_cap, "%s%s", dir, name);
    }
    else
    {
        (void)snprintf(out, (size_t)out_cap, "%s/%s", dir, name);
    }
#endif
    return PFS_OK;
}

static int pfs_mkvol_pfs_join(char *out, int out_cap, const char *dir, const char *name)
{
    if (NULL == out || out_cap <= 1 || NULL == dir || NULL == name || '\0' == name[0])
    {
        return PFS_EINVAL;
    }

    int need;
    if (0 == strcmp(dir, "/"))
    {
        need = 1 + (int)strlen(name) + 1;
        if (need > out_cap)
        {
            return PFS_ENAMETOOLONG;
        }
        (void)snprintf(out, (size_t)out_cap, "/%s", name);
    }
    else
    {
        need = (int)strlen(dir) + 1 + (int)strlen(name) + 1;
        if (need > out_cap)
        {
            return PFS_ENAMETOOLONG;
        }
        (void)snprintf(out, (size_t)out_cap, "%s/%s", dir, name);
    }
    return PFS_OK;
}

#ifdef _WIN32
static int pfs_mkvol_utf8_to_wide(const char *utf8, wchar_t **pp_w)
{
    *pp_w = NULL;

    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (n <= 0)
    {
        return PFS_EILSEQ;
    }

    wchar_t *p_w = (wchar_t *)pfs_malloc((size_t)n * sizeof(wchar_t));
    if (NULL == p_w)
    {
        return PFS_ENOMEM;
    }
    if (0 == MultiByteToWideChar(CP_UTF8, 0, utf8, -1, p_w, n))
    {
        pfs_free(p_w);
        return PFS_EILSEQ;
    }
    *pp_w = p_w;
    return PFS_OK;
}

static int pfs_mkvol_wide_to_utf8(const wchar_t *wide, char *out, int out_cap)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, out_cap, NULL, NULL);
    if (n <= 0)
    {
        return PFS_EILSEQ;
    }
    return PFS_OK;
}
#endif

static int pfs_mkvol_host_is_dir(const char *path)
{
#ifdef _WIN32
    wchar_t *p_w;

    int rc = pfs_mkvol_utf8_to_wide(path, &p_w);
    if (rc < 0)
    {
        return rc;
    }

    DWORD attr = GetFileAttributesW(p_w);
    pfs_free(p_w);
    if (INVALID_FILE_ATTRIBUTES == attr)
    {
        return PFS_ENOENT;
    }
    if (0 == (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
#else
    struct stat st;

    if (0 != stat(path, &st))
    {
        return PFS_ENOENT;
    }
    if (!S_ISDIR(st.st_mode))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
#endif
}

// 同 pfs_fopen, 失败时透传 FS 错误码 (勿一律 EIO)
static int pfs_mkvol_fopen(pfs_ctx_t *p_ctx, const char *path, int mode, pfs_file_t **pp_fp)
{
    if (NULL == pp_fp)
    {
        return PFS_EINVAL;
    }
    *pp_fp = NULL;
    if (NULL == p_ctx || NULL == path || '\0' == path[0] || NULL == p_ctx->p_fs_ops ||
            NULL == p_ctx->p_sbi)
    {
        return PFS_EINVAL;
    }

    int rc = pfs_fs_path_check_path(p_ctx->p_fs_ops, p_ctx->p_sbi, path, PFS_PATH_CHECK_CREAT);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (!pfs_fs_super_can_write(p_ctx->p_fs_ops, p_ctx->p_sbi))
    {
        return PFS_EPERM;
    }

    int priv_size = p_ctx->p_fs_ops->file.priv_size;

    if (priv_size <= 0)
    {
        return PFS_EINVAL;
    }

    pfs_file_t *p_file = (pfs_file_t *)pfs_mallocz(sizeof(*p_file));
    if (NULL == p_file)
    {
        return PFS_ENOMEM;
    }
    p_file->p_fpriv = pfs_mallocz((size_t)priv_size);
    if (NULL == p_file->p_fpriv)
    {
        pfs_free(p_file);
        return PFS_ENOMEM;
    }
    p_file->p_ctx = p_ctx;
    p_file->mode = mode;
    p_file->pos = 0;
    p_file->err = PFS_OK;
    p_file->eof = 0;

    rc = pfs_fs_file_open(p_ctx->p_fs_ops, p_ctx->p_sbi, path, mode, p_file->p_fpriv);
    if (PFS_OK != rc)
    {
        pfs_free(p_file->p_fpriv);
        pfs_free(p_file);
        return rc;
    }

    int64_t sz = pfs_fs_file_get_size(p_ctx->p_fs_ops, p_ctx->p_sbi, p_file->p_fpriv);

    if (sz >= 0)
    {
        p_file->size = sz;
    }
    *pp_fp = p_file;
    return PFS_OK;
}

static int pfs_mkvol_copy_file(pfs_ctx_t *p_ctx, const char *host_path, const char *pfs_path,
        uint8_t *copy_buf)
{
    pfs_file_t *fp;
    int rc;

#ifdef _WIN32
    wchar_t *p_w;

    rc = pfs_mkvol_utf8_to_wide(host_path, &p_w);
    if (rc < 0)
    {
        return rc;
    }

    HANDLE h = CreateFileW(p_w, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
    pfs_free(p_w);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_ENOENT;
    }
#else
    int fd = open(host_path, O_RDONLY);

    if (fd < 0)
    {
        return PFS_ENOENT;
    }
#endif

    rc = pfs_mkvol_fopen(p_ctx, pfs_path, PFS_O_RDWR | PFS_O_CREAT, &fp);
    if (rc < 0)
    {
#ifdef _WIN32
        CloseHandle(h);
#else
        close(fd);
#endif
        return rc;
    }

    for (;;)
    {
#ifdef _WIN32
        DWORD nread;

        if (0 == ReadFile(h, copy_buf, PFS_MKVOL_COPY_CHUNK, &nread, NULL))
        {
            (void)pfs_fclose(fp);
            CloseHandle(h);
            return PFS_EIO;
        }
        if (0 == nread)
        {
            break;
        }

        int nw = pfs_fwrite(copy_buf, 1, (int)nread, fp);
        if (nw < 0)
        {
            rc = nw;
            (void)pfs_fclose(fp);
            CloseHandle(h);
            return rc;
        }
        if (nw != (int)nread)
        {
            (void)pfs_fclose(fp);
            CloseHandle(h);
            return PFS_ENOSPC;
        }
#else
        ssize_t nread_s = read(fd, copy_buf, PFS_MKVOL_COPY_CHUNK);

        if (nread_s < 0)
        {
            (void)pfs_fclose(fp);
            close(fd);
            return PFS_EIO;
        }
        if (0 == nread_s)
        {
            break;
        }

        int nw = pfs_fwrite(copy_buf, 1, (int)nread_s, fp);

        if (nw < 0)
        {
            rc = nw;
            (void)pfs_fclose(fp);
            close(fd);
            return rc;
        }
        if (nw != (int)nread_s)
        {
            (void)pfs_fclose(fp);
            close(fd);
            return PFS_ENOSPC;
        }
#endif
    }

    rc = pfs_fclose(fp);
#ifdef _WIN32
    CloseHandle(h);
#else
    close(fd);
#endif
    return rc;
}

static int pfs_mkvol_copy_dir(pfs_ctx_t *p_ctx, const char *host_dir, const char *pfs_dir,
        uint8_t *copy_buf);

#ifdef _WIN32
static int pfs_mkvol_copy_dir(pfs_ctx_t *p_ctx, const char *host_dir, const char *pfs_dir,
        uint8_t *copy_buf)
{
    wchar_t *p_wdir;

    int rc = pfs_mkvol_utf8_to_wide(host_dir, &p_wdir);
    if (rc < 0)
    {
        return rc;
    }

    wchar_t pattern[PFS_MKVOL_PATH_MAX + 8];

    (void)_snwprintf(pattern, (sizeof(pattern) / sizeof(pattern[0])) - 1, L"%s\\*", p_wdir);
    pattern[(sizeof(pattern) / sizeof(pattern[0])) - 1] = L'\0';
    pfs_free(p_wdir);

    WIN32_FIND_DATAW fd;

    HANDLE h = FindFirstFileW(pattern, &fd);
    if (INVALID_HANDLE_VALUE == h)
    {
        return PFS_ENOENT;
    }
    do {
        char name_utf8[PFS_NAME_MAX + 1];
        char host_child[PFS_MKVOL_PATH_MAX];
        char pfs_child[PFS_MKVOL_PATH_MAX];
        int is_dir;

        if (0 == wcscmp(fd.cFileName, L".") || 0 == wcscmp(fd.cFileName, L".."))
        {
            continue;
        }
        if (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            continue;
        }
        rc = pfs_mkvol_wide_to_utf8(fd.cFileName, name_utf8, (int)sizeof(name_utf8));
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        is_dir = (0 != (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) ? 1 : 0;
        rc = pfs_mkvol_host_join(host_child, (int)sizeof(host_child), host_dir, name_utf8);
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        rc = pfs_mkvol_pfs_join(pfs_child, (int)sizeof(pfs_child), pfs_dir, name_utf8);
        if (rc < 0)
        {
            FindClose(h);
            return rc;
        }
        if (is_dir)
        {
            rc = pfs_mkdir(p_ctx, pfs_child, 0);
            if (rc < 0)
            {
                FindClose(h);
                return rc;
            }
            rc = pfs_mkvol_copy_dir(p_ctx, host_child, pfs_child, copy_buf);
            if (rc < 0)
            {
                FindClose(h);
                return rc;
            }
        }
        else
        {
            rc = pfs_mkvol_copy_file(p_ctx, host_child, pfs_child, copy_buf);
            if (rc < 0)
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
static int pfs_mkvol_copy_dir(pfs_ctx_t *p_ctx, const char *host_dir, const char *pfs_dir,
        uint8_t *copy_buf)
{
    DIR *p_d;
    struct dirent *p_de;

    p_d = opendir(host_dir);
    if (NULL == p_d)
    {
        return PFS_ENOENT;
    }
    while (NULL != (p_de = readdir(p_d)))
    {
        char host_child[PFS_MKVOL_PATH_MAX];
        char pfs_child[PFS_MKVOL_PATH_MAX];

        if (0 == strcmp(p_de->d_name, ".") || 0 == strcmp(p_de->d_name, ".."))
        {
            continue;
        }

        int rc = pfs_mkvol_host_join(host_child, (int)sizeof(host_child), host_dir, p_de->d_name);
        if (rc < 0)
        {
            closedir(p_d);
            return rc;
        }

        struct stat st;

        if (0 != lstat(host_child, &st))
        {
            closedir(p_d);
            return PFS_EIO;
        }
        if (0 != S_ISLNK(st.st_mode))
        {
            continue;
        }

        int is_dir = S_ISDIR(st.st_mode) ? 1 : 0;
        if (!is_dir && !S_ISREG(st.st_mode))
        {
            continue;
        }
        rc = pfs_mkvol_pfs_join(pfs_child, (int)sizeof(pfs_child), pfs_dir, p_de->d_name);
        if (rc < 0)
        {
            closedir(p_d);
            return rc;
        }
        if (is_dir)
        {
            rc = pfs_mkdir(p_ctx, pfs_child, 0);
            if (rc < 0)
            {
                closedir(p_d);
                return rc;
            }
            rc = pfs_mkvol_copy_dir(p_ctx, host_child, pfs_child, copy_buf);
            if (rc < 0)
            {
                closedir(p_d);
                return rc;
            }
        }
        else
        {
            rc = pfs_mkvol_copy_file(p_ctx, host_child, pfs_child, copy_buf);
            if (rc < 0)
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

static int pfs_mkvol_fill_writable(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes,
        pfs_fs_type_e fs_type, const char *src_dir, const char *volume_id)
{
    (void)volume_id;

    if (pfs_unlikely(NULL == p_blkio || part_offset < 0 || NULL == src_dir ||
            '\0' == src_dir[0]))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_blkio_is_open(p_blkio)))
    {
        return PFS_EINVAL;
    }

    int rc = pfs_mkvol_host_is_dir(src_dir);
    if (rc < 0)
    {
        return rc;
    }

    rc = pfs_mkfs(p_blkio, part_offset, vol_bytes, fs_type);
    if (rc < 0)
    {
        return rc;
    }

    pfs_ctx_t *p_ctx = pfs_ctx_create();
    if (NULL == p_ctx)
    {
        return PFS_ENOMEM;
    }
    rc = pfs_ctx_mount(p_ctx, p_blkio, part_offset, PFS_MNT_RDWR);
    if (rc < 0)
    {
        pfs_ctx_destroy(p_ctx);
        return rc;
    }

    uint8_t *copy_buf = (uint8_t *)pfs_malloc(PFS_MKVOL_COPY_CHUNK);

    if (NULL == copy_buf)
    {
        (void)pfs_ctx_umount(p_ctx);
        pfs_ctx_destroy(p_ctx);
        return PFS_ENOMEM;
    }

    rc = pfs_mkvol_copy_dir(p_ctx, src_dir, "/", copy_buf);
    pfs_free(copy_buf);
    if (rc < 0)
    {
        (void)pfs_ctx_umount(p_ctx);
        pfs_ctx_destroy(p_ctx);
        return rc;
    }

    // 灌入后刷盘 (动态 VHD); 对齐 mkfs 末尾 fsync
    if (pfs_blkio_fsync(p_blkio) < 0)
    {
        (void)pfs_ctx_umount(p_ctx);
        pfs_ctx_destroy(p_ctx);
        return PFS_EIO;
    }

    rc = pfs_ctx_umount(p_ctx);
    pfs_ctx_destroy(p_ctx);
    return rc;
}

int pfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type,
        const char *src_dir, const pfs_mkvol_opts_t *opts)
{
    if (pfs_unlikely(NULL == p_blkio || part_offset < 0 || NULL == src_dir ||
            '\0' == src_dir[0]))
    {
        return PFS_EINVAL;
    }
    if (pfs_unlikely(!pfs_blkio_is_open(p_blkio)))
    {
        return PFS_EINVAL;
    }

    const char *volume_id = NULL;

    if (NULL != opts)
    {
        volume_id = opts->volume_id;
    }

    // 只读 / 自建映像栈: 一次打包 (isofs 按内容定长; vol_bytes 暂不参与布局)
    if (PFS_ISOFS == fs_type)
    {
        (void)vol_bytes;

        int rc = pfs_isofs_mkvol(p_blkio, part_offset, src_dir, volume_id,
                PFS_ISOFS_MKVOL_JOLIET_DEFAULT, PFS_ISOFS_MKVOL_ROCK_DEFAULT);
        if (rc < 0)
        {
            // isofs 错误码与 PFS_* 数值对齐
            return rc;
        }
        return PFS_OK;
    }
    if (PFS_UDF == fs_type)
    {
        int rc = pfs_udf_mkvol(p_blkio, part_offset, vol_bytes, src_dir, volume_id);
        if (rc < 0)
        {
            return rc;
        }
        return PFS_OK;
    }
    // 可写 FS: mkfs + 挂载灌入 (fat / exfat / ntfs / ext*)
    if (PFS_FAT12 == fs_type || PFS_FAT16 == fs_type || PFS_FAT32 == fs_type ||
            PFS_EXFAT == fs_type || PFS_NTFS == fs_type ||
            PFS_EXT2 == fs_type || PFS_EXT3 == fs_type || PFS_EXT4 == fs_type)
    {
        return pfs_mkvol_fill_writable(p_blkio, part_offset, vol_bytes, fs_type, src_dir, volume_id);
    }
    return PFS_EOPNOTSUPP;
}

// end
