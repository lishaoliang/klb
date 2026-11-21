// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs 文件读 (FLAT_PLAIN/INLINE; 写路径 EPERM)
#include "erofs/pfs_erofs_internal.h"

#include <string.h>
#include <stdbool.h>

int pfs_erofs_file_open(pfs_erofs_sbi_t *p_sbi, const char *path, int mode, pfs_erofs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_erofs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & (PFS_O_RDWR | PFS_O_CREAT)))
    {
        return PFS_EPERM;
    }

    memset(p_fm, 0, sizeof(*p_fm));
    uint64_t nid;
    bool is_dir;
    int rc = pfs_erofs_lookup_path(p_sbi, path, &nid, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EINVAL;
    }
    rc = pfs_erofs_read_inode(p_sbi, nid, &p_fm->vi);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (PFS_EROFS_S_IFREG != (p_fm->vi.mode & PFS_EROFS_S_IFMT))
    {
        return PFS_EINVAL;
    }
    return PFS_OK;
}

void pfs_erofs_file_close(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_erofs_file_read(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    return pfs_erofs_map_read(p_sbi, &p_fm->vi, pos, buf, size, p_got);
}

int pfs_erofs_file_write(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
{
    (void)p_sbi;
    (void)p_fm;
    (void)pos;
    (void)buf;
    (void)size;
    (void)p_put;
    return PFS_EPERM;
}

int pfs_erofs_file_truncate(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_erofs_file_sync(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm)
{
    (void)p_sbi;
    (void)p_fm;
    return PFS_OK;
}

int64_t pfs_erofs_file_get_size(pfs_erofs_sbi_t *p_sbi, const pfs_erofs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->vi.i_size;
}

// end
