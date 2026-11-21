// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs 路径级 API (access/mkdir/rmdir/rename/remove/unlink; 经 pfs_fs_ops)
#include "pfs.h"
#include "pfs_compiler.h"
#include "base/pfs_mem.h"
#include "vfs/pfs_ctx.h"

#include <stdbool.h>

/// @brief 跨 ctx rename 拷贝块大小
#define PFS_PATH_COPY_CHUNK         4096

static bool pfs_path_ctx_mounted(const pfs_ctx_t *p_ctx)
{
    if (pfs_unlikely(NULL == p_ctx))
    {
        return false;
    }
    return (PFS_NONE != p_ctx->fs_type && NULL != p_ctx->p_fs_ops && NULL != p_ctx->p_sbi);
}

static bool pfs_path_ok(const char *path)
{
    if (pfs_unlikely((NULL == path) || ('\0' == path[0])))
    {
        return false;
    }
    return true;
}

static int pfs_path_validate(const pfs_ctx_t *p_ctx, const char *path, uint32_t flags)
{
    if (!pfs_path_ctx_mounted(p_ctx) || !pfs_path_ok(path))
    {
        return PFS_EINVAL;
    }
    return pfs_fs_path_check_path(p_ctx->p_fs_ops, p_ctx->p_sbi, path, flags);
}

static bool pfs_path_access_mode_ok(int mode)
{
    // 仅允许 F_OK 或 R/W/X 位组合
    if (0 == mode)
    {
        return true;
    }
    if (0 != (mode & ~(PFS_R_OK | PFS_W_OK | PFS_X_OK)))
    {
        return false;
    }
    return true;
}

int pfs_access(pfs_ctx_t *p_ctx, const char *path, int mode)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_ctx) || !pfs_path_ok(path) ||
            !pfs_path_access_mode_ok(mode)))
    {
        return PFS_EINVAL;
    }
    return pfs_fs_path_access(p_ctx->p_fs_ops, p_ctx->p_sbi, path, mode);
}

int pfs_mkdir(pfs_ctx_t *p_ctx, const char *path, int mode)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_ctx) || !pfs_path_ok(path)))
    {
        return PFS_EINVAL;
    }
    int rc = pfs_path_validate(p_ctx, path, PFS_PATH_CHECK_CREAT);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_fs_path_mkdir(p_ctx->p_fs_ops, p_ctx->p_sbi, path, mode);
}

int pfs_rmdir(pfs_ctx_t *p_ctx, const char *path)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_ctx) || !pfs_path_ok(path)))
    {
        return PFS_EINVAL;
    }
    return pfs_fs_path_rmdir(p_ctx->p_fs_ops, p_ctx->p_sbi, path);
}

int pfs_rename(pfs_ctx_t *p_ctx, const char *oldpath, const char *newpath)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_ctx) ||
            !pfs_path_ok(oldpath) || !pfs_path_ok(newpath)))
    {
        return PFS_EINVAL;
    }
    int rc = pfs_path_validate(p_ctx, newpath, PFS_PATH_CHECK_CREAT);
    if (PFS_OK != rc)
    {
        return rc;
    }
    return pfs_fs_path_rename(p_ctx->p_fs_ops, p_ctx->p_sbi, oldpath, newpath);
}

static bool pfs_path_is_dir(pfs_ctx_t *p_ctx, const char *path)
{
    pfs_dir_t *p_dir = pfs_opendir(p_ctx, path);

    if (NULL == p_dir)
    {
        return false;
    }

    pfs_closedir(p_dir);
    return true;
}

static int pfs_path_copy_file(pfs_ctx_t *p_src_ctx, const char *src_path,
        pfs_ctx_t *p_dst_ctx, const char *dst_path)
{
    pfs_file_t *p_src = pfs_fopen(p_src_ctx, src_path, PFS_O_RD);

    if (NULL == p_src)
    {
        return PFS_ENOENT;
    }

    pfs_file_t *p_dst = pfs_fopen(p_dst_ctx, dst_path, PFS_O_RDWR | PFS_O_CREAT);

    if (NULL == p_dst)
    {
        pfs_fclose(p_src);
        return PFS_EIO;
    }

    uint8_t *buf = (uint8_t *)pfs_malloc(PFS_PATH_COPY_CHUNK);

    int rc = PFS_OK;

    for (;;)
    {
        int nread = pfs_fread(buf, 1, PFS_PATH_COPY_CHUNK, p_src);
        if (nread < 0)
        {
            rc = pfs_ferror(p_src);
            break;
        }
        if (0 == nread)
        {
            if (0 != pfs_ferror(p_src))
            {
                rc = pfs_ferror(p_src);
            }
            break;
        }

        int nwrite = pfs_fwrite(buf, 1, nread, p_dst);
        if (nwrite < 0)
        {
            rc = pfs_ferror(p_dst);
            break;
        }
        if (nwrite != nread)
        {
            rc = PFS_EIO;
            break;
        }
    }

    pfs_free(buf);
    pfs_fclose(p_src);
    if (PFS_OK != rc)
    {
        pfs_fclose(p_dst);
        (void)pfs_unlink(p_dst_ctx, dst_path);
        return rc;
    }

    rc = pfs_fclose(p_dst);
    if (PFS_OK != rc)
    {
        (void)pfs_unlink(p_dst_ctx, dst_path);
        return rc;
    }

    return PFS_OK;
}

int pfs_rename_cross(pfs_ctx_t *p_old_ctx, const char *oldpath,
        pfs_ctx_t *p_new_ctx, const char *newpath)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_old_ctx) || !pfs_path_ctx_mounted(p_new_ctx) ||
            !pfs_path_ok(oldpath) || !pfs_path_ok(newpath)))
    {
        return PFS_EINVAL;
    }

    if (p_old_ctx == p_new_ctx)
    {
        return pfs_rename(p_old_ctx, oldpath, newpath);
    }

    int rc = pfs_access(p_old_ctx, oldpath, PFS_F_OK);
    if (PFS_OK != rc)
    {
        return rc;
    }

    if (pfs_path_is_dir(p_old_ctx, oldpath))
    {
        return PFS_EISDIR;
    }

    rc = pfs_access(p_new_ctx, newpath, PFS_F_OK);
    if (PFS_OK == rc)
    {
        return PFS_EEXIST;
    }
    else if (PFS_ENOENT != rc)
    {
        return rc;
    }

    rc = pfs_path_copy_file(p_old_ctx, oldpath, p_new_ctx, newpath);
    if (PFS_OK != rc)
    {
        return rc;
    }

    rc = pfs_unlink(p_old_ctx, oldpath);
    if (PFS_OK != rc)
    {
        (void)pfs_unlink(p_new_ctx, newpath);
        return rc;
    }

    return PFS_OK;
}

int pfs_remove(pfs_ctx_t *p_ctx, const char *path)
{
    // POSIX remove: 先 unlink; 目录再 rmdir (空目录)
    int rc = pfs_unlink(p_ctx, path);
    if (PFS_OK == rc)
    {
        return PFS_OK;
    }
    // 目录: fat/exfat 常 EPERM; ntfs 常 EINVAL
    if (PFS_EPERM == rc || PFS_EINVAL == rc)
    {
        int rc_dir = pfs_rmdir(p_ctx, path);
        if (PFS_OK == rc_dir)
        {
            return PFS_OK;
        }
        if (PFS_ENOTEMPTY == rc_dir)
        {
            return rc_dir;
        }
    }
    return rc;
}

int pfs_unlink(pfs_ctx_t *p_ctx, const char *path)
{
    if (pfs_unlikely(!pfs_path_ctx_mounted(p_ctx) || !pfs_path_ok(path)))
    {
        return PFS_EINVAL;
    }
    return pfs_fs_path_unlink(p_ctx->p_fs_ops, p_ctx->p_sbi, path);
}

// end
