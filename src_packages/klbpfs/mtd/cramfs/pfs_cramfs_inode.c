// Doc Encode : UTF-8 BOM, Unix(LF)
// cramfs super/inode/块读 (对照 fs/cramfs/inode.c + uncompress.c)
#include "cramfs/pfs_cramfs_internal.h"
#include "base/pfsmtd_mem.h"

#include "zlib.h"

#include <string.h>

static int pfs_cramfs_zlib_inflate(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap, int *p_out_len)
{
    if (NULL == src || NULL == dst || NULL == p_out_len || src_len <= 0 || dst_cap <= 0)
    {
        return PFS_EINVAL;
    }

    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    int zrc = inflateInit(&strm);
    if (Z_OK != zrc)
    {
        return PFS_ENOMEM;
    }
    strm.next_in = (Bytef *)src;
    strm.avail_in = (uInt)src_len;
    strm.next_out = (Bytef *)dst;
    strm.avail_out = (uInt)dst_cap;
    zrc = inflate(&strm, Z_FINISH);
    int out_len = (int)strm.total_out;
    inflateEnd(&strm);
    if (Z_STREAM_END != zrc && Z_OK != zrc)
    {
        return PFS_EIO;
    }
    if (out_len <= 0 || out_len > dst_cap)
    {
        return PFS_EIO;
    }
    *p_out_len = out_len;
    return PFS_OK;
}

static int pfs_cramfs_try_super_at(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t sb_rel,
        pfs_cramfs_super_t *p_sb, pfs_cramfs_inode_t *p_root)
{
    int n = pfs_blkio_pread(p_blkio, p_sb, (int)sizeof(*p_sb), part_offset + sb_rel);
    if (n != (int)sizeof(*p_sb))
    {
        if (0 > n)
        {
            return pfs_cramfs_map_blkio_rc(n);
        }
        return PFS_ENOEXEC;
    }
    if (PFS_CRAMFS_MAGIC_WEND == p_sb->magic)
    {
        return PFS_ENOEXEC;
    }
    if (PFS_CRAMFS_MAGIC != p_sb->magic)
    {
        return PFS_ENOEXEC;
    }
    if (0 != (p_sb->flags & ~PFS_CRAMFS_SUPPORTED_FLAGS))
    {
        return PFS_ENOEXEC;
    }
    if (0 == (p_sb->flags & PFS_CRAMFS_FLAG_WRONG_SIGNATURE))
    {
        if (0 != memcmp(p_sb->signature, PFS_CRAMFS_SIGNATURE, 16))
        {
            return PFS_ENOEXEC;
        }
    }

    pfs_cramfs_inode_decode(p_sb->root_w0, p_sb->root_w1, p_sb->root_w2, p_root);
    if (PFS_CRAMFS_S_IFDIR != (p_root->mode & PFS_CRAMFS_S_IFMT))
    {
        return PFS_ENOEXEC;
    }
    p_root->mode = (uint16_t)(p_root->mode | 0555);

    uint32_t root_off = pfs_cramfs_inode_byte_off(p_root);
    if (0 != root_off && 0 == (p_sb->flags & PFS_CRAMFS_FLAG_SHIFTED_ROOT_OFFSET))
    {
        if (root_off != (uint32_t)sizeof(*p_sb) && root_off != (uint32_t)(512 + sizeof(*p_sb)))
        {
            // 空根 offset=0 已排除; 其它非常规偏移拒绝
            if (0 != root_off)
            {
                return PFS_ENOEXEC;
            }
        }
    }
    return PFS_OK;
}

int pfs_cramfs_read_super(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t *p_sb_rel,
        pfs_cramfs_super_t *p_sb, pfs_cramfs_inode_t *p_root)
{

    if (NULL == p_blkio || NULL == p_sb_rel || NULL == p_sb || NULL == p_root || part_offset < 0)
    {
        return PFS_EINVAL;
    }

    int rc = pfs_cramfs_try_super_at(p_blkio, part_offset, 0, p_sb, p_root);
    if (PFS_OK == rc)
    {
        *p_sb_rel = 0;
        return PFS_OK;
    }
    rc = pfs_cramfs_try_super_at(p_blkio, part_offset, 512, p_sb, p_root);
    if (PFS_OK == rc)
    {
        *p_sb_rel = 512;
        return PFS_OK;
    }
    return PFS_ENOEXEC;
}

int pfs_cramfs_read_inode_at(pfs_cramfs_sbi_t *p_sbi, uint32_t abs_off, pfs_cramfs_inode_t *p_ino,
        char *name, int name_cap, int *p_nlen)
{
    if (NULL == p_sbi || NULL == p_ino)
    {
        return PFS_EINVAL;
    }

    uint8_t hdr[12];
    int n = pfs_blkio_pread(p_sbi->p_blkio, hdr, 12, p_sbi->part_offset + (int64_t)abs_off);
    if (n != 12)
    {
        if (0 > n)
        {
            return pfs_cramfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    uint32_t w0 = pfs_cramfs_le32(hdr + 0);
    uint32_t w1 = pfs_cramfs_le32(hdr + 4);
    uint32_t w2 = pfs_cramfs_le32(hdr + 8);
    pfs_cramfs_inode_decode(w0, w1, w2, p_ino);

    unsigned int padded = pfs_cramfs_name_padded(p_ino);
    if (padded > PFS_CRAMFS_MAXPATHLEN)
    {
        return PFS_EBADMSG;
    }
    int true_len = 0;
    uint8_t nbuf[PFS_CRAMFS_MAXPATHLEN];
    if (padded > 0)
    {
        n = pfs_blkio_pread(p_sbi->p_blkio, nbuf, (int)padded, p_sbi->part_offset + (int64_t)abs_off + 12);
        if (n != (int)padded)
        {
            if (0 > n)
            {
                return pfs_cramfs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        true_len = (int)padded;
        while (true_len > 0 && '\0' == nbuf[true_len - 1])
        {
            true_len--;
        }
    }
    if (NULL != p_nlen)
    {
        *p_nlen = true_len;
    }
    if (NULL != name && name_cap > 0)
    {
        int i = true_len;
        if (i >= name_cap)
        {
            i = name_cap - 1;
        }
        if (i > 0)
        {
            memcpy(name, nbuf, (size_t)i);
        }
        name[i] = '\0';
    }
    return PFS_OK;
}

int pfs_cramfs_read_page(pfs_cramfs_sbi_t *p_sbi, const pfs_cramfs_inode_t *p_ino, uint32_t data_off,
        uint32_t page_idx, uint8_t *out_page, int *p_out_len)
{
    if (NULL == p_sbi || NULL == p_ino || NULL == out_page || NULL == p_out_len)
    {
        return PFS_EINVAL;
    }

    uint32_t nblocks = (p_ino->size + PFS_CRAMFS_PAGE_SIZE - 1u) / PFS_CRAMFS_PAGE_SIZE;
    if (0 == nblocks || page_idx >= nblocks)
    {
        *p_out_len = 0;
        return PFS_OK;
    }

    int max_out = PFS_CRAMFS_PAGE_SIZE;
    if (page_idx + 1u == nblocks)
    {
        max_out = (int)(p_ino->size - page_idx * PFS_CRAMFS_PAGE_SIZE);
        if (max_out <= 0)
        {
            *p_out_len = 0;
            return PFS_OK;
        }
    }

    uint32_t ptr;
    int n;
    {
        uint8_t pb[4];
        n = pfs_blkio_pread(p_sbi->p_blkio, pb, 4,
                p_sbi->part_offset + (int64_t)data_off + (int64_t)page_idx * 4);
        if (n != 4)
        {
            if (0 > n)
            {
                return pfs_cramfs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        ptr = pfs_cramfs_le32(pb);
    }

    uint32_t flags = 0;
    if (0 != (p_sbi->flags & PFS_CRAMFS_FLAG_EXT_BLOCK_POINTERS))
    {
        flags = ptr & PFS_CRAMFS_BLK_FLAGS;
        ptr &= ~PFS_CRAMFS_BLK_FLAGS;
    }

    uint32_t start;
    uint32_t len;
    if (0 != (flags & PFS_CRAMFS_BLK_FLAG_DIRECT_PTR))
    {
        start = ptr << PFS_CRAMFS_BLK_DIRECT_PTR_SHIFT;
        if (0 != (flags & PFS_CRAMFS_BLK_FLAG_UNCOMPRESSED))
        {
            len = (uint32_t)max_out;
        }
        else
        {
            uint8_t lb[2];
            n = pfs_blkio_pread(p_sbi->p_blkio, lb, 2, p_sbi->part_offset + (int64_t)start);
            if (n != 2)
            {
                if (0 > n)
                {
                    return pfs_cramfs_map_blkio_rc(n);
                }
                return PFS_EIO;
            }
            len = pfs_cramfs_le16(lb);
            start += 2;
        }
    }
    else
    {
        uint32_t end;
        if (0 == page_idx)
        {
            start = data_off + nblocks * 4u;
        }
        else
        {
            uint32_t prev;
            uint8_t pb[4];
            n = pfs_blkio_pread(p_sbi->p_blkio, pb, 4,
                    p_sbi->part_offset + (int64_t)data_off + (int64_t)(page_idx - 1u) * 4);
            if (n != 4)
            {
                if (0 > n)
                {
                    return pfs_cramfs_map_blkio_rc(n);
                }
                return PFS_EIO;
            }
            prev = pfs_cramfs_le32(pb);
            if (0 != (p_sbi->flags & PFS_CRAMFS_FLAG_EXT_BLOCK_POINTERS))
            {
                prev &= ~PFS_CRAMFS_BLK_FLAGS;
            }
            start = prev;
        }
        end = ptr;
        if (end < start)
        {
            return PFS_EBADMSG;
        }
        len = end - start;
    }

    if (0 == len)
    {
        // hole
        memset(out_page, 0, (size_t)max_out);
        *p_out_len = max_out;
        return PFS_OK;
    }

    uint8_t *cbuf = (uint8_t *)pfsmtd_malloc((size_t)len);
    if (NULL == cbuf)
    {
        return PFS_ENOMEM;
    }
    n = pfs_blkio_pread(p_sbi->p_blkio, cbuf, (int)len, p_sbi->part_offset + (int64_t)start);
    if (n != (int)len)
    {
        pfsmtd_free(cbuf);
        if (0 > n)
        {
            return pfs_cramfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }

    if (0 != (flags & PFS_CRAMFS_BLK_FLAG_UNCOMPRESSED))
    {
        if ((int)len > max_out)
        {
            pfsmtd_free(cbuf);
            return PFS_EBADMSG;
        }
        memcpy(out_page, cbuf, (size_t)len);
        if ((int)len < max_out)
        {
            memset(out_page + len, 0, (size_t)(max_out - (int)len));
        }
        pfsmtd_free(cbuf);
        *p_out_len = max_out;
        return PFS_OK;
    }

    int out_len;
    int rc = pfs_cramfs_zlib_inflate(cbuf, (int)len, out_page, PFS_CRAMFS_PAGE_SIZE, &out_len);
    pfsmtd_free(cbuf);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (out_len < max_out)
    {
        memset(out_page + out_len, 0, (size_t)(max_out - out_len));
        out_len = max_out;
    }
    else if (out_len > max_out)
    {
        out_len = max_out;
    }
    *p_out_len = out_len;
    return PFS_OK;
}

// end
