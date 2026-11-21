// Doc Encode : UTF-8 BOM, Unix(LF)
// squashfs 块读: 元数据流 / 数据块 / 表; zlib 解压 (对照 cache.c + block 语义)
#include "squashfs/pfs_squashfs_internal.h"
#include "base/pfsmtd_mem.h"
#include "base/pfsmtd_util.h"

#include "zlib.h"

#include <string.h>

static int pfs_squashfs_pread(pfs_squashfs_sbi_t *p_sbi, void *buf, int len, int64_t abs_off)
{
    if (len <= 0)
    {
        return PFS_EINVAL;
    }
    if (abs_off < p_sbi->part_offset)
    {
        return PFS_EIO;
    }
    if ((uint64_t)(abs_off - p_sbi->part_offset) + (uint64_t)len > p_sbi->bytes_used
            && p_sbi->bytes_used > 0)
    {
        // bytes_used 覆盖有效映像; 允许等于边界
        if ((uint64_t)(abs_off - p_sbi->part_offset) >= p_sbi->bytes_used)
        {
            return PFS_EIO;
        }
    }

    int n = pfs_blkio_pread(p_sbi->p_blkio, buf, len, abs_off);
    if (n != len)
    {
        if (n < 0)
        {
            return pfs_squashfs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    return PFS_OK;
}

static int pfs_squashfs_zlib_inflate(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap,
        int *p_out_len)
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

// length==0: 元数据块 (2 字节头); 否则数据块 size_code
static int pfs_squashfs_load_block(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_rel, int length,
        uint8_t *out, int out_cap, int *p_out_len, uint64_t *p_next)
{
    int64_t abs_off = p_sbi->part_offset + (int64_t)disk_rel;
    int compressed;
    int csize;

    if (0 == length)
    {
        uint8_t hdr[2];
        int rc = pfs_squashfs_pread(p_sbi, hdr, 2, abs_off);
        if (PFS_OK != rc)
        {
            return rc;
        }
        csize = (int)pfs_squashfs_le16(hdr);
        compressed = PFS_SQUASHFS_COMPRESSED(csize);
        csize = (int)PFS_SQUASHFS_COMPRESSED_SIZE((unsigned int)csize);
        if (csize <= 0 || csize > PFS_SQUASHFS_METADATA_SIZE)
        {
            return PFS_EIO;
        }
        abs_off += 2;
        if (NULL != p_next)
        {
            *p_next = disk_rel + 2u + (uint64_t)csize;
        }
        if (out_cap < PFS_SQUASHFS_METADATA_SIZE)
        {
            return PFS_EINVAL;
        }
    }
    else
    {
        compressed = PFS_SQUASHFS_COMPRESSED_BLOCK((unsigned int)length);
        csize = (int)PFS_SQUASHFS_COMPRESSED_SIZE_BLOCK((unsigned int)length);
        if (csize <= 0 || csize > (int)p_sbi->block_size)
        {
            return PFS_EIO;
        }
        if (NULL != p_next)
        {
            *p_next = disk_rel + (uint64_t)csize;
        }
        if (out_cap < (int)p_sbi->block_size)
        {
            return PFS_EINVAL;
        }
    }

    if (!compressed)
    {
        if (csize > out_cap)
        {
            return PFS_EIO;
        }

        int rc = pfs_squashfs_pread(p_sbi, out, csize, abs_off);
        if (PFS_OK != rc)
        {
            return rc;
        }
        *p_out_len = csize;
        return PFS_OK;
    }

    if (PFS_SQUASHFS_COMP_ZLIB != p_sbi->compression)
    {
        return PFS_EOPNOTSUPP;
    }

    int rc = pfs_squashfs_pread(p_sbi, p_sbi->comp_buf, csize, abs_off);
    if (PFS_OK != rc)
    {
        return rc;
    }

    int out_len;

    rc = pfs_squashfs_zlib_inflate(p_sbi->comp_buf, csize, out, out_cap, &out_len);
    if (PFS_OK != rc)
    {
        return rc;
    }
    if (0 == length)
    {
        if (out_len > PFS_SQUASHFS_METADATA_SIZE)
        {
            return PFS_EIO;
        }
    }
    *p_out_len = out_len;
    return PFS_OK;
}

static int pfs_squashfs_meta_get(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_rel)
{
    if (p_sbi->meta_block == disk_rel && p_sbi->meta_len > 0)
    {
        return PFS_OK;
    }

    int out_len;
    uint64_t next;
    int rc = pfs_squashfs_load_block(p_sbi, disk_rel, 0, p_sbi->meta_data, PFS_SQUASHFS_METADATA_SIZE,
            &out_len, &next);
    if (PFS_OK != rc)
    {
        p_sbi->meta_block = (uint64_t)PFS_SQUASHFS_INVALID_BLK;
        p_sbi->meta_len = 0;
        return rc;
    }

    p_sbi->meta_block = disk_rel;
    p_sbi->meta_next = next;
    p_sbi->meta_len = out_len;
    return PFS_OK;
}

int pfs_squashfs_read_metadata(pfs_squashfs_sbi_t *p_sbi, void *buf, uint64_t *p_block, int *p_offset,
        int length)
{
    if (NULL == p_sbi || NULL == p_block || NULL == p_offset || length < 0)
    {
        return PFS_EINVAL;
    }
    if (*p_offset < 0 || *p_offset >= PFS_SQUASHFS_METADATA_SIZE)
    {
        return PFS_EIO;
    }

    uint8_t *dst = (uint8_t *)buf;
    int remain = length;

    while (remain > 0)
    {
        int rc = pfs_squashfs_meta_get(p_sbi, *p_block);
        if (PFS_OK != rc)
        {
            return rc;
        }
        if (*p_offset >= p_sbi->meta_len)
        {
            return PFS_EIO;
        }

        int bytes = p_sbi->meta_len - *p_offset;
        if (bytes > remain)
        {
            bytes = remain;
        }
        if (NULL != dst)
        {
            memcpy(dst, p_sbi->meta_data + *p_offset, (size_t)bytes);
            dst += bytes;
        }
        remain -= bytes;
        *p_offset += bytes;

        if (*p_offset == p_sbi->meta_len)
        {
            *p_block = p_sbi->meta_next;
            *p_offset = 0;
        }
    }

    return length;
}

int pfs_squashfs_read_data_block(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_block, int size_code,
        uint8_t *out, int out_cap, int *p_out_len)
{
    if (NULL == p_sbi || NULL == out || NULL == p_out_len)
    {
        return PFS_EINVAL;
    }
    if (0 == size_code)
    {
        // sparse hole
        if (out_cap < (int)p_sbi->block_size)
        {
            return PFS_EINVAL;
        }
        memset(out, 0, (size_t)p_sbi->block_size);
        *p_out_len = (int)p_sbi->block_size;
        return PFS_OK;
    }
    return pfs_squashfs_load_block(p_sbi, disk_block, size_code, out, out_cap, p_out_len, NULL);
}

int pfs_squashfs_read_table(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_off, void *buf, int length)
{
    if (NULL == p_sbi || NULL == buf || length <= 0)
    {
        return PFS_EINVAL;
    }

    int64_t abs_off = p_sbi->part_offset + (int64_t)disk_off;

    return pfs_squashfs_pread(p_sbi, buf, length, abs_off);
}

int pfs_squashfs_frag_lookup(pfs_squashfs_sbi_t *p_sbi, unsigned int fragment, uint64_t *p_frag_block)
{
    if (NULL == p_sbi || NULL == p_frag_block)
    {
        return PFS_EINVAL;
    }
    if (0 == p_sbi->fragments || NULL == p_sbi->fragment_index)
    {
        return PFS_EIO;
    }
    if (fragment >= p_sbi->fragments)
    {
        return PFS_EIO;
    }

    unsigned int block = PFS_SQUASHFS_FRAGMENT_INDEX(fragment);
    int offset = (int)PFS_SQUASHFS_FRAGMENT_INDEX_OFFSET(fragment);

    if (block >= p_sbi->fragment_index_count)
    {
        return PFS_EIO;
    }

    uint64_t start_block = p_sbi->fragment_index[block];
    uint8_t raw[sizeof(pfs_squashfs_fragment_entry_t)];
    int rc = pfs_squashfs_read_metadata(p_sbi, raw, &start_block, &offset, (int)sizeof(raw));
    if (rc < 0)
    {
        return rc;
    }

    pfs_squashfs_fragment_entry_t ent;

    ent.start_block = pfs_squashfs_le64(raw);
    ent.size = pfs_squashfs_le32(raw + 8);
    *p_frag_block = ent.start_block;
    int size_code = pfs_squashfs_block_size_u32(ent.size);
    if (size_code < 0)
    {
        return PFS_EIO;
    }
    return size_code;
}

// end
