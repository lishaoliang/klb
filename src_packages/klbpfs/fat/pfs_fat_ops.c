// Doc Encode : UTF-8 BOM, Unix(LF)
// FAT12/16/32 分表填 pfs_fs_ops_t (适配层; vfs 按表分派时使用)
// super 除 mkfs/fs_type/name 外共用; file/dir/path/ctrl 三表共享
#include "fat/pfs_fat.h"
#include "fat/pfs_fat_internal.h"
#include "base/pfs_fs_ops.h"
#include "base/pfs_path_check.h"

#include <string.h>

////////////////////////////////////////////////////////////
// 私有态

typedef struct _pfs_fat_fpriv_t {
    pfs_fat_file_meta_t     meta;
    pfs_fat_dir_entry_t     loc;                        // sync 回写定位
    int                     dir_loc_valid;
    int                     dirty_meta;
} pfs_fat_fpriv_t;

typedef struct _pfs_fat_dpriv_t {
    uint32_t                start_clu;
    int                     is_fixed_root;
    int64_t                 cpos;
} pfs_fat_dpriv_t;

////////////////////////////////////////////////////////////
// super

static int pfs_fat_ops_init(void *p_sbi)
{
    return pfs_fat_init((pfs_fat_sbi_t *)p_sbi);
}

static void pfs_fat_ops_deinit(void *p_sbi)
{
    pfs_fat_deinit((pfs_fat_sbi_t *)p_sbi);
}

static int pfs_fat_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_fat_probe(p_blkio, part_offset);
}

static int pfs_fat_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_fat_mount((pfs_fat_sbi_t *)p_sbi, p_blkio, part_offset, (pfs_fat_mount_state_e)mode);
}

static int pfs_fat_ops_remount(void *p_sbi, int mode)
{
    return pfs_fat_remount((pfs_fat_sbi_t *)p_sbi, (pfs_fat_mount_state_e)mode);
}

static int pfs_fat_ops_umount(void *p_sbi)
{
    return pfs_fat_umount((pfs_fat_sbi_t *)p_sbi);
}

static int pfs_fat_ops_is_mounted(const void *p_sbi)
{
    return pfs_fat_sbi_is_mounted((const pfs_fat_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_fat_ops_can_write(const void *p_sbi)
{
    return pfs_fat_sbi_can_write((const pfs_fat_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_fat_ops_mkfs12(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_fat_mkfs12(p_blkio, part_offset, vol_bytes);
}

static int pfs_fat_ops_mkfs16(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_fat_mkfs16(p_blkio, part_offset, vol_bytes);
}

static int pfs_fat_ops_mkfs32(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_fat_mkfs32(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file

static void pfs_fat_ops_fpriv_from_info(pfs_fat_fpriv_t *p_fp, const pfs_fat_dir_entry_t *p_info)
{
    memset(p_fp, 0, sizeof(*p_fp));
    p_fp->meta.start_clu = p_info->start_clu;
    p_fp->meta.size = (int64_t)p_info->size;
    p_fp->meta.hint_clu_off = 0;
    p_fp->meta.hint_clu = p_info->start_clu;
    p_fp->loc = *p_info;
    p_fp->dir_loc_valid = (p_info->sfn_cpos >= 0) ? 1 : 0;
    p_fp->dirty_meta = 0;
}

static int pfs_fat_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_sbi_t *p_fat = (pfs_fat_sbi_t *)p_sbi;
    pfs_fat_fpriv_t *p_fp = (pfs_fat_fpriv_t *)p_fpriv;
    if (!pfs_fat_sbi_is_mounted(p_fat))
    {
        return PFS_FAT_RET_EINVAL;
    }

    int want_write = (0 != (mode & PFS_O_RDWR)) ? 1 : 0;
    int want_creat = (0 != (mode & PFS_O_CREAT)) ? 1 : 0;
    if (want_write && !pfs_fat_sbi_can_write(p_fat))
    {
        return PFS_FAT_RET_EPERM;
    }

    pfs_fat_dir_entry_t info;
    int rc = pfs_fat_lookup_path(p_fat, path, &info);
    if (PFS_FAT_RET_OK == rc)
    {
        if (PFS_FAT_TYPE_FILE != info.type)
        {
            return PFS_FAT_RET_EINVAL;
        }
        pfs_fat_ops_fpriv_from_info(p_fp, &info);
        return PFS_FAT_RET_OK;
    }
    if (PFS_FAT_RET_ENOENT == rc && want_creat && want_write)
    {
        rc = pfs_fat_create_file(p_fat, path, &info);
        if (PFS_FAT_RET_OK != rc)
        {
            return rc;
        }
        pfs_fat_ops_fpriv_from_info(p_fp, &info);
        return PFS_FAT_RET_OK;
    }
    return rc;
}

static void pfs_fat_ops_file_close(void *p_sbi, void *p_fpriv)
{
    (void)p_sbi;
    if (NULL != p_fpriv)
    {
        memset(p_fpriv, 0, sizeof(pfs_fat_fpriv_t));
    }
}

static int pfs_fat_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_got || size < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_fpriv_t *p_fp = (pfs_fat_fpriv_t *)p_fpriv;
    return pfs_fat_file_read((pfs_fat_sbi_t *)p_sbi, &p_fp->meta, pos, buf, size, p_got);
}

static int pfs_fat_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_put || size < 0)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_fpriv_t *p_fp = (pfs_fat_fpriv_t *)p_fpriv;
    int rc = pfs_fat_file_write((pfs_fat_sbi_t *)p_sbi, &p_fp->meta, pos, buf, size, p_put);
    if (PFS_FAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 1;
    }
    return rc;
}

static int pfs_fat_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_fpriv_t *p_fp = (pfs_fat_fpriv_t *)p_fpriv;
    int rc = pfs_fat_file_truncate((pfs_fat_sbi_t *)p_sbi, &p_fp->meta, size);
    if (PFS_FAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 1;
    }
    return rc;
}

static int pfs_fat_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_fpriv_t *p_fp = (pfs_fat_fpriv_t *)p_fpriv;
    if (0 == p_fp->dirty_meta)
    {
        return PFS_FAT_RET_OK;
    }
    if (0 == p_fp->dir_loc_valid)
    {
        return PFS_FAT_RET_EINVAL;
    }
    int rc = pfs_fat_update_sfn_meta((pfs_fat_sbi_t *)p_sbi, &p_fp->loc,
            p_fp->meta.start_clu, (uint32_t)p_fp->meta.size);
    if (PFS_FAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 0;
        p_fp->loc.start_clu = p_fp->meta.start_clu;
        p_fp->loc.size = (uint32_t)p_fp->meta.size;
    }
    return rc;
}

static int64_t pfs_fat_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    (void)p_sbi;
    if (NULL == p_fpriv)
    {
        return -1;
    }

    const pfs_fat_fpriv_t *p_fp = (const pfs_fat_fpriv_t *)p_fpriv;
    return p_fp->meta.size;
}

////////////////////////////////////////////////////////////
// dir

static int pfs_fat_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_sbi_t *p_fat = (pfs_fat_sbi_t *)p_sbi;
    pfs_fat_dpriv_t *p_dp = (pfs_fat_dpriv_t *)p_dpriv;
    if (!pfs_fat_sbi_is_mounted(p_fat))
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_dir_entry_t info;
    int rc = pfs_fat_lookup_path(p_fat, path, &info);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_FAT_TYPE_DIR != info.type)
    {
        return PFS_FAT_RET_EINVAL;
    }
    memset(p_dp, 0, sizeof(*p_dp));
    p_dp->start_clu = info.start_clu;
    p_dp->is_fixed_root = info.is_fixed_root ? 1 : 0;
    p_dp->cpos = 0;
    return PFS_FAT_RET_OK;
}

static void pfs_fat_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    (void)p_sbi;
    if (NULL != p_dpriv)
    {
        memset(p_dpriv, 0, sizeof(pfs_fat_dpriv_t));
    }
}

static int pfs_fat_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_dpriv_t *p_dp = (pfs_fat_dpriv_t *)p_dpriv;
    memset(p_ent, 0, sizeof(*p_ent));
    pfs_fat_dir_entry_t de;
    int rc = pfs_fat_readdir((pfs_fat_sbi_t *)p_sbi, p_dp->start_clu, p_dp->is_fixed_root,
            &p_dp->cpos, &de, p_ent->d_name, (int)sizeof(p_ent->d_name));
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if ('\0' == p_ent->d_name[0])
    {
        return 0;
    }
    if (0 != (de.attr & FAT_ATTR_DIR))
    {
        p_ent->d_type = PFS_DT_DIR;
    }
    else
    {
        p_ent->d_type = PFS_DT_REG;
    }
    return 1;
}

////////////////////////////////////////////////////////////
// path

static int pfs_fat_ops_access(void *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_sbi_t *p_fat = (pfs_fat_sbi_t *)p_sbi;
    if (!pfs_fat_sbi_is_mounted(p_fat))
    {
        return PFS_FAT_RET_EINVAL;
    }

    pfs_fat_dir_entry_t info;
    int rc = pfs_fat_lookup_path(p_fat, path, &info);
    if (PFS_FAT_RET_OK != rc)
    {
        return rc;
    }
    if (0 != (mode & PFS_W_OK))
    {
        if (!pfs_fat_sbi_can_write(p_fat))
        {
            return PFS_FAT_RET_EPERM;
        }
        if (0 != (info.attr & FAT_ATTR_RO))
        {
            return PFS_FAT_RET_EPERM;
        }
    }
    return PFS_FAT_RET_OK;
}

static int pfs_fat_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    return pfs_fat_mkdir((pfs_fat_sbi_t *)p_sbi, path);
}

static int pfs_fat_ops_rmdir(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    return pfs_fat_rmdir((pfs_fat_sbi_t *)p_sbi, path);
}

static int pfs_fat_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_FAT_RET_EINVAL;
    }
    return pfs_fat_rename((pfs_fat_sbi_t *)p_sbi, oldpath, newpath);
}

static int pfs_fat_ops_unlink(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_FAT_RET_EINVAL;
    }
    return pfs_fat_unlink((pfs_fat_sbi_t *)p_sbi, path);
}

static int pfs_fat_ops_map_path_check(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_FAT_RET_OK;
    }
    if (PFS_ENAMETOOLONG == rc)
    {
        return PFS_FAT_RET_ENAMETOOLONG;
    }
    return PFS_FAT_RET_EINVAL;
}

static int pfs_fat_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfs_fat_ops_map_path_check(pfs_path_check_win_fat(path));
}

////////////////////////////////////////////////////////////
// 三张表: 仅 name / fs_type / mkfs 不同

const pfs_fs_ops_t g_pfs_fat12_fs_ops = {
    "fat12",
    PFS_FAT12,
    {
        (int)sizeof(pfs_fat_sbi_t),
        pfs_fat_ops_init,
        pfs_fat_ops_deinit,
        pfs_fat_ops_mount,
        pfs_fat_ops_remount,
        pfs_fat_ops_umount,
        pfs_fat_ops_is_mounted,
        pfs_fat_ops_can_write,
    },
    {
        (int)sizeof(pfs_fat_fpriv_t),
        pfs_fat_ops_file_open,
        pfs_fat_ops_file_close,
        pfs_fat_ops_file_read,
        pfs_fat_ops_file_write,
        pfs_fat_ops_file_truncate,
        pfs_fat_ops_file_fdatasync,
        pfs_fat_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_fat_dpriv_t),
        pfs_fat_ops_dir_open,
        pfs_fat_ops_dir_close,
        pfs_fat_ops_dir_readdir,
    },
    {
        pfs_fat_ops_access,
        pfs_fat_ops_mkdir,
        pfs_fat_ops_rmdir,
        pfs_fat_ops_rename,
        pfs_fat_ops_unlink,
        pfs_fat_ops_check_path,
    },
    {
        pfs_fat_ops_probe,
        pfs_fat_ops_mkfs12,
        NULL,
        NULL,
    },
};

const pfs_fs_ops_t g_pfs_fat16_fs_ops = {
    "fat16",
    PFS_FAT16,
    {
        (int)sizeof(pfs_fat_sbi_t),
        pfs_fat_ops_init,
        pfs_fat_ops_deinit,
        pfs_fat_ops_mount,
        pfs_fat_ops_remount,
        pfs_fat_ops_umount,
        pfs_fat_ops_is_mounted,
        pfs_fat_ops_can_write,
    },
    {
        (int)sizeof(pfs_fat_fpriv_t),
        pfs_fat_ops_file_open,
        pfs_fat_ops_file_close,
        pfs_fat_ops_file_read,
        pfs_fat_ops_file_write,
        pfs_fat_ops_file_truncate,
        pfs_fat_ops_file_fdatasync,
        pfs_fat_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_fat_dpriv_t),
        pfs_fat_ops_dir_open,
        pfs_fat_ops_dir_close,
        pfs_fat_ops_dir_readdir,
    },
    {
        pfs_fat_ops_access,
        pfs_fat_ops_mkdir,
        pfs_fat_ops_rmdir,
        pfs_fat_ops_rename,
        pfs_fat_ops_unlink,
        pfs_fat_ops_check_path,
    },
    {
        pfs_fat_ops_probe,
        pfs_fat_ops_mkfs16,
        NULL,
        NULL,
    },
};

const pfs_fs_ops_t g_pfs_fat32_fs_ops = {
    "fat32",
    PFS_FAT32,
    {
        (int)sizeof(pfs_fat_sbi_t),
        pfs_fat_ops_init,
        pfs_fat_ops_deinit,
        pfs_fat_ops_mount,
        pfs_fat_ops_remount,
        pfs_fat_ops_umount,
        pfs_fat_ops_is_mounted,
        pfs_fat_ops_can_write,
    },
    {
        (int)sizeof(pfs_fat_fpriv_t),
        pfs_fat_ops_file_open,
        pfs_fat_ops_file_close,
        pfs_fat_ops_file_read,
        pfs_fat_ops_file_write,
        pfs_fat_ops_file_truncate,
        pfs_fat_ops_file_fdatasync,
        pfs_fat_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_fat_dpriv_t),
        pfs_fat_ops_dir_open,
        pfs_fat_ops_dir_close,
        pfs_fat_ops_dir_readdir,
    },
    {
        pfs_fat_ops_access,
        pfs_fat_ops_mkdir,
        pfs_fat_ops_rmdir,
        pfs_fat_ops_rename,
        pfs_fat_ops_unlink,
        pfs_fat_ops_check_path,
    },
    {
        pfs_fat_ops_probe,
        pfs_fat_ops_mkfs32,
        NULL,
        NULL,
    },
};

// end
