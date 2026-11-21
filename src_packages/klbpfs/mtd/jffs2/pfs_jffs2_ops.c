// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 填 pfs_fs_ops_t (读 + 写子集)
#include "jffs2/pfs_jffs2_internal.h"
#include "pfsmtd.h"
#include "base/pfsmtd_path_check.h"

////////////////////////////////////////////////////////////
// super

static int pfs_jffs2_ops_init(void *p_sbi)
{
    return pfs_jffs2_init((pfs_jffs2_sbi_t *)p_sbi);
}

static void pfs_jffs2_ops_deinit(void *p_sbi)
{
    pfs_jffs2_deinit((pfs_jffs2_sbi_t *)p_sbi);
}

static int pfs_jffs2_ops_probe(pfs_blkio_t *p_blkio, int64_t part_offset)
{
    return pfs_jffs2_probe(p_blkio, part_offset);
}

static int pfs_jffs2_ops_mount(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode)
{
    return pfs_jffs2_mount((pfs_jffs2_sbi_t *)p_sbi, p_blkio, part_offset, mode);
}

static int pfs_jffs2_ops_remount(void *p_sbi, int mode)
{
    return pfs_jffs2_remount((pfs_jffs2_sbi_t *)p_sbi, mode);
}

static int pfs_jffs2_ops_umount(void *p_sbi)
{
    return pfs_jffs2_umount((pfs_jffs2_sbi_t *)p_sbi);
}

static int pfs_jffs2_ops_is_mounted(const void *p_sbi)
{
    return pfs_jffs2_sbi_is_mounted((const pfs_jffs2_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_jffs2_ops_can_write(const void *p_sbi)
{
    return pfs_jffs2_sbi_can_write((const pfs_jffs2_sbi_t *)p_sbi) ? 1 : 0;
}

static int pfs_jffs2_ops_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    return pfs_jffs2_mkfs(p_blkio, part_offset, vol_bytes);
}

////////////////////////////////////////////////////////////
// file / dir / path

static int pfs_jffs2_ops_file_open(void *p_sbi, const char *path, int mode, void *p_fpriv)
{
    return pfs_jffs2_file_open((pfs_jffs2_sbi_t *)p_sbi, path, mode, (pfs_jffs2_file_meta_t *)p_fpriv);
}

static void pfs_jffs2_ops_file_close(void *p_sbi, void *p_fpriv)
{
    pfs_jffs2_file_close((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_file_meta_t *)p_fpriv);
}

static int pfs_jffs2_ops_file_read(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got)
{
    return pfs_jffs2_file_read((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_file_meta_t *)p_fpriv, pos, buf, size, p_got);
}

static int pfs_jffs2_ops_file_write(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put)
{
    return pfs_jffs2_file_write((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_file_meta_t *)p_fpriv, pos, buf, size, p_put);
}

static int pfs_jffs2_ops_file_truncate(void *p_sbi, void *p_fpriv, int64_t size)
{
    return pfs_jffs2_file_truncate((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_file_meta_t *)p_fpriv, size);
}

static int pfs_jffs2_ops_file_fdatasync(void *p_sbi, void *p_fpriv)
{
    return pfs_jffs2_file_sync((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_file_meta_t *)p_fpriv);
}

static int64_t pfs_jffs2_ops_file_get_size(void *p_sbi, const void *p_fpriv)
{
    return pfs_jffs2_file_get_size((pfs_jffs2_sbi_t *)p_sbi, (const pfs_jffs2_file_meta_t *)p_fpriv);
}

static int pfs_jffs2_ops_dir_open(void *p_sbi, const char *path, void *p_dpriv)
{
    return pfs_jffs2_dir_open((pfs_jffs2_sbi_t *)p_sbi, path, (pfs_jffs2_dir_meta_t *)p_dpriv);
}

static void pfs_jffs2_ops_dir_close(void *p_sbi, void *p_dpriv)
{
    pfs_jffs2_dir_close((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_dir_meta_t *)p_dpriv);
}

static int pfs_jffs2_ops_dir_readdir(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent)
{
    return pfs_jffs2_dir_readdir((pfs_jffs2_sbi_t *)p_sbi, (pfs_jffs2_dir_meta_t *)p_dpriv, p_ent);
}

static int pfs_jffs2_ops_access(void *p_sbi, const char *path, int mode)
{
    return pfs_jffs2_access((pfs_jffs2_sbi_t *)p_sbi, path, mode);
}

static int pfs_jffs2_ops_mkdir(void *p_sbi, const char *path, int mode)
{
    return pfs_jffs2_mkdir((pfs_jffs2_sbi_t *)p_sbi, path, mode);
}

static int pfs_jffs2_ops_rmdir(void *p_sbi, const char *path)
{
    return pfs_jffs2_rmdir((pfs_jffs2_sbi_t *)p_sbi, path);
}

static int pfs_jffs2_ops_rename(void *p_sbi, const char *oldpath, const char *newpath)
{
    return pfs_jffs2_rename((pfs_jffs2_sbi_t *)p_sbi, oldpath, newpath);
}

static int pfs_jffs2_ops_unlink(void *p_sbi, const char *path)
{
    return pfs_jffs2_unlink((pfs_jffs2_sbi_t *)p_sbi, path);
}

static int pfs_jffs2_ops_check_path(void *p_sbi, const char *path, uint32_t flags)
{
    (void)p_sbi;
    (void)flags;
    return pfsmtd_path_check_win_fat(path);
}

////////////////////////////////////////////////////////////

const pfs_fs_ops_t g_pfs_jffs2_fs_ops = {
    "jffs2",
    (pfs_fs_type_e)PFSMTD_JFFS2,
    {
        (int)sizeof(pfs_jffs2_sbi_t),
        pfs_jffs2_ops_init,
        pfs_jffs2_ops_deinit,
        pfs_jffs2_ops_mount,
        pfs_jffs2_ops_remount,
        pfs_jffs2_ops_umount,
        pfs_jffs2_ops_is_mounted,
        pfs_jffs2_ops_can_write,
    },
    {
        (int)sizeof(pfs_jffs2_file_meta_t),
        pfs_jffs2_ops_file_open,
        pfs_jffs2_ops_file_close,
        pfs_jffs2_ops_file_read,
        pfs_jffs2_ops_file_write,
        pfs_jffs2_ops_file_truncate,
        pfs_jffs2_ops_file_fdatasync,
        pfs_jffs2_ops_file_get_size,
    },
    {
        (int)sizeof(pfs_jffs2_dir_meta_t),
        pfs_jffs2_ops_dir_open,
        pfs_jffs2_ops_dir_close,
        pfs_jffs2_ops_dir_readdir,
    },
    {
        pfs_jffs2_ops_access,
        pfs_jffs2_ops_mkdir,
        pfs_jffs2_ops_rmdir,
        pfs_jffs2_ops_rename,
        pfs_jffs2_ops_unlink,
        pfs_jffs2_ops_check_path,
    },
    {
        pfs_jffs2_ops_probe,
        pfs_jffs2_ops_mkfs,
        NULL,
        NULL,
    },
};

const pfs_fs_ops_t *pfsmtd_get_jffs2(void)
{
    return &g_pfs_jffs2_fs_ops;
}

// end
