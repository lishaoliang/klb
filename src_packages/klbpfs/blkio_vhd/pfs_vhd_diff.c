// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD Differencing 打桩: 父子链读 / 写时分配
#include "blkio_vhd/pfs_vhd_internal.h"

#include <stddef.h>

int pfs_vhd_diff_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    // 打桩: 尚未解析 parent locator / 沿链读
    return PFS_BLKIO_RET_STATE;
}

int pfs_vhd_diff_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    return PFS_BLKIO_RET_STATE;
}

// end
