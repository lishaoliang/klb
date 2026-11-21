// Doc Encode : UTF-8 BOM, Unix(LF)
// yaffs2 格式化 mkfs (2048+64 packed; 写根目录对象头) / mkvol (mkfs + 灌 src_dir)
#include "yaffs/pfs_yaffs_internal.h"
#include "base/pfsmtd_dirwalk.h"
#include "base/pfsmtd_mem.h"

#include <string.h>
#include <stdbool.h>

int pfs_yaffs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes)
{
    if (NULL == p_blkio || part_offset < 0)
    {
        return PFS_EINVAL;
    }

    int chunk_size = PFS_YAFFS_MKFS_CHUNK_SIZE;
    int spare_size = PFS_YAFFS_MKFS_SPARE_SIZE;
    int page_size = chunk_size + spare_size;
    if (page_size > (int)(4096 + 256))
    {
        return PFS_EINVAL;
    }

    int64_t sz;
    int64_t flash_size;

    sz = pfs_blkio_get_size(p_blkio);
    if (sz < 0)
    {
        return pfs_yaffs_map_blkio_rc((int)sz);
    }
    if (part_offset >= sz)
    {
        return PFS_EINVAL;
    }
    flash_size = sz - part_offset;
    if (flash_size < (int64_t)page_size)
    {
        return PFS_ENOSPC;
    }

    int64_t fill;
    int64_t ofs;
    uint8_t chunk[4096 + 256];
    pfs_yaffs_obj_hdr_t *p_hdr;
    pfs_yaffs_packed_tags2_t *p_tags;
    int n;
    int w;

    fill = flash_size;
    if (0 != vol_bytes && (int64_t)vol_bytes < fill)
    {
        fill = (int64_t)vol_bytes;
    }
    fill = (fill / (int64_t)page_size) * (int64_t)page_size;
    if (fill < (int64_t)page_size)
    {
        fill = (int64_t)page_size;
    }

    // 空页填 0xFF
    memset(chunk, 0xff, (size_t)page_size);
    for (ofs = 0; ofs < fill; )
    {
        w = (int)(fill - ofs);
        if (w > page_size)
        {
            w = page_size;
        }
        n = pfs_blkio_pwrite(p_blkio, chunk, w, part_offset + ofs);
        if (n != w)
        {
            if (n < 0)
            {
                return pfs_yaffs_map_blkio_rc(n);
            }
            return PFS_EIO;
        }
        ofs += w;
    }

    memset(chunk, 0xff, (size_t)page_size);
    p_hdr = (pfs_yaffs_obj_hdr_t *)chunk;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->type = PFS_YAFFS_OBJECT_TYPE_DIRECTORY;
    p_hdr->parent_obj_id = PFS_YAFFS_OBJECTID_ROOT;
    p_hdr->yst_mode = PFS_YAFFS_S_IFDIR | 0755;
    p_hdr->name[0] = '\0';

    p_tags = (pfs_yaffs_packed_tags2_t *)(chunk + chunk_size);
    memset(p_tags, 0, sizeof(*p_tags));
    p_tags->seq_number = PFS_YAFFS_LOWEST_SEQUENCE_NUMBER;
    p_tags->obj_id = PFS_YAFFS_OBJECTID_ROOT;
    p_tags->chunk_id = 0;
    p_tags->n_bytes = (uint32_t)sizeof(*p_hdr);

    n = pfs_blkio_pwrite(p_blkio, chunk, page_size, part_offset);
    if (n != page_size)
    {
        if (n < 0)
        {
            return pfs_yaffs_map_blkio_rc(n);
        }
        return PFS_EIO;
    }
    n = pfs_blkio_fsync(p_blkio);
    if (n < 0)
    {
        return pfs_yaffs_map_blkio_rc(n);
    }
    return PFS_OK;
}

typedef struct _pfs_yaffs_mkvol_ctx_t {
    pfs_yaffs_sbi_t        *p_sbi;
} pfs_yaffs_mkvol_ctx_t;

static int pfs_yaffs_mkvol_on_entry(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir)
{
    pfs_yaffs_mkvol_ctx_t *p_ctx;

    p_ctx = (pfs_yaffs_mkvol_ctx_t *)ctx;
    if (is_dir)
    {
        return pfs_yaffs_mkdir(p_ctx->p_sbi, vfs_rel, 0755);
    }

    pfs_yaffs_file_meta_t fm;
    uint8_t *data;
    uint32_t size;
    int put;
    int64_t pos;

    int rc = pfsmtd_host_read_file(host_path, &data, &size);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_yaffs_file_open(p_ctx->p_sbi, vfs_rel, PFS_O_RDWR | PFS_O_CREAT, &fm);
    if (PFS_OK != rc)
    {
        pfsmtd_free(data);
        return rc;
    }
    pos = 0;
    while ((uint32_t)pos < size)
    {
        int chunk;

        chunk = (int)(size - (uint32_t)pos);
        if (chunk > p_ctx->p_sbi->chunk_size)
        {
            chunk = p_ctx->p_sbi->chunk_size;
        }
        rc = pfs_yaffs_file_write(p_ctx->p_sbi, &fm, pos, data + pos, chunk, &put);
        if (PFS_OK != rc || put != chunk)
        {
            pfs_yaffs_file_close(p_ctx->p_sbi, &fm);
            pfsmtd_free(data);
            return (PFS_OK != rc) ? rc : PFS_EIO;
        }
        pos += chunk;
    }
    pfs_yaffs_file_close(p_ctx->p_sbi, &fm);
    pfsmtd_free(data);
    return PFS_OK;
}

int pfs_yaffs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id)
{
    (void)volume_id;
    if (NULL == src_dir || '\0' == src_dir[0])
    {
        return PFS_EINVAL;
    }

    pfs_yaffs_sbi_t sbi;
    pfs_yaffs_mkvol_ctx_t ctx;

    int rc = pfs_yaffs_mkfs(p_blkio, part_offset, vol_bytes);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_yaffs_init(&sbi);
    if (PFS_OK != rc)
    {
        return rc;
    }
    rc = pfs_yaffs_mount(&sbi, p_blkio, part_offset, PFS_MNT_RDWR);
    if (PFS_OK != rc)
    {
        pfs_yaffs_deinit(&sbi);
        return rc;
    }
    ctx.p_sbi = &sbi;
    rc = pfsmtd_dirwalk(src_dir, pfs_yaffs_mkvol_on_entry, &ctx);
    (void)pfs_yaffs_umount(&sbi);
    pfs_yaffs_deinit(&sbi);
    return rc;
}

// end
