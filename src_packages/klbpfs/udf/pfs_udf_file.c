// Doc Encode : UTF-8 BOM, Unix(LF)
// UDF file read (multi-extent + IN_ICB)
#include "udf/pfs_udf_internal.h"

#include <string.h>

int pfs_udf_file_read(pfs_udf_sbi_t *p_sbi, const pfs_udf_file_meta_t *p_fm,
        int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || NULL == p_got || size < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (!pfs_udf_sbi_is_mounted(p_sbi))
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (PFS_UDF_TYPE_FILE != p_fm->inode.type && PFS_UDF_TYPE_DIR != p_fm->inode.type)
    {
        return PFS_UDF_RET_EINVAL;
    }
    if (pos < 0)
    {
        return PFS_UDF_RET_EINVAL;
    }
    *p_got = 0;
    if (0 == size || (uint64_t)pos >= p_fm->inode.size)
    {
        return PFS_UDF_RET_OK;
    }
    if (0 == p_fm->inode.n_extents)
    {
        return PFS_UDF_RET_EBADMSG;
    }

    int left = size;
    if ((uint64_t)left > p_fm->inode.size - (uint64_t)pos)
    {
        left = (int)(p_fm->inode.size - (uint64_t)pos);
    }
    uint8_t *out = (uint8_t *)buf;
    int64_t file_off = pos;

    for (int si = 0; si < p_fm->inode.n_extents && left > 0; si++)
    {
        uint32_t sec_sz;
        int64_t off_in_sec;
        int chunk;
        int64_t disk_off;

        sec_sz = p_fm->inode.sec[si].len;
        if (file_off >= (int64_t)sec_sz)
        {
            file_off -= (int64_t)sec_sz;
            continue;
        }
        off_in_sec = file_off;
        chunk = (int)((int64_t)sec_sz - off_in_sec);
        if (chunk > left)
        {
            chunk = left;
        }
        disk_off = pfs_udf_map_to_off(p_sbi, p_fm->inode.part_ref, p_fm->inode.sec[si].block);
        if (disk_off < 0)
        {
            return (int)disk_off;
        }
        disk_off += (int64_t)p_fm->inode.sec[si].skip + off_in_sec;
        int rc = pfs_blkio_pread(p_sbi->p_blkio, out, chunk, disk_off);
        if (rc < 0)
        {
            return pfs_udf_map_blkio_rc(rc);
        }
        if (rc != chunk)
        {
            *p_got += rc;
            return PFS_UDF_RET_EIO;
        }
        out += chunk;
        left -= chunk;
        *p_got += chunk;
        file_off = 0;
    }
    return PFS_UDF_RET_OK;
}

// end
