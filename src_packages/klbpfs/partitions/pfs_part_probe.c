// Doc Encode : UTF-8 BOM, Unix(LF)
// 分区探测门面; 序对齐 linux-7.1 block/partitions/core.c (efi 先于 msdos)
// 对外返回 pfs_ret_e (PFS_*); 内部探测仍用 PFS_PART_RET_* (数值相同)
#include "pfs.h"
#include "partitions/pfs_part_internal.h"

#include <string.h>

int pfs_part_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out)
{
    if (NULL == p_blkio || NULL == p_out)
    {
        return PFS_EINVAL;
    }
    if (!pfs_blkio_is_open(p_blkio))
    {
        return PFS_EINVAL;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_NONE;

    int rc = pfs_part_gpt_probe(p_blkio, p_out);
    if (PFS_PART_RET_OK == rc)
    {
        return PFS_OK;
    }
    if (PFS_PART_RET_EOPNOTSUPP != rc && PFS_PART_RET_ENOEXEC != rc)
    {
        return rc;
    }

    rc = pfs_part_mbr_probe(p_blkio, p_out);
    if (PFS_PART_RET_OK == rc)
    {
        return PFS_OK;
    }
    if (PFS_PART_RET_EOPNOTSUPP != rc && PFS_PART_RET_ENOEXEC != rc)
    {
        return rc;
    }

    memset(p_out, 0, sizeof(*p_out));
    p_out->scheme = PFS_PART_SCHEME_NONE;
    return PFS_OK;
}

// end
