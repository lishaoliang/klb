// Doc Encode : UTF-8 BOM, Unix(LF)
// NTFS 填 pfs_fs_ops_t (适配层; vfs 按表分派时使用)
#include "ntfs/pfs_ntfs.h"
#include "ntfs/pfs_ntfs_internal.h"
#include "base/pfs_fs_ops.h"
#include "base/pfs_path_check.h"

#include <string.h>

////////////////////////////////////////////////////////////
// super

static int pfs_ntfs_ops_init(void *p_sbi)
{
    return pfs_ntfs_init((pfs_ntfs_sbi_t *)p_sbi);
}

static void pfs_ntfs_ops_deinit(void *p_sbi)
{
    pfs_ntfs_deinit((pfs_ntfs_sbi_t *)p_sbi);
}

static int pfs_ntfs_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_ntfs_probe(p_blkio, part_offset);
}

static int pfs_ntfs_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_ntfs_mount((pfs_ntfs_sbi_t *)p_sbi, p_blkio, part_offset, (pfs_ntfs_mount_state_e)mode);
}

static int pfs_ntfs_ops_remount(void *p_sbi, int mode)
{
    return pfs_ntfs_remount((pfs_ntfs_sbi_t *)p_sbi, (pfs_ntfs_mount_state_e)mode);
}

static int pfs_ntfs_ops_umount(void *p_sbi)
{
    return pfs_ntfs_umount((pfs_ntfs_sbi_t *)p_sbi);
}

static int pfs_ntfs_ops_is_mounted(const void *p_sbi)
{
    return pfs_ntfs_sbi_is_mounted((const pfs_ntfs_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_ntfs_ops_can_write(const void *p_sbi)
{
    return pfs_ntfs_sbi_can_write((const pfs_ntfs_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_ntfs_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ntfs_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file (p_fpriv = pfs_ntfs_file_meta_t)

static int pfs_ntfs_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    pfs_ntfs_file_meta_t *p_meta = (pfs_ntfs_file_meta_t *)p_fpriv;
    if (!pfs_ntfs_sbi_is_mounted(p_nt))
    {
        return PFS_NTFS_RET_EINVAL;
    }

    int want_write = (0 != (mode & PFS_O_RDWR)) ? 1 : 0;
    int want_creat = (0 != (mode & PFS_O_CREAT)) ? 1 : 0;
    if (want_write && !pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }

    pfs_ntfs_file_meta_reset(p_meta);
    int rc = pfs_ntfs_file_open(p_nt, path, p_meta);
    if (PFS_NTFS_RET_OK == rc)
    {
        return PFS_NTFS_RET_OK;
    }
    if (PFS_NTFS_RET_ENOENT == rc && want_creat && want_write)
    {
        rc = pfs_ntfs_create_file(p_nt, path);
        if (PFS_NTFS_RET_OK != rc)
        {
            return rc;
        }
        pfs_ntfs_file_meta_reset(p_meta);
        return pfs_ntfs_file_open(p_nt, path, p_meta);
    }
    return rc;
}

static void pfs_ntfs_ops_file_close(void *p_sbi, void *p_fpriv)
{
    (void)p_sbi;
    if (NULL != p_fpriv)
    {
        pfs_ntfs_file_meta_reset((pfs_ntfs_file_meta_t *)p_fpriv);
    }
}

static int pfs_ntfs_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_got || size < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return pfs_ntfs_file_read((pfs_ntfs_sbi_t *)p_sbi, (pfs_ntfs_file_meta_t *)p_fpriv, pos, buf, size, p_got);
}

static int pfs_ntfs_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_put || size < 0)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return pfs_ntfs_file_write((pfs_ntfs_sbi_t *)p_sbi, (pfs_ntfs_file_meta_t *)p_fpriv, pos, buf, size, p_put);
}

static int pfs_ntfs_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return pfs_ntfs_file_truncate((pfs_ntfs_sbi_t *)p_sbi, (pfs_ntfs_file_meta_t *)p_fpriv, size);
}

static int pfs_ntfs_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    return pfs_ntfs_file_sync((pfs_ntfs_sbi_t *)p_sbi, (pfs_ntfs_file_meta_t *)p_fpriv);
}

static int64_t pfs_ntfs_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    (void)p_sbi;
    if (NULL == p_fpriv)
    {
        return -1;
    }
    const pfs_ntfs_file_meta_t *p_meta = (const pfs_ntfs_file_meta_t *)p_fpriv;
    if (!p_meta->valid)
    {
        return -1;
    }
    return p_meta->ino.data_size;
}

////////////////////////////////////////////////////////////
// dir (p_dpriv = pfs_ntfs_dir_meta_t)

static int pfs_ntfs_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    pfs_ntfs_dir_meta_t *p_meta = (pfs_ntfs_dir_meta_t *)p_dpriv;
    if (!pfs_ntfs_sbi_is_mounted(p_nt))
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_dir_meta_reset(p_meta);
    return pfs_ntfs_dir_open(p_nt, path, p_meta);
}

static void pfs_ntfs_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    (void)p_sbi;
    if (NULL != p_dpriv)
    {
        pfs_ntfs_dir_meta_reset((pfs_ntfs_dir_meta_t *)p_dpriv);
    }
}

static int pfs_ntfs_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_dir_meta_t *p_meta = (pfs_ntfs_dir_meta_t *)p_dpriv;
    memset(p_ent, 0, sizeof(*p_ent));
    uint8_t d_type;
    int rc = pfs_ntfs_readdir((pfs_ntfs_sbi_t *)p_sbi, p_meta, &d_type, p_ent->d_name,
            (int)sizeof(p_ent->d_name));
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    else if ('\0' == p_ent->d_name[0])
    {
        return 0;
    }
    p_ent->d_type = d_type;
    return 1;
}

////////////////////////////////////////////////////////////
// path

static int pfs_ntfs_ops_access(void *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    if (!pfs_ntfs_sbi_is_mounted(p_nt))
    {
        return PFS_NTFS_RET_EINVAL;
    }

    pfs_ntfs_lookup_t lu;
    int rc = pfs_ntfs_lookup_path(p_nt, path, &lu);
    if (PFS_NTFS_RET_OK != rc)
    {
        return rc;
    }
    if (0 != (mode & PFS_W_OK) && !pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }
    return PFS_NTFS_RET_OK;
}

static int pfs_ntfs_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    if (!pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }
    return pfs_ntfs_mkdir(p_nt, path);
}

static int pfs_ntfs_ops_rmdir(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    if (!pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }
    return pfs_ntfs_rmdir(p_nt, path);
}

static int pfs_ntfs_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    if (!pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }
    return pfs_ntfs_rename(p_nt, oldpath, newpath);
}

static int pfs_ntfs_ops_unlink(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_NTFS_RET_EINVAL;
    }
    pfs_ntfs_sbi_t *p_nt = (pfs_ntfs_sbi_t *)p_sbi;
    if (!pfs_ntfs_sbi_can_write(p_nt))
    {
        return PFS_NTFS_RET_EPERM;
    }
    return pfs_ntfs_unlink(p_nt, path);
}

static int pfs_ntfs_ops_map_path_check(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_NTFS_RET_OK;
    }
    if (PFS_ENAMETOOLONG == rc)
    {
        return PFS_NTFS_RET_ENAMETOOLONG;
    }
    return PFS_NTFS_RET_EINVAL;
}

static int pfs_ntfs_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfs_ntfs_ops_map_path_check(pfs_path_check_win_ntfs(path));
}

////////////////////////////////////////////////////////////

const pfs_fs_ops_t g_pfs_ntfs_fs_ops = {
    "ntfs",
    PFS_NTFS,
    {
        (int)sizeof(pfs_ntfs_sbi_t),
        pfs_ntfs_ops_init,
        pfs_ntfs_ops_deinit,
        pfs_ntfs_ops_mount,
        pfs_ntfs_ops_remount,
        pfs_ntfs_ops_umount,
        pfs_ntfs_ops_is_mounted,
        pfs_ntfs_ops_can_write,
    },
    {
        (int)sizeof(pfs_ntfs_file_meta_t),
        pfs_ntfs_ops_file_open,
        pfs_ntfs_ops_file_close,
        pfs_ntfs_ops_file_read,
        pfs_ntfs_ops_file_write,
        pfs_ntfs_ops_file_truncate,
        pfs_ntfs_ops_file_fdatasync,
        pfs_ntfs_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_ntfs_dir_meta_t),
        pfs_ntfs_ops_dir_open,
        pfs_ntfs_ops_dir_close,
        pfs_ntfs_ops_dir_readdir,
    },
    {
        pfs_ntfs_ops_access,
        pfs_ntfs_ops_mkdir,
        pfs_ntfs_ops_rmdir,
        pfs_ntfs_ops_rename,
        pfs_ntfs_ops_unlink,
        pfs_ntfs_ops_check_path,
    },
    {
        pfs_ntfs_ops_probe,
        pfs_ntfs_ops_mkfs,
        NULL,
        NULL,
    },
};

// end
