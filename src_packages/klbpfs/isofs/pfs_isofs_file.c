// Doc Encode : UTF-8 BOM, Unix(LF)
// ISO 9660 文件只读 (多 extent + ext_attr 已含在 section.extent)
#include "isofs/pfs_isofs_internal.h"

#include <string.h>

int pfs_isofs_file_read(pfs_isofs_sbi_t *p_sbi, const pfs_isofs_file_meta_t *p_fm,
        int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || NULL == p_got || 0 > size)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (!pfs_isofs_sbi_is_mounted(p_sbi))
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (PFS_ISOFS_TYPE_FILE != p_fm->inode.type)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (0 > pos)
    {
        return PFS_ISOFS_RET_EINVAL;
    }
    if (0 == p_fm->inode.n_sections)
    {
        return PFS_ISOFS_RET_EBADMSG;
    }

    *p_got = 0;
    if (0 == size || (uint64_t)pos >= (uint64_t)p_fm->inode.size)
    {
        return PFS_ISOFS_RET_OK;
    }

    int left = size;
    if ((uint64_t)left > (uint64_t)p_fm->inode.size - (uint64_t)pos)
    {
        left = (int)((uint64_t)p_fm->inode.size - (uint64_t)pos);
    }
    uint8_t *out = (uint8_t *)buf;
    int64_t file_off = pos;

    for (int si = 0; si < (int)p_fm->inode.n_sections && left > 0; si++)
    {
        uint32_t sec_sz = p_fm->inode.sec[si].size;
        if (file_off >= (int64_t)sec_sz)
        {
            file_off -= (int64_t)sec_sz;
            continue;
        }
        int64_t off_in_sec = file_off;
        int chunk = (int)((int64_t)sec_sz - off_in_sec);
        if (chunk > left)
        {
            chunk = left;
        }
        int64_t disk_off = pfs_isofs_lba_to_off(p_sbi, p_fm->inode.sec[si].extent) + off_in_sec;
        int rc = pfs_blkio_pread(p_sbi->p_blkio, out, chunk, disk_off);
        if (0 > rc)
        {
            return pfs_isofs_map_blkio_rc(rc);
        }
        else if (rc != chunk)
        {
            *p_got += rc;
            return PFS_ISOFS_RET_EIO;
        }
        out += chunk;
        left -= chunk;
        *p_got += chunk;
        file_off = 0;
    }
    return PFS_ISOFS_RET_OK;
}

// end
