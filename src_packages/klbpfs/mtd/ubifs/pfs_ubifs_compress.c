// Doc Encode : UTF-8 BOM, Unix(LF)
// ubifs 解压 (写子集仅 NONE)
#include "ubifs/pfs_ubifs_internal.h"

#include <string.h>

int pfs_ubifs_decompress(int compr_type, const void *in, int in_len, void *out, int *p_out_len)
{
    if (NULL == in || NULL == out || NULL == p_out_len || in_len < 0)
    {
        return PFS_EINVAL;
    }
    if (PFS_UBIFS_COMPR_NONE != compr_type)
    {
        return PFS_EOPNOTSUPP;
    }
    memcpy(out, in, (size_t)in_len);
    *p_out_len = in_len;
    return PFS_OK;
}

// end
