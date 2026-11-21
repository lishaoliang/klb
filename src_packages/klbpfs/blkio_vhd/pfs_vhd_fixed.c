// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD Fixed: 逻辑盘偏移线性映射到宿主文件 (数据在文件头, footer 在尾)
#include "blkio_vhd/pfs_vhd_internal.h"
#include "base/pfs_blkio_raw.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

static bool pfs_vhd_fixed_range_ok(const pfs_vhd_priv_t *p_priv, int64_t offset, int size)
{
    if (offset < 0 || size < 0 || size > PFS_BLKIO_IO_SIZE_MAX)
    {
        return false;
    }
    if (0 == size)
    {
        return true;
    }

    uint64_t off_u = (uint64_t)offset;
    if (off_u >= p_priv->current_size)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->current_size)
    {
        return false;
    }
    return true;
}

int pfs_vhd_fixed_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhd_fixed_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    // Fixed: 逻辑 offset == 宿主文件 offset
    return pfs_blkio_raw_pread(&p_priv->host, buf, size, offset);
}

int pfs_vhd_fixed_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhd_fixed_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    return pfs_blkio_raw_pwrite(&p_priv->host, buf, size, offset);
}

// end
