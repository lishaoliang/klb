// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs 文件读 (zlib/未压缩/空洞; 写 EPERM)
#include "cramfs/pfs_cramfs_internal.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

int pfs_cramfs_file_open(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode, pfs_cramfs_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_cramfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 != (mode & (PFS_O_RDWR | PFS_O_CREAT)))
    {
        return PFS_EPERM;
    }

    memset(p_fm, 0, sizeof(*p_fm));
    pfs_cramfs_inode_t ino = {0};
    bool is_dir = false;
    int rc = pfs_cramfs_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EINVAL;
    }
    if (PFS_CRAMFS_S_IFREG != (ino.mode & PFS_CRAMFS_S_IFMT))
    {
        return PFS_EINVAL;
    }

    p_fm->ino = ino;
    p_fm->data_off = pfs_cramfs_inode_byte_off(&ino);
    return PFS_OK;
}

void pfs_cramfs_file_close(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_cramfs_file_read(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || size < 0 || pos < 0)
    {
        return PFS_EINVAL;
    }
    if (!pfs_cramfs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 == size || pos >= (int64_t)p_fm->ino.size)
    {
        return PFS_OK;
    }
    if (0 == p_fm->ino.size || 0 == p_fm->data_off)
    {
        return PFS_OK;
    }

    int64_t remain = (int64_t)p_fm->ino.size - pos;
    if (remain > (int64_t)size)
    {
        remain = (int64_t)size;
    }

    uint8_t *page = (uint8_t *)pfsmtd_malloc(PFS_CRAMFS_PAGE_SIZE);
    if (NULL == page)
    {
        return PFS_ENOMEM;
    }

    uint8_t *out = (uint8_t *)buf;
    int total = 0;
    while (remain > 0)
    {
        uint32_t page_idx = (uint32_t)(pos / PFS_CRAMFS_PAGE_SIZE);
        uint32_t off_in_page = (uint32_t)(pos % PFS_CRAMFS_PAGE_SIZE);
        int out_len;
        int rc = pfs_cramfs_read_page(p_sbi, &p_fm->ino, p_fm->data_off, page_idx, page, &out_len);
        if (PFS_OK != rc)
        {
            pfsmtd_free(page);
            return rc;
        }
        if (out_len <= (int)off_in_page)
        {
            break;
        }
        {
            uint32_t take = (uint32_t)out_len - off_in_page;

            if ((int64_t)take > remain)
            {
                take = (uint32_t)remain;
            }
            memcpy(out, page + off_in_page, (size_t)take);
            out += take;
            pos += (int64_t)take;
            remain -= (int64_t)take;
            total += (int)take;
        }
    }

    pfsmtd_free(page);
    *p_got = total;
    return PFS_OK;
}

int pfs_cramfs_file_write(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
{
    (void)p_sbi;
    (void)p_fm;
    (void)pos;
    (void)buf;
    (void)size;
    (void)p_put;
    return PFS_EPERM;
}

int pfs_cramfs_file_truncate(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_cramfs_file_sync(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    (void)p_fm;
    return PFS_OK;
}

int64_t pfs_cramfs_file_get_size(pfs_cramfs_sbi_t *p_sbi, const pfs_cramfs_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->ino.size;
}

// end
