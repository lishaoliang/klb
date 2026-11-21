// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs mkfs (空根 DIR) / mkvol (空树; 非空暂 EOPNOTSUPP, 用 squashfs/romfs 打包)
#include "cramfs/pfs_cramfs_internal.h"
#include "base/pfsmtd_dirwalk.h"

#include <string.h>
#include <stdbool.h>

static int pfs_cramfs_mkvol_count(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    int *p_n = (int *)ctx;
    (*p_n)++;
    return PFS_OK;
}

static void pfs_cramfs_put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

int pfs_cramfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    (void)vol_bytes;
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    uint8_t sb[76];
    memset(sb, 0, sizeof(sb));
    pfs_cramfs_put_le32(sb + 0, PFS_CRAMFS_MAGIC);
    pfs_cramfs_put_le32(sb + 4, (uint32_t)sizeof(sb));
    pfs_cramfs_put_le32(sb + 8, PFS_CRAMFS_FLAG_FSID_VERSION_2);
    memcpy(sb + 16, PFS_CRAMFS_SIGNATURE, 16);
    // root: mode=DIR|0755, size=0, offset=0
    uint32_t w0 = (uint32_t)(PFS_CRAMFS_S_IFDIR | 0755);
    pfs_cramfs_put_le32(sb + 64, w0);
    pfs_cramfs_put_le32(sb + 68, 0);
    pfs_cramfs_put_le32(sb + 72, 0);

    int n = pfs_blkio_pwrite(p_blkio, sb, (int)sizeof(sb), part_offset);
    if (n != (int)sizeof(sb))
    {
        if (0 > n)
        {
            return pfs_cramfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    n = pfs_blkio_fsync(p_blkio);
    if (0 > n)
    {
        return pfs_cramfs_map_blkio_rc(n);
    }
    return PFS_OK;
}

int pfs_cramfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)volume_id;
    if (NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }

    int count = 0;
    int rc = pfsmtd_dirwalk(src_dir, pfs_cramfs_mkvol_count, &count);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (0 < count)
    {
        // 非空树灌装未做; 空盘 mkfs 可用; 内容打包用 squashfs/romfs
        return PFS_EOPNOTSUPP;
    }
    return pfs_cramfs_mkfs(p_blkio, part_offset, vol_bytes);
}

// end
