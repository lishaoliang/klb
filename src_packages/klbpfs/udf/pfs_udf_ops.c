// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF 填 pfs_fs_ops_t (Type1+SBD 卷可 RDWR; VAT/Metadata 仍 RO)
#include "udf/pfs_udf.h"
#include "udf/pfs_udf_internal.h"
#include "base/pfs_fs_ops.h"
#include "base/pfs_path_check.h"

#include <string.h>

////////////////////////////////////////////////////////////
// super

static int pfs_udf_ops_init(void *p_sbi)
{
    return pfs_udf_init((pfs_udf_sbi_t *)p_sbi);
}

static void pfs_udf_ops_deinit(void *p_sbi)
{
    pfs_udf_deinit((pfs_udf_sbi_t *)p_sbi);
}

static int pfs_udf_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_udf_probe(p_blkio, part_offset);
}

static int pfs_udf_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_udf_mount((pfs_udf_sbi_t *)p_sbi, p_blkio, part_offset,
            (pfs_udf_mount_state_e)mode);
}

static int pfs_udf_ops_remount(void *p_sbi, int mode)
{
    return pfs_udf_remount((pfs_udf_sbi_t *)p_sbi, (pfs_udf_mount_state_e)mode);
}

static int pfs_udf_ops_umount(void *p_sbi)
{
    return pfs_udf_umount((pfs_udf_sbi_t *)p_sbi);
}

static int pfs_udf_ops_is_mounted(const void *p_sbi)
{
    return pfs_udf_sbi_is_mounted((const pfs_udf_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_udf_ops_can_write(const void *p_sbi)
{
    return pfs_udf_sbi_can_write((const pfs_udf_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_udf_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_udf_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file

static int pfs_udf_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fpriv)
    {
        return PFS_UDF_RET_EINVAL;
    }
    pfs_udf_sbi_t *p_udf = (pfs_udf_sbi_t *)p_sbi;
    pfs_udf_file_meta_t *p_fm = (pfs_udf_file_meta_t *)p_fpriv;
    if (!pfs_udf_sbi_is_mounted(p_udf))
    {
        return PFS_UDF_RET_EINVAL;
    }

    int creat = (0 != (mode & PFS_O_CREAT));
    if (0 != (mode & PFS_O_RDWR))
    {
        if (!pfs_udf_sbi_can_write(p_udf))
        {
            return PFS_UDF_RET_EPERM;
        }
    }
    else if (0 != (mode & (PFS_O_CREAT)))
    {
        return PFS_UDF_RET_EPERM;
    }

    pfs_udf_inode_t info;
    int rc = pfs_udf_lookup_path(p_udf, path, &info);
    if (PFS_UDF_RET_ENOENT == rc && creat && (mode & PFS_O_RDWR))
    {
        rc = pfs_udf_create_file(p_udf, path, &info);
    }
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_UDF_TYPE_FILE != info.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    memset(p_fm, 0, sizeof(*p_fm));
    p_fm->inode = info;
    return PFS_UDF_RET_OK;
}

static void pfs_udf_ops_file_close(void *p_sbi, void *p_fpriv)
{
    (void)p_sbi;
    if (NULL != p_fpriv)
    {
        memset(p_fpriv, 0, sizeof(pfs_udf_file_meta_t));
    }
}

static int pfs_udf_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size,
        int *p_got)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_UDF_RET_EINVAL;
    }
    return pfs_udf_file_read((pfs_udf_sbi_t *)p_sbi, (const pfs_udf_file_meta_t *)p_fpriv,
            pos, buf, size, p_got);
}

static int pfs_udf_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf,
        int size, int *p_put)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_UDF_RET_EINVAL;
    }
    return pfs_udf_file_write((pfs_udf_sbi_t *)p_sbi, (pfs_udf_file_meta_t *)p_fpriv,
            pos, buf, size, p_put);
}

static int pfs_udf_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    if (NULL == p_sbi || NULL == p_fpriv)
    {
        return PFS_UDF_RET_EINVAL;
    }
    return pfs_udf_file_truncate((pfs_udf_sbi_t *)p_sbi, (pfs_udf_file_meta_t *)p_fpriv, size);
}

static int pfs_udf_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    (void)p_fpriv;
    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write((pfs_udf_sbi_t *)p_sbi))
    {
        return PFS_UDF_RET_OK;
    }
    return pfs_udf_bitmap_flush((pfs_udf_sbi_t *)p_sbi);
}

static int pfs_udf_ops_fsync(void *p_sbi)
{
    if (NULL == p_sbi)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write((pfs_udf_sbi_t *)p_sbi))
    {
        return PFS_UDF_RET_OK;
    }
    return pfs_udf_bitmap_flush((pfs_udf_sbi_t *)p_sbi);
}

static int64_t pfs_udf_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    (void)p_sbi;
    if (NULL == p_fpriv)
    {
        return (int64_t)PFS_UDF_RET_EINVAL;
    }
    const pfs_udf_file_meta_t *p_fm = (const pfs_udf_file_meta_t *)p_fpriv;
    return (int64_t)p_fm->inode.size;
}

////////////////////////////////////////////////////////////
// dir

static int pfs_udf_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    if (NULL == p_sbi || NULL == path || NULL == p_dpriv)
    {
        return PFS_UDF_RET_EINVAL;
    }
    pfs_udf_sbi_t *p_udf = (pfs_udf_sbi_t *)p_sbi;
    pfs_udf_dir_meta_t *p_dm = (pfs_udf_dir_meta_t *)p_dpriv;
    if (!pfs_udf_sbi_is_mounted(p_udf))
    {
        return PFS_UDF_RET_EINVAL;
    }

    pfs_udf_inode_t info;
    int rc = pfs_udf_lookup_path(p_udf, path, &info);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    if (PFS_UDF_TYPE_DIR != info.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    memset(p_dm, 0, sizeof(*p_dm));
    p_dm->dir = info;
    p_dm->pos = 0;
    return PFS_UDF_RET_OK;
}

static void pfs_udf_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    (void)p_sbi;
    if (NULL != p_dpriv)
    {
        memset(p_dpriv, 0, sizeof(pfs_udf_dir_meta_t));
    }
}

static int pfs_udf_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{

    if (NULL == p_sbi || NULL == p_dpriv || NULL == p_ent)
    {
        return PFS_UDF_RET_EINVAL;
    }
    int rc = pfs_udf_dir_read_next((pfs_udf_sbi_t *)p_sbi, (pfs_udf_dir_meta_t *)p_dpriv, p_ent);
    if (0 > rc)
    {
        return rc;
    }
    if ('\0' == p_ent->d_name[0])
    {
        return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////
// path

static int pfs_udf_ops_access(void *p_sbi, const char *path, int mode)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (0 != (mode & PFS_W_OK))
    {
        if (!pfs_udf_sbi_can_write((const pfs_udf_sbi_t *)p_sbi))
        {
            return PFS_UDF_RET_EPERM;
        }
    }
    pfs_udf_inode_t info;
    int rc = pfs_udf_lookup_path((pfs_udf_sbi_t *)p_sbi, path, &info);
    if (PFS_UDF_RET_OK != rc)
    {
        return rc;
    }
    (void)info;
    return PFS_UDF_RET_OK;
}

static int pfs_udf_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    (void)mode;
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write((const pfs_udf_sbi_t *)p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }
    return pfs_udf_mkdir((pfs_udf_sbi_t *)p_sbi, path);
}

static int pfs_udf_ops_rmdir(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write((const pfs_udf_sbi_t *)p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }
    return pfs_udf_rmdir((pfs_udf_sbi_t *)p_sbi, path);
}

static int pfs_udf_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    (void)p_sbi;
    (void)oldpath;
    (void)newpath;
    return PFS_UDF_RET_EPERM;
}

static int pfs_udf_ops_unlink(void *p_sbi, const char *path)
{
    if (NULL == p_sbi || NULL == path)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_can_write((const pfs_udf_sbi_t *)p_sbi))
    {
        return PFS_UDF_RET_EPERM;
    }
    return pfs_udf_unlink((pfs_udf_sbi_t *)p_sbi, path);
}

static int pfs_udf_ops_map_path_check(int rc)
{
    if (PFS_OK == rc)
    {
        return PFS_UDF_RET_OK;
    }
    if (PFS_ENAMETOOLONG == rc)
    {
        return PFS_UDF_RET_ENAMETOOLONG;
    }
    return PFS_UDF_RET_EINVAL;
}

static int pfs_udf_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfs_udf_ops_map_path_check(pfs_path_check_win_fat(path));
}

////////////////////////////////////////////////////////////

const pfs_fs_ops_t g_pfs_udf_fs_ops = {
    "udf",
    PFS_UDF,
    {
        (int)sizeof(pfs_udf_sbi_t),
        pfs_udf_ops_init,
        pfs_udf_ops_deinit,
        pfs_udf_ops_mount,
        pfs_udf_ops_remount,
        pfs_udf_ops_umount,
        pfs_udf_ops_is_mounted,
        pfs_udf_ops_can_write,
    },
    {
        (int)sizeof(pfs_udf_file_meta_t),
        pfs_udf_ops_file_open,
        pfs_udf_ops_file_close,
        pfs_udf_ops_file_read,
        pfs_udf_ops_file_write,
        pfs_udf_ops_file_truncate,
        pfs_udf_ops_file_fdatasync,
        pfs_udf_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_udf_dir_meta_t),
        pfs_udf_ops_dir_open,
        pfs_udf_ops_dir_close,
        pfs_udf_ops_dir_readdir,
    },
    {
        pfs_udf_ops_access,
        pfs_udf_ops_mkdir,
        pfs_udf_ops_rmdir,
        pfs_udf_ops_rename,
        pfs_udf_ops_unlink,
        pfs_udf_ops_check_path,
    },
    {
        pfs_udf_ops_probe,
        pfs_udf_ops_mkfs,
        pfs_udf_ops_fsync,
        NULL,
    },
};

// end
