// Doc Encode : UTF-8 BOM, Unix(LF)
// pfs_fs_ops 虚接口封装 (调表内回调; 不依赖 fat/exfat/ntfs)
#include "base/pfs_fs_ops.h"
#include "pfs_compiler.h"

////////////////////////////////////////////////////////////
// 将子模块 RET_* / 同类负码映到 pfs_ret_e

static int pfs_fs_ops_map_ret(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_OK;
    }
    else if (pfs_unlikely((PFS_EPERM == rc) || (PFS_ENOENT == rc) ||
            (PFS_EIO == rc) || (PFS_ENOEXEC == rc) ||
            (PFS_ENOMEM == rc) || (PFS_EBUSY == rc) ||
            (PFS_EEXIST == rc) || (PFS_ENOTDIR == rc) ||
            (PFS_EISDIR == rc) || (PFS_EINVAL == rc) ||
            (PFS_ENOSPC == rc) || (PFS_EROFS == rc) ||
            (PFS_ERANGE == rc) || (PFS_ENAMETOOLONG == rc) ||
            (PFS_ENOTEMPTY == rc) || (PFS_EBADMSG == rc) ||
            (PFS_EILSEQ == rc) || (PFS_EOPNOTSUPP == rc)))
    {
        return rc;
    }
    return PFS_EIO;
}

////////////////////////////////////////////////////////////
// super

int pfs_fs_super_init(const pfs_fs_ops_t *p_fs_ops, void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->super.init)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->super.init(p_sbi));
}

void pfs_fs_super_deinit(const pfs_fs_ops_t *p_fs_ops, void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fs_ops->super.deinit)
    {
        return;
    }
    p_fs_ops->super.deinit(p_sbi);
}

int pfs_fs_super_mount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_blkio)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->super.mount)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->super.mount(p_sbi, p_blkio, part_offset, mode));
}

int pfs_fs_super_remount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, int mode)
{
    if (NULL == p_fs_ops || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->super.remount)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->super.remount(p_sbi, mode));
}

int pfs_fs_super_umount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->super.umount)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->super.umount(p_sbi));
}

int pfs_fs_super_is_mounted(const pfs_fs_ops_t *p_fs_ops, const void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fs_ops->super.is_mounted)
    {
        return 0;
    }
    return p_fs_ops->super.is_mounted(p_sbi) ? 1 : 0;
}

int pfs_fs_super_can_write(const pfs_fs_ops_t *p_fs_ops, const void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fs_ops->super.can_write)
    {
        return 0;
    }
    return p_fs_ops->super.can_write(p_sbi) ? 1 : 0;
}

////////////////////////////////////////////////////////////
// ctrl

int pfs_fs_ctrl_probe(const pfs_fs_ops_t *p_fs_ops, pfs_blkio_t *p_blkio, int64_t part_offset)
{
    if (NULL == p_fs_ops || NULL == p_blkio)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->ctrl.probe)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->ctrl.probe(p_blkio, part_offset));
}

int pfs_fs_ctrl_mkfs(const pfs_fs_ops_t *p_fs_ops, pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    if (NULL == p_fs_ops || NULL == p_blkio)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->ctrl.mkfs)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->ctrl.mkfs(p_blkio, part_offset, vol_bytes));
}

int pfs_fs_ctrl_fsync(const pfs_fs_ops_t *p_fs_ops, void *p_sbi)
{
    if (NULL == p_fs_ops || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->ctrl.fsync)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->ctrl.fsync(p_sbi));
}

int pfs_fs_ctrl_ioctl(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, uint32_t cmd, void *arg, int arg_size)
{
    if (NULL == p_fs_ops || NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->ctrl.ioctl)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->ctrl.ioctl(p_sbi, cmd, arg, arg_size));
}

////////////////////////////////////////////////////////////
// file

int pfs_fs_file_open(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->file.open)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->file.open(p_sbi, path, mode, p_fpriv));
}

void pfs_fs_file_close(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv)
{
    if (NULL == p_fs_ops || NULL == p_fs_ops->file.close)
    {
        return;
    }
    p_fs_ops->file.close(p_sbi, p_fpriv);
}

int pfs_fs_file_read(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_got)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->file.read)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->file.read(p_sbi, p_fpriv, pos, buf, size, p_got));
}

int pfs_fs_file_write(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_put)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->file.write)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->file.write(p_sbi, p_fpriv, pos, buf, size, p_put));
}

int pfs_fs_file_truncate(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->file.truncate)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->file.truncate(p_sbi, p_fpriv, size));
}

int pfs_fs_file_fdatasync(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->file.fdatasync)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->file.fdatasync(p_sbi, p_fpriv));
}

int64_t pfs_fs_file_get_size(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const void *p_fpriv)
{
    if (NULL == p_fs_ops || NULL == p_fpriv || NULL == p_fs_ops->file.get_size)
    {
        return -1;
    }
    return p_fs_ops->file.get_size(p_sbi, p_fpriv);
}

////////////////////////////////////////////////////////////
// dir

int pfs_fs_dir_open(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->dir.open)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->dir.open(p_sbi, path, p_dpriv));
}

void pfs_fs_dir_close(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_dpriv)
{
    if (NULL == p_fs_ops || NULL == p_fs_ops->dir.close)
    {
        return;
    }
    p_fs_ops->dir.close(p_sbi, p_dpriv);
}

int pfs_fs_dir_readdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->dir.readdir)
    {
        return PFS_EOPNOTSUPP;
    }

    int rc = p_fs_ops->dir.readdir(p_sbi, p_dpriv, p_ent);
    if (rc < 0)
    {
        return pfs_fs_ops_map_ret(rc);
    }
    return rc;
}

////////////////////////////////////////////////////////////
// path

int pfs_fs_path_access(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.access)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.access(p_sbi, path, mode));
}

int pfs_fs_path_mkdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.mkdir)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.mkdir(p_sbi, path, mode));
}

int pfs_fs_path_rmdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.rmdir)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.rmdir(p_sbi, path));
}

int pfs_fs_path_rename(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.rename)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.rename(p_sbi, oldpath, newpath));
}

int pfs_fs_path_unlink(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path)
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.unlink)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.unlink(p_sbi, path));
}

int pfs_fs_path_check_path(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, uint32_t flags)
{
    if (NULL == p_fs_ops || NULL == p_sbi || NULL == path || ('\0' == path[0]))
    {
        return PFS_EINVAL;
    }
    if (NULL == p_fs_ops->path.check_path)
    {
        return PFS_OK;
    }
    return pfs_fs_ops_map_ret(p_fs_ops->path.check_path(p_sbi, path, flags));
}

// end
