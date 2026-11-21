// Doc Encode : UTF-8 BOM, Unix(LF)
// erofs inode 解析与未压缩 map 读 (对照 inode.c / data.c; 仅 FLAT_PLAIN/INLINE)
#include "erofs/pfs_erofs_internal.h"

#include <string.h>

static uint64_t pfs_erofs_iloc(const pfs_erofs_sbi_t *p_sbi, uint64_t nid)
{
    return ((uint64_t)p_sbi->meta_blkaddr << p_sbi->blkszbits) + (nid << PFS_EROFS_ISLOTBITS);
}

static uint64_t pfs_erofs_iblks(const pfs_erofs_inode_t *p_vi, uint32_t blksz)
{
    if (0 == blksz)
    {
        return 0;
    }
    return (p_vi->i_size + (uint64_t)blksz - 1u) / (uint64_t)blksz;
}

int pfs_erofs_read_sb(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_erofs_super_block_t *p_sb)
{
    if (NULL == p_blkio || NULL == p_sb || 0 > part_offset)
    {
        return PFS_EINVAL;
    }

    memset(p_sb, 0, sizeof(*p_sb));
    int n = pfs_blkio_pread(p_blkio, p_sb, (int)sizeof(*p_sb), part_offset + (int64_t)PFS_EROFS_SUPER_OFFSET);
    if (n < 128)
    {
        if (n < 0)
        {
            return pfs_erofs_map_blkio_rc(n);
        }
        return PFS_ENOEXEC;
    }
    if (PFS_EROFS_MAGIC != p_sb->magic)
    {
        return PFS_ENOEXEC;
    }
    return PFS_OK;
}

int pfs_erofs_read_inode(pfs_erofs_sbi_t *p_sbi, uint64_t nid, pfs_erofs_inode_t *p_vi)
{
    if (NULL == p_sbi || NULL == p_vi || !pfs_erofs_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_vi, 0, sizeof(*p_vi));

    uint8_t buf[128];
    uint64_t iloc = pfs_erofs_iloc(p_sbi, nid);
    int64_t abs_off = p_sbi->part_offset + (int64_t)iloc;
    int n = pfs_blkio_pread(p_sbi->p_blkio, buf, (int)sizeof(buf), abs_off);
    if (n < (int)sizeof(pfs_erofs_inode_compact_t))
    {
        if (n < 0)
        {
            return pfs_erofs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }

    uint16_t ifmt = pfs_erofs_le16(buf + 0);
    uint16_t version = (uint16_t)((ifmt >> PFS_EROFS_I_VERSION_BIT) & 1u);
    uint16_t datalayout = (uint16_t)((ifmt >> PFS_EROFS_I_DATALAYOUT_BIT) & 7u);
    if (datalayout >= PFS_EROFS_INODE_CHUNK_BASED + 1)
    {
        return PFS_EOPNOTSUPP;
    }
    if (PFS_EROFS_INODE_COMPRESSED_FULL == datalayout
            || PFS_EROFS_INODE_COMPRESSED_COMPACT == datalayout
            || PFS_EROFS_INODE_CHUNK_BASED == datalayout)
    {
        return PFS_EOPNOTSUPP;
    }

    uint16_t xattr_icount = pfs_erofs_le16(buf + 2);
    uint16_t mode = pfs_erofs_le16(buf + 4);
    uint16_t i_nb = pfs_erofs_le16(buf + 6);
    uint64_t i_size = 0;
    uint64_t addrmask = 0;
    uint64_t startblk = 0;

    if (PFS_EROFS_INODE_LAYOUT_EXTENDED == version)
    {
        if (n < (int)sizeof(pfs_erofs_inode_extended_t))
        {
            return PFS_EIO;
        }
        i_size = pfs_erofs_le64(buf + 8);
        uint32_t startblk_lo = pfs_erofs_le32(buf + 16);

        addrmask = ((uint64_t)1 << 48) - 1u;
        startblk = (uint64_t)startblk_lo | ((uint64_t)i_nb << 32);
        p_vi->inode_isize = (uint32_t)sizeof(pfs_erofs_inode_extended_t);
    }
    else if (PFS_EROFS_INODE_LAYOUT_COMPACT == version)
    {
        i_size = (uint64_t)pfs_erofs_le32(buf + 8);
        uint32_t startblk_lo = pfs_erofs_le32(buf + 16);

        if (PFS_EROFS_S_IFDIR != (mode & PFS_EROFS_S_IFMT)
                && 0 != ((ifmt >> PFS_EROFS_I_NLINK_1_BIT) & 1u))
        {
            addrmask = ((uint64_t)1 << 48) - 1u;
            startblk = (uint64_t)startblk_lo | ((uint64_t)i_nb << 32);
        }
        else
        {
            addrmask = 0xffffffffu;
            startblk = (uint64_t)startblk_lo;
        }
        p_vi->inode_isize = (uint32_t)sizeof(pfs_erofs_inode_compact_t);
    }
    else
    {
        return PFS_EOPNOTSUPP;
    }

    if (PFS_EROFS_INODE_FLAT_PLAIN == datalayout
            && 0 == ((startblk ^ PFS_EROFS_NULL_ADDR) & addrmask))
    {
        startblk = PFS_EROFS_NULL_ADDR;
    }

    p_vi->nid = nid;
    p_vi->mode = mode;
    p_vi->datalayout = datalayout;
    p_vi->xattr_isize = pfs_erofs_xattr_ibody_size(xattr_icount);
    p_vi->i_size = i_size;
    p_vi->startblk = startblk;
    p_vi->iloc = iloc;
    p_vi->dot_omitted = 0;
    if (PFS_EROFS_S_IFDIR == (mode & PFS_EROFS_S_IFMT))
    {
        p_vi->dot_omitted = (int)((ifmt >> PFS_EROFS_I_DOT_OMITTED_BIT) & 1u);
    }
    return PFS_OK;
}

int pfs_erofs_map_read(pfs_erofs_sbi_t *p_sbi, const pfs_erofs_inode_t *p_vi, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (NULL == p_sbi || NULL == p_vi || NULL == buf || size < 0 || 0 > pos)
    {
        return PFS_EINVAL;
    }
    if (0 == size || pos >= (int64_t)p_vi->i_size)
    {
        return PFS_OK;
    }

    if (PFS_EROFS_INODE_FLAT_PLAIN != p_vi->datalayout
            && PFS_EROFS_INODE_FLAT_INLINE != p_vi->datalayout)
    {
        return PFS_EOPNOTSUPP;
    }

    int64_t remain = (int64_t)p_vi->i_size - pos;
    if (remain > (int64_t)size)
    {
        remain = (int64_t)size;
    }

    uint32_t blksz = p_sbi->blksz;
    uint64_t iblks = pfs_erofs_iblks(p_vi, blksz);
    uint64_t flat_end;

    if (PFS_EROFS_INODE_FLAT_INLINE == p_vi->datalayout)
    {
        if (0 == iblks)
        {
            return PFS_OK;
        }
        flat_end = (iblks - 1u) * (uint64_t)blksz;
    }
    else
    {
        flat_end = p_vi->i_size;
    }

    uint8_t *out = (uint8_t *)buf;
    int total = 0;
    int64_t cur = pos;

    while (remain > 0)
    {
        uint64_t chunk;
        uint64_t abs_pa;
        uint32_t take;
        int n;

        if (cur < (int64_t)flat_end)
        {
            if (PFS_EROFS_NULL_ADDR == p_vi->startblk)
            {
                out[0] = 0;
                out++;
                cur++;
                remain--;
                total++;
                continue;
            }
            abs_pa = (p_vi->startblk << p_sbi->blkszbits) + (uint64_t)cur;
            chunk = flat_end - (uint64_t)cur;
        }
        else
        {
            // tail inline: 落在 inode meta 块
            abs_pa = p_vi->iloc + (uint64_t)p_vi->inode_isize + (uint64_t)p_vi->xattr_isize
                    + ((uint64_t)cur & ((uint64_t)blksz - 1u));
            chunk = p_vi->i_size - (uint64_t)cur;
            if (((abs_pa & ((uint64_t)blksz - 1u)) + chunk) > (uint64_t)blksz)
            {
                return PFS_EBADMSG;
            }
        }

        if (chunk > (uint64_t)remain)
        {
            chunk = (uint64_t)remain;
        }
        if (chunk > 0xffffffffu)
        {
            chunk = 0xffffffffu;
        }
        take = (uint32_t)chunk;
        n = pfs_blkio_pread(p_sbi->p_blkio, out, (int)take, p_sbi->part_offset + (int64_t)abs_pa);
        if (n != (int)take)
        {
            if (n < 0)
            {
                return pfs_erofs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        out += take;
        cur += (int64_t)take;
        remain -= (int64_t)take;
        total += (int)take;
    }

    *p_got = total;
    return PFS_OK;
}

// end
