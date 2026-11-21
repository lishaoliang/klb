// Doc Encode : UTF-8 BOM, Unix(LF)
// jffs2 文件读写 (NONE/ZERO/ZLIB 读; creat + EOF append COMPR_NONE)
#include "jffs2/pfs_jffs2_internal.h"
#include "base/pfsmtd_mem.h"

#include "zlib.h"

#include <string.h>
#include <stdbool.h>

static int pfs_jffs2_zlib_inflate(const uint8_t *src, int src_len, uint8_t *dst, int dst_cap, int *p_out_len)
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

static pfs_jffs2_dnode_t *pfs_jffs2_best_dnode(pfs_jffs2_ic_t *p_ic, uint32_t file_ofs)
{
    pfs_jffs2_dnode_t *p_best = NULL;

    for (pfs_jffs2_dnode_t *p_dn = p_ic->nodes; NULL != p_dn; p_dn = p_dn->next)
    {
        if (p_dn->dsize <= 0)
        {
            continue;
        }
        if (file_ofs < p_dn->offset || file_ofs >= p_dn->offset + p_dn->dsize)
        {
            continue;
        }
        if (NULL == p_best || p_dn->version > p_best->version)
        {
            p_best = p_dn;
        }
    }
    return p_best;
}

static int pfs_jffs2_read_dnode(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_dnode_t *p_dn, uint32_t file_ofs, uint8_t *dst, int want, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (want <= 0)
    {
        return PFS_OK;
    }
    if (file_ofs < p_dn->offset || file_ofs >= p_dn->offset + p_dn->dsize)
    {
        return PFS_EINVAL;
    }

    uint32_t rel = file_ofs - p_dn->offset;
    uint32_t copy_n = p_dn->dsize - rel;
    if (copy_n > (uint32_t)want)
    {
        copy_n = (uint32_t)want;
    }

    if (PFS_JFFS2_COMPR_ZERO == p_dn->compr)
    {
        memset(dst, 0, (size_t)copy_n);
        *p_got = (int)copy_n;
        return PFS_OK;
    }

    pfs_jffs2_raw_inode_t ri;
    int n = pfs_blkio_pread(p_sbi->p_blkio, &ri, (int)sizeof(ri), p_sbi->part_offset + (int64_t)p_dn->flash_ofs);
    if (n != (int)sizeof(ri))
    {
        if (0 > n)
        {
            return pfs_jffs2_map_blkio_rc(n);
        }
        return PFS_EIO;
    }

    int64_t abs_data = p_sbi->part_offset + (int64_t)p_dn->flash_ofs + (int64_t)sizeof(ri);

    if (PFS_JFFS2_COMPR_NONE == p_dn->compr)
    {
        if (p_dn->csize != p_dn->dsize)
        {
            return PFS_EBADMSG;
        }
        uint8_t *cbuf = (uint8_t *)pfsmtd_malloc((size_t)p_dn->csize);
        if (NULL == cbuf)
        {
            return PFS_ENOMEM;
        }
        n = pfs_blkio_pread(p_sbi->p_blkio, cbuf, (int)p_dn->csize, abs_data);
        if (n != (int)p_dn->csize)
        {
            pfsmtd_free(cbuf);
            if (0 > n)
            {
                return pfs_jffs2_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        uint32_t crc = pfs_jffs2_crc32(cbuf, (size_t)p_dn->csize);
        if (crc != ri.data_crc)
        {
            pfsmtd_free(cbuf);
            return PFS_EBADMSG;
        }
        memcpy(dst, cbuf + rel, (size_t)copy_n);
        pfsmtd_free(cbuf);
        *p_got = (int)copy_n;
        return PFS_OK;
    }

    if (PFS_JFFS2_COMPR_ZLIB == p_dn->compr)
    {
        uint8_t *cbuf = (uint8_t *)pfsmtd_malloc((size_t)p_dn->csize);
        uint8_t *dbuf = (uint8_t *)pfsmtd_malloc((size_t)p_dn->dsize);
        if (NULL == cbuf || NULL == dbuf)
        {
            pfsmtd_free(cbuf);
            pfsmtd_free(dbuf);
            return PFS_ENOMEM;
        }
        n = pfs_blkio_pread(p_sbi->p_blkio, cbuf, (int)p_dn->csize, abs_data);
        if (n != (int)p_dn->csize)
        {
            pfsmtd_free(cbuf);
            pfsmtd_free(dbuf);
            if (0 > n)
            {
                return pfs_jffs2_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        uint32_t crc = pfs_jffs2_crc32(cbuf, (size_t)p_dn->csize);
        if (crc != ri.data_crc)
        {
            pfsmtd_free(cbuf);
            pfsmtd_free(dbuf);
            return PFS_EBADMSG;
        }
        int out_len;
        int rc = pfs_jffs2_zlib_inflate(cbuf, (int)p_dn->csize, dbuf, (int)p_dn->dsize, &out_len);
        pfsmtd_free(cbuf);
        if (PFS_OK != rc || (uint32_t)out_len != p_dn->dsize)
        {
            pfsmtd_free(dbuf);
            return (PFS_OK != rc) ? rc : PFS_EIO;
        }
        memcpy(dst, dbuf + rel, (size_t)copy_n);
        pfsmtd_free(dbuf);
        *p_got = (int)copy_n;
        return PFS_OK;
    }

    return PFS_EOPNOTSUPP;
}

int pfs_jffs2_file_open(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode, pfs_jffs2_file_meta_t *p_fm)
{
    if (NULL == p_sbi || NULL == path || NULL == p_fm)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }

    memset(p_fm, 0, sizeof(*p_fm));
    int want_create = (0 != (mode & PFS_O_CREAT)) ? 1 : 0;
    int want_write = (0 != (mode & PFS_O_RDWR)) ? 1 : 0;
    if (want_create && !want_write)
    {
        return PFS_EINVAL;
    }
    if ((want_create || want_write) && !pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }

    uint32_t ino;
    bool is_dir;

    int rc = pfs_jffs2_lookup_path(p_sbi, path, &ino, &is_dir);
    if (PFS_ENOENT == rc && want_create)
    {
        rc = pfs_jffs2_creat(p_sbi, path, 0644u, &ino);
        if (PFS_OK != rc)
        {
            return rc;
        }
        is_dir = false;
    }
    else if (PFS_OK != rc)
    {
        return rc;
    }
    if (is_dir)
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, ino);
    p_fm->ino = ino;
    p_fm->isize = (NULL != p_ic) ? p_ic->isize : 0;
    p_fm->writable = want_write;
    return PFS_OK;
}

void pfs_jffs2_file_close(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL != p_fm)
    {
        memset(p_fm, 0, sizeof(*p_fm));
    }
}

int pfs_jffs2_file_read(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got)
{
    if (NULL == p_got)
    {
        return PFS_EINVAL;
    }
    *p_got = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || size < 0 || pos < 0)
    {
        return PFS_EINVAL;
    }
    if (!pfs_jffs2_sbi_is_mounted(p_sbi))
    {
        return PFS_EINVAL;
    }
    if (0 == size)
    {
        return PFS_OK;
    }
    if (pos >= (int64_t)p_fm->isize)
    {
        return PFS_OK;
    }

    int64_t remain = (int64_t)p_fm->isize - pos;
    if (remain > (int64_t)size)
    {
        remain = (int64_t)size;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, p_fm->ino);
    if (NULL == p_ic)
    {
        // 无数据节点: 当空洞填 0
        memset(buf, 0, (size_t)remain);
        *p_got = (int)remain;
        return PFS_OK;
    }

    uint8_t *out = (uint8_t *)buf;
    int total = 0;
    uint32_t cur = (uint32_t)pos;
    while (remain > 0)
    {
        pfs_jffs2_dnode_t *p_best = pfs_jffs2_best_dnode(p_ic, cur);
        if (NULL == p_best)
        {
            // 稀疏空洞
            out[0] = 0;
            out++;
            cur++;
            remain--;
            total++;
            continue;
        }
        int got;
        int rc = pfs_jffs2_read_dnode(p_sbi, p_best, cur, out, (int)remain, &got);
        if (PFS_OK != rc)
        {
            return rc;
        }
        if (got <= 0)
        {
            return PFS_EIO;
        }
        out += got;
        cur += (uint32_t)got;
        remain -= got;
        total += got;
    }

    *p_got = total;
    return PFS_OK;
}

int pfs_jffs2_file_write(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put)
{
    if (NULL == p_put)
    {
        return PFS_EINVAL;
    }
    *p_put = 0;
    if (NULL == p_sbi || NULL == p_fm || NULL == buf || size < 0 || pos < 0)
    {
        return PFS_EINVAL;
    }
    if (!p_fm->writable || !pfs_jffs2_sbi_can_write(p_sbi))
    {
        return PFS_EPERM;
    }
    if (0 == size)
    {
        return PFS_OK;
    }
    if (pos != (int64_t)p_fm->isize)
    {
        return PFS_EINVAL;
    }

    pfs_jffs2_ic_t *p_ic = pfs_jffs2_ic_find(p_sbi, p_fm->ino);
    if (NULL == p_ic)
    {
        return PFS_ENOENT;
    }
    uint32_t mode = p_ic->mode;
    if (0 == (mode & PFS_JFFS2_S_IFMT))
    {
        mode = PFS_JFFS2_S_IFREG | 0644;
    }

    const uint8_t *in = (const uint8_t *)buf;
    int left = size;
    while (left > 0)
    {
        int chunk = left;
        if (chunk > PFS_JFFS2_WRITE_CHUNK_MAX)
        {
            chunk = PFS_JFFS2_WRITE_CHUNK_MAX;
        }
        uint32_t off = p_fm->isize;
        uint32_t new_size = p_fm->isize + (uint32_t)chunk;
        uint32_t ver = p_ic->highest_version + 1;
        int rc = pfs_jffs2_emit_inode(p_sbi, p_fm->ino, ver, mode, new_size, off, (uint32_t)chunk, in, PFS_JFFS2_COMPR_NONE);
        if (PFS_OK != rc)
        {
            return rc;
        }
        p_ic->highest_version = ver;
        p_ic->isize = new_size;
        p_fm->isize = new_size;
        in += chunk;
        left -= chunk;
    }

    *p_put = size;
    return PFS_OK;
}

int pfs_jffs2_file_truncate(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t size)
{
    (void)p_sbi;
    (void)p_fm;
    (void)size;
    return PFS_EPERM;
}

int pfs_jffs2_file_sync(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm)
{
    (void)p_fm;
    if (NULL == p_sbi)
    {
        return PFS_EINVAL;
    }
    if (pfs_jffs2_sbi_can_write(p_sbi))
    {
        return pfs_jffs2_sync_media(p_sbi);
    }
    return PFS_OK;
}

int64_t pfs_jffs2_file_get_size(pfs_jffs2_sbi_t *p_sbi, const pfs_jffs2_file_meta_t *p_fm)
{
    (void)p_sbi;
    if (NULL == p_fm)
    {
        return -1;
    }
    return (int64_t)p_fm->isize;
}

// end
