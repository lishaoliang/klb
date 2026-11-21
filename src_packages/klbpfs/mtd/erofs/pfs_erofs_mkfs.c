// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs mkfs (magic@1024 + 空 FLAT DIR 根) / mkvol (空树; 非空暂 EOPNOTSUPP)
#include "erofs/pfs_erofs_internal.h"
#include "base/pfsmtd_dirwalk.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

static int pfs_erofs_mkvol_count(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    int *p_n = (int *)ctx;
    (void)vfs_rel;
    (void)host_path;
    (void)is_dir;
    (*p_n)++;
    return PFS_OK;
}

static void pfs_erofs_put_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
}

static void pfs_erofs_put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static void pfs_erofs_put_le64(uint8_t *p, uint64_t v)
{
    pfs_erofs_put_le32(p, (uint32_t)v);
    pfs_erofs_put_le32(p + 4, (uint32_t)(v >> 32));
}

int pfs_erofs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    const int blkszbits = 12;
    const int blksz = 1 << blkszbits;

    (void)vol_bytes;
    if (NULL == p_blkio || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    // super@1024; meta block 1 起; root nid=1 -> iloc = blksz + 32
    int64_t need = (int64_t)blksz * 2;
    uint8_t *buf = (uint8_t *)pfsmtd_mallocz((size_t)need);

    if (NULL == buf)
    {
        return PFS_ENOMEM;
    }

    uint8_t *sb = buf + PFS_EROFS_SUPER_OFFSET;
    pfs_erofs_put_le32(sb + 0, PFS_EROFS_MAGIC);
    sb[12] = (uint8_t)blkszbits; // blkszbits
    pfs_erofs_put_le16(sb + 14, 1); // rootnid_2b
    pfs_erofs_put_le64(sb + 16, 1); // inos
    pfs_erofs_put_le32(sb + 36, 2); // blocks_lo
    pfs_erofs_put_le32(sb + 40, 1); // meta_blkaddr
    // volume_name / uuid 已 0

    uint8_t *ino = buf + blksz + 32; // nid 1
    uint16_t fmt = (uint16_t)(PFS_EROFS_INODE_FLAT_PLAIN << PFS_EROFS_I_DATALAYOUT_BIT);
    pfs_erofs_put_le16(ino + 0, fmt);
    pfs_erofs_put_le16(ino + 4, (uint16_t)(PFS_EROFS_S_IFDIR | 0755));
    pfs_erofs_put_le16(ino + 6, 1); // nlink
    pfs_erofs_put_le32(ino + 8, 0); // i_size
    pfs_erofs_put_le32(ino + 16, 0); // i_u startblk
    pfs_erofs_put_le32(ino + 20, 1); // i_ino

    int n = pfs_blkio_pwrite(p_blkio, buf, (int)need, part_offset);
    pfsmtd_free(buf);
    if (n != (int)need)
    {
        if (n < 0)
        {
            return pfs_erofs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    n = pfs_blkio_fsync(p_blkio);
    if (n < 0)
    {
        return pfs_erofs_map_blkio_rc(n);
    }
    return PFS_OK;
}

int pfs_erofs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)volume_id;
    if (NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }

    int count = 0;
    int rc = pfsmtd_dirwalk(src_dir, pfs_erofs_mkvol_count, &count);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (count > 0)
    {
        return PFS_EOPNOTSUPP;
    }
    return pfs_erofs_mkfs(p_blkio, part_offset, vol_bytes);
}

// end
