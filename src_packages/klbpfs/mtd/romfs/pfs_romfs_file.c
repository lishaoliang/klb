// Doc Encode : UTF-8 BOM, Unix(LF)
// romfs 文件读 (无压缩; REG/SYM; 写 EPERM)
#include "romfs/pfs_romfs_internal.h"

#include <string.h>
#include <stdbool.h>

int pfs_romfs_file_open(pfs_romfs_sbi_t *p_sbi, const char *path, int mode, pfs_romfs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & (PFS_O_RDWR | PFS_O_CREAT)))
    {
        return PFS_EPERM;
    }

    memset(p_fm, 0, sizeof(*p_fm));
    pfs_romfs_fh_t fh;
    bool is_dir;
    int rc = pfs_romfs_lookup_path(p_sbi, path, &fh, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EINVAL;
    }
    if (PFS_ROMFH_REG != fh.type && PFS_ROMFH_SYM != fh.type)
    {
        return PFS_EINVAL;
    }

    p_fm->fh = fh;
    return PFS_OK;
}

void pfs_romfs_file_close(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_romfs_file_read(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || 0 > size || 0 > pos)
    {
        return PFS_EINVAL;
    }
    if (!pfs_romfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 == size || (int64_t)p_fm->fh.size <= pos)
    {
        return PFS_OK;
    }

    int64_t remain = (int64_t)p_fm->fh.size - pos;
    if ((int64_t)size < remain)
    {
        remain = (int64_t)size;
    }
    int want = (int)remain;
    if (0 >= want)
    {
        return PFS_OK;
    }

    int n = pfs_blkio_pread(p_sbi->p_blkio, buf, want,
            p_sbi->part_offset + (int64_t)p_fm->fh.data_off + pos);
    if (0 > n)
    {
        return pfs_romfs_map_blkio_rc(n);
    }
    else if (n != want)
    {
        return PFS_EIO;
    }
    *p_got = n;
    return PFS_OK;
}

int pfs_romfs_file_write(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
{
    (void)p_sbi;
    (void)p_fm;
    (void)pos;
    (void)buf;
    (void)size;
    (void)p_put;
    return PFS_EPERM;
}

int pfs_romfs_file_truncate(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_romfs_file_sync(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    (void)p_fm;
    return PFS_OK;
}

int64_t pfs_romfs_file_get_size(pfs_romfs_sbi_t *p_sbi, const pfs_romfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->fh.size;
}

// end
