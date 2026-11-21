// Doc Encode : UTF-8 BOM, Unix(LF)
// ext2 / ext3 / ext4 填 pfs_fs_ops_t (适配层; vfs 按表分派; 回调与 sbi 共用)
#include "ext4/pfs_ext4.h"
#include "ext4/pfs_ext4_internal.h"
#include "base/pfs_fs_ops.h"
#include "base/pfs_path_check.h"
#include "pfs.h"

#include <string.h>

////////////////////////////////////////////////////////////
// super

static int pfs_ext4_ops_init(void *p_sbi)
{
    return pfs_ext4_init((pfs_ext4_sbi_t *)p_sbi);
}

static void pfs_ext4_ops_deinit(void *p_sbi)
{
    pfs_ext4_deinit((pfs_ext4_sbi_t *)p_sbi);
}

static int pfs_ext4_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_ext4_probe(p_blkio, part_offset);
}

static int pfs_ext4_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_ext4_mount((pfs_ext4_sbi_t *)p_sbi, p_blkio, part_offset, (pfs_ext4_mount_state_e)mode);
}

static int pfs_ext4_ops_remount(void *p_sbi, int mode)
{
    return pfs_ext4_remount((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_mount_state_e)mode);
}

static int pfs_ext4_ops_umount(void *p_sbi)
{
    return pfs_ext4_umount((pfs_ext4_sbi_t *)p_sbi);
}

static int pfs_ext4_ops_is_mounted(const void *p_sbi)
{
    return pfs_ext4_sbi_is_mounted((const pfs_ext4_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_ext4_ops_can_write(const void *p_sbi)
{
    return pfs_ext4_sbi_can_write((const pfs_ext4_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_ext2_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext2_mkfs(p_blkio, part_offset, vol_bytes);
}

static int pfs_ext3_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext3_mkfs(p_blkio, part_offset, vol_bytes);
}

static int pfs_ext4_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_ext4_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file (p_fpriv = pfs_ext4_file_meta_t)

static int pfs_ext4_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_is_mounted((const pfs_ext4_sbi_t *)p_sbi))
    {
        return PFS_EXT4_RET_EINVAL;
    }
    memset(p_fpriv, 0, sizeof(pfs_ext4_file_meta_t));
    return pfs_ext4_file_open((pfs_ext4_sbi_t *)p_sbi, path, mode, (pfs_ext4_file_meta_t *)p_fpriv);
}

static void pfs_ext4_ops_file_close(void *p_sbi, void *p_fpriv)
{
    (void)p_sbi;
    if (NULL != p_fpriv)
    {
        memset(p_fpriv, 0, sizeof(pfs_ext4_file_meta_t));
    }
}

static int pfs_ext4_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size,
        int *p_got)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_file_read((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_file_meta_t *)p_fpriv,
            pos, buf, size, p_got);
}

static int pfs_ext4_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf,
        int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_file_write((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_file_meta_t *)p_fpriv,
            pos, buf, size, p_put);
}

static int pfs_ext4_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_file_truncate((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_file_meta_t *)p_fpriv, size);
}

static int pfs_ext4_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_file_sync((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_file_meta_t *)p_fpriv);
}

static int pfs_ext4_ops_fsync(void *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_sync_sb((pfs_ext4_sbi_t *)p_sbi);
}

static int64_t pfs_ext4_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    (void)p_sbi;
    if (NULL == p_fpriv)
    {
        return (int64_t)PFS_EXT4_RET_EINVAL;
    }
    const pfs_ext4_file_meta_t *p_fm = (const pfs_ext4_file_meta_t *)p_fpriv;
    return (int64_t)p_fm->size;
}

////////////////////////////////////////////////////////////
// dir (p_dpriv = pfs_ext4_dir_meta_t)

static int pfs_ext4_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    pfs_ext4_sbi_t *p_ext = (pfs_ext4_sbi_t *)p_sbi;
    pfs_ext4_dir_meta_t *p_dm = (pfs_ext4_dir_meta_t *)p_dpriv;
    if (!pfs_ext4_sbi_is_mounted(p_ext))
    {
        return PFS_EXT4_RET_EINVAL;
    }

    pfs_ext4_inode_raw_t raw;
    uint32_t ino;
    int rc = pfs_ext4_path_lookup(p_ext, path, &ino, &raw);
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_S_IFDIR != (raw.i_mode & PFS_EXT4_S_IFMT))
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (0u != (raw.i_flags & PFS_EXT4_INDEX_FL))
    {
        return PFS_EXT4_RET_EOPNOTSUPP;
    }

    memset(p_dm, 0, sizeof(*p_dm));
    p_dm->ino = ino;
    p_dm->size = pfs_ext4_inode_size(&raw);
    p_dm->flags = raw.i_flags;
    memcpy(p_dm->i_block, raw.i_block, sizeof(p_dm->i_block));
    p_dm->pos = 0;
    return PFS_EXT4_RET_OK;
}

static void pfs_ext4_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    (void)p_sbi;
    if (NULL != p_dpriv)
    {
        memset(p_dpriv, 0, sizeof(pfs_ext4_dir_meta_t));
    }
}

static int pfs_ext4_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    memset(p_ent, 0, sizeof(*p_ent));
    pfs_ext4_type_e typ;
    int rc = pfs_ext4_dir_readdir((pfs_ext4_sbi_t *)p_sbi, (pfs_ext4_dir_meta_t *)p_dpriv,
            p_ent->d_name, (int)sizeof(p_ent->d_name), &typ, NULL);
    if (PFS_EXT4_RET_ENOENT == rc)
    {
        return 0;
    }
    if (PFS_EXT4_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXT4_TYPE_DIR == typ)
    {
        p_ent->d_type = PFS_DT_DIR;
    }
    else if (PFS_EXT4_TYPE_FILE == typ)
    {
        p_ent->d_type = PFS_DT_REG;
    }
    else
    {
        p_ent->d_type = PFS_DT_UNKNOWN;
    }
    return 1;
}

////////////////////////////////////////////////////////////
// path

static int pfs_ext4_ops_access(void *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    return pfs_ext4_access((pfs_ext4_sbi_t *)p_sbi, path, mode);
}

static int pfs_ext4_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write((const pfs_ext4_sbi_t *)p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    return pfs_ext4_mkdir((pfs_ext4_sbi_t *)p_sbi, path);
}

static int pfs_ext4_ops_rmdir(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write((const pfs_ext4_sbi_t *)p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    return pfs_ext4_rmdir((pfs_ext4_sbi_t *)p_sbi, path);
}

static int pfs_ext4_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write((const pfs_ext4_sbi_t *)p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    return pfs_ext4_rename((pfs_ext4_sbi_t *)p_sbi, oldpath, newpath);
}

static int pfs_ext4_ops_unlink(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXT4_RET_EINVAL;
    }
    if (!pfs_ext4_sbi_can_write((const pfs_ext4_sbi_t *)p_sbi))
    {
        return PFS_EXT4_RET_EPERM;
    }
    return pfs_ext4_unlink((pfs_ext4_sbi_t *)p_sbi, path);
}

static int pfs_ext4_ops_map_path_check(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_EXT4_RET_OK;
    }
    if (PFS_ENAMETOOLONG == rc)
    {
        return PFS_EXT4_RET_ENAMETOOLONG;
    }
    return PFS_EXT4_RET_EINVAL;
}

static int pfs_ext4_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfs_ext4_ops_map_path_check(pfs_path_check_name_max(path));
}

////////////////////////////////////////////////////////////
// 三张表: 仅 name / fs_type / ctrl.mkfs 不同; 回调与 sbi 共用

const pfs_fs_ops_t g_pfs_ext2_fs_ops = {
    "ext2",
    PFS_EXT2,
    {
        (int)sizeof(pfs_ext4_sbi_t),
        pfs_ext4_ops_init,
        pfs_ext4_ops_deinit,
        pfs_ext4_ops_mount,
        pfs_ext4_ops_remount,
        pfs_ext4_ops_umount,
        pfs_ext4_ops_is_mounted,
        pfs_ext4_ops_can_write,
    },
    {
        (int)sizeof(pfs_ext4_file_meta_t),
        pfs_ext4_ops_file_open,
        pfs_ext4_ops_file_close,
        pfs_ext4_ops_file_read,
        pfs_ext4_ops_file_write,
        pfs_ext4_ops_file_truncate,
        pfs_ext4_ops_file_fdatasync,
        pfs_ext4_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_ext4_dir_meta_t),
        pfs_ext4_ops_dir_open,
        pfs_ext4_ops_dir_close,
        pfs_ext4_ops_dir_readdir,
    },
    {
        pfs_ext4_ops_access,
        pfs_ext4_ops_mkdir,
        pfs_ext4_ops_rmdir,
        pfs_ext4_ops_rename,
        pfs_ext4_ops_unlink,
        pfs_ext4_ops_check_path,
    },
    {
        pfs_ext4_ops_probe,
        pfs_ext2_ops_mkfs,
        pfs_ext4_ops_fsync,
        NULL,
    },
};

const pfs_fs_ops_t g_pfs_ext3_fs_ops = {
    "ext3",
    PFS_EXT3,
    {
        (int)sizeof(pfs_ext4_sbi_t),
        pfs_ext4_ops_init,
        pfs_ext4_ops_deinit,
        pfs_ext4_ops_mount,
        pfs_ext4_ops_remount,
        pfs_ext4_ops_umount,
        pfs_ext4_ops_is_mounted,
        pfs_ext4_ops_can_write,
    },
    {
        (int)sizeof(pfs_ext4_file_meta_t),
        pfs_ext4_ops_file_open,
        pfs_ext4_ops_file_close,
        pfs_ext4_ops_file_read,
        pfs_ext4_ops_file_write,
        pfs_ext4_ops_file_truncate,
        pfs_ext4_ops_file_fdatasync,
        pfs_ext4_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_ext4_dir_meta_t),
        pfs_ext4_ops_dir_open,
        pfs_ext4_ops_dir_close,
        pfs_ext4_ops_dir_readdir,
    },
    {
        pfs_ext4_ops_access,
        pfs_ext4_ops_mkdir,
        pfs_ext4_ops_rmdir,
        pfs_ext4_ops_rename,
        pfs_ext4_ops_unlink,
        pfs_ext4_ops_check_path,
    },
    {
        pfs_ext4_ops_probe,
        pfs_ext3_ops_mkfs,
        pfs_ext4_ops_fsync,
        NULL,
    },
};

const pfs_fs_ops_t g_pfs_ext4_fs_ops = {
    "ext4",
    PFS_EXT4,
    {
        (int)sizeof(pfs_ext4_sbi_t),
        pfs_ext4_ops_init,
        pfs_ext4_ops_deinit,
        pfs_ext4_ops_mount,
        pfs_ext4_ops_remount,
        pfs_ext4_ops_umount,
        pfs_ext4_ops_is_mounted,
        pfs_ext4_ops_can_write,
    },
    {
        (int)sizeof(pfs_ext4_file_meta_t),
        pfs_ext4_ops_file_open,
        pfs_ext4_ops_file_close,
        pfs_ext4_ops_file_read,
        pfs_ext4_ops_file_write,
        pfs_ext4_ops_file_truncate,
        pfs_ext4_ops_file_fdatasync,
        pfs_ext4_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_ext4_dir_meta_t),
        pfs_ext4_ops_dir_open,
        pfs_ext4_ops_dir_close,
        pfs_ext4_ops_dir_readdir,
    },
    {
        pfs_ext4_ops_access,
        pfs_ext4_ops_mkdir,
        pfs_ext4_ops_rmdir,
        pfs_ext4_ops_rename,
        pfs_ext4_ops_unlink,
        pfs_ext4_ops_check_path,
    },
    {
        pfs_ext4_ops_probe,
        pfs_ext4_ops_mkfs,
        pfs_ext4_ops_fsync,
        NULL,
    },
};


// end
