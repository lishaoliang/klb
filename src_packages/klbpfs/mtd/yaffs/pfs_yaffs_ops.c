// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs 填 pfs_fs_ops_t (读 + 写子集)
#include "yaffs/pfs_yaffs_internal.h"
#include "pfsmtd.h"
#include "base/pfsmtd_path_check.h"

////////////////////////////////////////////////////////////
// super

static int pfs_yaffs_ops_init(void *p_sbi)
{
    return pfs_yaffs_init((pfs_yaffs_sbi_t *)p_sbi);
}

static void pfs_yaffs_ops_deinit(void *p_sbi)
{
    pfs_yaffs_deinit((pfs_yaffs_sbi_t *)p_sbi);
}

static int pfs_yaffs_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_yaffs_probe(p_blkio, part_offset);
}

static int pfs_yaffs_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_yaffs_mount((pfs_yaffs_sbi_t *)p_sbi, p_blkio, part_offset, mode);
}

static int pfs_yaffs_ops_remount(void *p_sbi, int mode)
{
    return pfs_yaffs_remount((pfs_yaffs_sbi_t *)p_sbi, mode);
}

static int pfs_yaffs_ops_umount(void *p_sbi)
{
    return pfs_yaffs_umount((pfs_yaffs_sbi_t *)p_sbi);
}

static int pfs_yaffs_ops_is_mounted(const void *p_sbi)
{
    return pfs_yaffs_sbi_is_mounted((const pfs_yaffs_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_yaffs_ops_can_write(const void *p_sbi)
{
    return pfs_yaffs_sbi_can_write((const pfs_yaffs_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_yaffs_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_yaffs_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file / dir / path

static int pfs_yaffs_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    return pfs_yaffs_file_open((pfs_yaffs_sbi_t *)p_sbi, path, mode, (pfs_yaffs_file_meta_t *)p_fpriv);
}

static void pfs_yaffs_ops_file_close(void *p_sbi, void *p_fpriv)
{
    pfs_yaffs_file_close((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_file_meta_t *)p_fpriv);
}

static int pfs_yaffs_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    return pfs_yaffs_file_read((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_file_meta_t *)p_fpriv, pos, buf, size, p_got);
}

static int pfs_yaffs_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    return pfs_yaffs_file_write((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_file_meta_t *)p_fpriv, pos, buf, size, p_put);
}

static int pfs_yaffs_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    return pfs_yaffs_file_truncate((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_file_meta_t *)p_fpriv, size);
}

static int pfs_yaffs_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    return pfs_yaffs_file_sync((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_file_meta_t *)p_fpriv);
}

static int64_t pfs_yaffs_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    return pfs_yaffs_file_get_size((pfs_yaffs_sbi_t *)p_sbi, (const pfs_yaffs_file_meta_t *)p_fpriv);
}

static int pfs_yaffs_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    return pfs_yaffs_dir_open((pfs_yaffs_sbi_t *)p_sbi, path, (pfs_yaffs_dir_meta_t *)p_dpriv);
}

static void pfs_yaffs_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    pfs_yaffs_dir_close((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_dir_meta_t *)p_dpriv);
}

static int pfs_yaffs_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    return pfs_yaffs_dir_readdir((pfs_yaffs_sbi_t *)p_sbi, (pfs_yaffs_dir_meta_t *)p_dpriv, p_ent);
}

static int pfs_yaffs_ops_access(void *p_sbi, const char *path, int mode)
{
    return pfs_yaffs_access((pfs_yaffs_sbi_t *)p_sbi, path, mode);
}

static int pfs_yaffs_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    return pfs_yaffs_mkdir((pfs_yaffs_sbi_t *)p_sbi, path, mode);
}

static int pfs_yaffs_ops_rmdir(void *p_sbi, const char *path)
{
    return pfs_yaffs_rmdir((pfs_yaffs_sbi_t *)p_sbi, path);
}

static int pfs_yaffs_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    return pfs_yaffs_rename((pfs_yaffs_sbi_t *)p_sbi, oldpath, newpath);
}

static int pfs_yaffs_ops_unlink(void *p_sbi, const char *path)
{
    return pfs_yaffs_unlink((pfs_yaffs_sbi_t *)p_sbi, path);
}

static int pfs_yaffs_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfsmtd_path_check_win_fat(path);
}

////////////////////////////////////////////////////////////

const pfs_fs_ops_t g_pfs_yaffs_fs_ops = {
    "yaffs",
    (pfs_fs_type_e)PFSMTD_YAFFS,
    {
        (int)sizeof(pfs_yaffs_sbi_t),
        pfs_yaffs_ops_init,
        pfs_yaffs_ops_deinit,
        pfs_yaffs_ops_mount,
        pfs_yaffs_ops_remount,
        pfs_yaffs_ops_umount,
        pfs_yaffs_ops_is_mounted,
        pfs_yaffs_ops_can_write,
    },
    {
        (int)sizeof(pfs_yaffs_file_meta_t),
        pfs_yaffs_ops_file_open,
        pfs_yaffs_ops_file_close,
        pfs_yaffs_ops_file_read,
        pfs_yaffs_ops_file_write,
        pfs_yaffs_ops_file_truncate,
        pfs_yaffs_ops_file_fdatasync,
        pfs_yaffs_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_yaffs_dir_meta_t),
        pfs_yaffs_ops_dir_open,
        pfs_yaffs_ops_dir_close,
        pfs_yaffs_ops_dir_readdir,
    },
    {
        pfs_yaffs_ops_access,
        pfs_yaffs_ops_mkdir,
        pfs_yaffs_ops_rmdir,
        pfs_yaffs_ops_rename,
        pfs_yaffs_ops_unlink,
        pfs_yaffs_ops_check_path,
    },
    {
        pfs_yaffs_ops_probe,
        pfs_yaffs_ops_mkfs,
        NULL,
        NULL,
    },
};

const pfs_fs_ops_t *pfsmtd_get_yaffs(void)
{
    return &g_pfs_yaffs_fs_ops;
}

// end
