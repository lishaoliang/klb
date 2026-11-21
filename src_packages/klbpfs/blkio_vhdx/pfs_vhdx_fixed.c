// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX Fixed: 读/写 (块已预分配)
#include "blkio_vhdx/pfs_vhdx_internal.h"

int pfs_vhdx_fixed_pread(pfs_vhdx_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    return pfs_vhdx_payload_pread(p_priv, buf, size, offset);
}

int pfs_vhdx_fixed_pwrite(pfs_vhdx_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    return pfs_vhdx_payload_pwrite(p_priv, buf, size, offset);
}

// end
