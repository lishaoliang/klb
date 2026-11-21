// Doc Encode : UTF-8 BOM, Unix(LF)
// exFAT 填 pfs_fs_ops_t (适配层; vfs 按表分派时使用)
#include "exfat/pfs_exfat.h"
#include "exfat/pfs_exfat_internal.h"
#include "base/pfs_fs_ops.h"
#include "base/pfs_path_check.h"

#include <string.h>

////////////////////////////////////////////////////////////
// 私有态 (vfs 按 priv_size 提供缓冲)

typedef struct _pfs_exfat_fpriv_t {
    pfs_exfat_file_meta_t   meta;
    pfs_exfat_dir_entry_t   loc;                        // sync 回写定位
    int                     dir_loc_valid;
    int                     dirty_meta;
} pfs_exfat_fpriv_t;

typedef struct _pfs_exfat_dpriv_t {
    pfs_exfat_chain_t       chain;
    int64_t                 cpos;
} pfs_exfat_dpriv_t;

////////////////////////////////////////////////////////////
// super

static int pfs_exfat_ops_init(void *p_sbi)
{
    return pfs_exfat_init((pfs_exfat_sbi_t *)p_sbi);
}

static void pfs_exfat_ops_deinit(void *p_sbi)
{
    pfs_exfat_deinit((pfs_exfat_sbi_t *)p_sbi);
}

static int pfs_exfat_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_exfat_probe(p_blkio, part_offset);
}

static int pfs_exfat_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_exfat_mount((pfs_exfat_sbi_t *)p_sbi, p_blkio, part_offset, (pfs_exfat_mount_state_e)mode);
}

static int pfs_exfat_ops_remount(void *p_sbi, int mode)
{
    return pfs_exfat_remount((pfs_exfat_sbi_t *)p_sbi, (pfs_exfat_mount_state_e)mode);
}

static int pfs_exfat_ops_umount(void *p_sbi)
{
    return pfs_exfat_umount((pfs_exfat_sbi_t *)p_sbi);
}

static int pfs_exfat_ops_is_mounted(const void *p_sbi)
{
    return pfs_exfat_sbi_is_mounted((const pfs_exfat_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_exfat_ops_can_write(const void *p_sbi)
{
    return pfs_exfat_sbi_can_write((const pfs_exfat_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_exfat_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_exfat_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file

static void pfs_exfat_ops_fpriv_from_info(pfs_exfat_fpriv_t *p_fp, const pfs_exfat_dir_entry_t *p_info)
{
    memset(p_fp, 0, sizeof(*p_fp));
    p_fp->meta.start_clu = p_info->start_clu;
    p_fp->meta.flags = p_info->flags;
    p_fp->meta.size = p_info->size;
    p_fp->meta.valid_size = p_info->valid_size;
    p_fp->meta.hint_clu_off = 0;
    p_fp->meta.hint_clu = p_info->start_clu;
    p_fp->loc = *p_info;
    p_fp->dir_loc_valid = 1;
    p_fp->dirty_meta = 0;
}

static int pfs_exfat_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_sbi_t *p_ex = (pfs_exfat_sbi_t *)p_sbi;
    pfs_exfat_fpriv_t *p_fp = (pfs_exfat_fpriv_t *)p_fpriv;
    if (!pfs_exfat_sbi_is_mounted(p_ex))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    int want_write = (0 != (mode & PFS_O_RDWR)) ? 1 : 0;
    int want_creat = (0 != (mode & PFS_O_CREAT)) ? 1 : 0;
    if (want_write && !pfs_exfat_sbi_can_write(p_ex))
    {
        return PFS_EXFAT_RET_EPERM;
    }

    pfs_exfat_dir_entry_t info;
    int rc = pfs_exfat_lookup_path(p_ex, path, &info);
    if (PFS_EXFAT_RET_OK == rc)
    {
        if (PFS_EXFAT_TYPE_FILE != info.type)
        {
            return PFS_EXFAT_RET_EINVAL;
        }
        pfs_exfat_ops_fpriv_from_info(p_fp, &info);
        return PFS_EXFAT_RET_OK;
    }
    if (PFS_EXFAT_RET_ENOENT == rc && want_creat && want_write)
    {
        rc = pfs_exfat_create_file(p_ex, path, &info);
        if (PFS_EXFAT_RET_OK != rc)
        {
            return rc;
        }
        pfs_exfat_ops_fpriv_from_info(p_fp, &info);
        return PFS_EXFAT_RET_OK;
    }
    return rc;
}

static void pfs_exfat_ops_file_close(void *p_sbi, void *p_fpriv)
{
    (void)p_sbi;
    if (NULL != p_fpriv)
    {
        memset(p_fpriv, 0, sizeof(pfs_exfat_fpriv_t));
    }
}

static int pfs_exfat_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_got || size < 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_fpriv_t *p_fp = (pfs_exfat_fpriv_t *)p_fpriv;
    return pfs_exfat_file_read((pfs_exfat_sbi_t *)p_sbi, &p_fp->meta, pos, buf, size, p_got);
}

static int pfs_exfat_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_fpriv || NULL == buf || NULL == p_put || size < 0)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_fpriv_t *p_fp = (pfs_exfat_fpriv_t *)p_fpriv;
    int rc = pfs_exfat_file_write((pfs_exfat_sbi_t *)p_sbi, &p_fp->meta, pos, buf, size, p_put);
    if (PFS_EXFAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 1;
    }
    return rc;
}

static int pfs_exfat_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_fpriv_t *p_fp = (pfs_exfat_fpriv_t *)p_fpriv;
    int rc = pfs_exfat_file_truncate((pfs_exfat_sbi_t *)p_sbi, &p_fp->meta, size);
    if (PFS_EXFAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 1;
    }
    return rc;
}

static int pfs_exfat_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_fpriv_t *p_fp = (pfs_exfat_fpriv_t *)p_fpriv;
    if (0 == p_fp->dirty_meta)
    {
        return PFS_EXFAT_RET_OK;
    }
    if (0 == p_fp->dir_loc_valid)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    int rc = pfs_exfat_update_stream_meta((pfs_exfat_sbi_t *)p_sbi, &p_fp->loc,
            p_fp->meta.start_clu, p_fp->meta.flags, p_fp->meta.size, p_fp->meta.valid_size);
    if (PFS_EXFAT_RET_OK == rc)
    {
        p_fp->dirty_meta = 0;
        p_fp->loc.start_clu = p_fp->meta.start_clu;
        p_fp->loc.flags = p_fp->meta.flags;
        p_fp->loc.size = p_fp->meta.size;
        p_fp->loc.valid_size = p_fp->meta.valid_size;
    }
    return rc;
}

static int64_t pfs_exfat_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    (void)p_sbi;
    if (NULL == p_fpriv)
    {
        return -1;
    }

    const pfs_exfat_fpriv_t *p_fp = (const pfs_exfat_fpriv_t *)p_fpriv;
    return p_fp->meta.size;
}

////////////////////////////////////////////////////////////
// dir

static int pfs_exfat_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_sbi_t *p_ex = (pfs_exfat_sbi_t *)p_sbi;
    pfs_exfat_dpriv_t *p_dp = (pfs_exfat_dpriv_t *)p_dpriv;
    if (!pfs_exfat_sbi_is_mounted(p_ex))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_dir_entry_t info;
    int rc = pfs_exfat_lookup_path(p_ex, path, &info);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_EXFAT_TYPE_DIR != info.type)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    // 链几何对齐 vfs pfs_dir_set_exfat_from_info
    {
        const pfs_exfat_bs_t *p_bs = PFS_EXFAT_SBI_BS(p_ex);

        memset(p_dp, 0, sizeof(*p_dp));
        p_dp->chain.dir = info.start_clu;
        p_dp->chain.flags = info.flags;
        if (0 == info.size && info.start_clu == p_bs->root_dir_clu)
        {
            p_dp->chain.size = p_bs->num_clusters;
            p_dp->chain.flags = EXFAT_ALLOC_FAT_CHAIN;
        }
        else if (0 == info.size)
        {
            p_dp->chain.size = 0;
        }
        else
        {
            p_dp->chain.size = PFS_EXFAT_B_TO_CLU_ROUND_UP((uint64_t)info.size, p_bs);
        }
        p_dp->cpos = 0;
    }
    return PFS_EXFAT_RET_OK;
}

static void pfs_exfat_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    (void)p_sbi;
    if (NULL != p_dpriv)
    {
        memset(p_dpriv, 0, sizeof(pfs_exfat_dpriv_t));
    }
}

static int pfs_exfat_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    if (NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_dpriv_t *p_dp = (pfs_exfat_dpriv_t *)p_dpriv;
    memset(p_ent, 0, sizeof(*p_ent));
    pfs_exfat_dir_entry_t de;
    int rc = pfs_exfat_readdir((pfs_exfat_sbi_t *)p_sbi, &p_dp->chain, 0, &p_dp->cpos, &de,
            p_ent->d_name, (int)sizeof(p_ent->d_name));
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if ('\0' == p_ent->d_name[0])
    {
        return 0;
    }
    if (0 != (de.attr & EXFAT_ATTR_SUBDIR))
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

static int pfs_exfat_ops_access(void *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_sbi_t *p_ex = (pfs_exfat_sbi_t *)p_sbi;
    if (!pfs_exfat_sbi_is_mounted(p_ex))
    {
        return PFS_EXFAT_RET_EINVAL;
    }

    pfs_exfat_dir_entry_t info;
    int rc = pfs_exfat_lookup_path(p_ex, path, &info);
    if (PFS_EXFAT_RET_OK != rc)
    {
        return rc;
    }
    if (0 != (mode & PFS_W_OK))
    {
        if (!pfs_exfat_sbi_can_write(p_ex))
        {
            return PFS_EXFAT_RET_EPERM;
        }
        if (0 != (info.attr & EXFAT_ATTR_READONLY))
        {
            return PFS_EXFAT_RET_EPERM;
        }
    }
    return PFS_EXFAT_RET_OK;
}

static int pfs_exfat_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return pfs_exfat_mkdir((pfs_exfat_sbi_t *)p_sbi, path);
}

static int pfs_exfat_ops_rmdir(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return pfs_exfat_rmdir((pfs_exfat_sbi_t *)p_sbi, path);
}

static int pfs_exfat_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    if (NULL == p_sbi || NULL == oldpath || NULL == newpath)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return pfs_exfat_rename((pfs_exfat_sbi_t *)p_sbi, oldpath, newpath);
}

static int pfs_exfat_ops_unlink(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_EXFAT_RET_EINVAL;
    }
    return pfs_exfat_unlink((pfs_exfat_sbi_t *)p_sbi, path);
}

static int pfs_exfat_ops_map_path_check(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_EXFAT_RET_OK;
    }
    if (PFS_ENAMETOOLONG == rc)
    {
        return PFS_EXFAT_RET_ENAMETOOLONG;
    }
    return PFS_EXFAT_RET_EINVAL;
}

static int pfs_exfat_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfs_exfat_ops_map_path_check(pfs_path_check_win_fat(path));
}

////////////////////////////////////////////////////////////

const pfs_fs_ops_t g_pfs_exfat_fs_ops = {
    "exfat",
    PFS_EXFAT,
    {
        (int)sizeof(pfs_exfat_sbi_t),
        pfs_exfat_ops_init,
        pfs_exfat_ops_deinit,
        pfs_exfat_ops_mount,
        pfs_exfat_ops_remount,
        pfs_exfat_ops_umount,
        pfs_exfat_ops_is_mounted,
        pfs_exfat_ops_can_write,
    },
    {
        (int)sizeof(pfs_exfat_fpriv_t),
        pfs_exfat_ops_file_open,
        pfs_exfat_ops_file_close,
        pfs_exfat_ops_file_read,
        pfs_exfat_ops_file_write,
        pfs_exfat_ops_file_truncate,
        pfs_exfat_ops_file_fdatasync,
        pfs_exfat_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_exfat_dpriv_t),
        pfs_exfat_ops_dir_open,
        pfs_exfat_ops_dir_close,
        pfs_exfat_ops_dir_readdir,
    },
    {
        pfs_exfat_ops_access,
        pfs_exfat_ops_mkdir,
        pfs_exfat_ops_rmdir,
        pfs_exfat_ops_rename,
        pfs_exfat_ops_unlink,
        pfs_exfat_ops_check_path,
    },
    {
        pfs_exfat_ops_probe,
        pfs_exfat_ops_mkfs,
        NULL,
        NULL,
    },
};

// end
