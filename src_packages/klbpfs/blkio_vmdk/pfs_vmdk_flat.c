// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK FLAT/ZERO extent: 描述符打开与线性读写
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void pfs_vmdk_flat_close_extents(pfs_vmdk_priv_t *p_priv)
{
    if (NULL == p_priv || NULL == p_priv->p_extents)
    {
        return;
    }
    for (int i = 0; i < p_priv->extent_count; i++)
    {
        pfs_vmdk_extent_t *p_ext = &p_priv->p_extents[i];
        if (p_ext->host_open)
        {
            (void)pfs_blkio_raw_close(&p_ext->host);
            p_ext->host_open = 0;
        }
    }
    pfs_free(p_priv->p_extents);
    p_priv->p_extents = NULL;
    p_priv->extent_count = 0;
}

int pfs_vmdk_flat_open_from_desc(pfs_vmdk_priv_t *p_priv, const pfs_vmdk_desc_t *p_desc,
        pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_priv || NULL == p_desc)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (p_desc->extent_count <= 0 || p_desc->extent_count > PFS_VMDK_EXTENT_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    // 本阶段仅支持 FLAT + ZERO 组合 (monolithicFlat / 分片 flat)
    int flat_or_zero = 0;
    for (int i = 0; i < p_desc->extent_count; i++)
    {
        int t = p_desc->extents[i].type;
        if (PFS_VMDK_EXT_FLAT == t || PFS_VMDK_EXT_ZERO == t)
        {
            flat_or_zero++;
        }
        else if (PFS_VMDK_EXT_SPARSE == t)
        {
            // 描述符指向独立 sparse 文件: 本阶段拒绝 (请直接打开该 sparse 文件)
            return PFS_BLKIO_RET_STATE;
        }
        else
        {
            return PFS_BLKIO_RET_STATE;
        }
    }
    if (0 == flat_or_zero)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->p_extents = (pfs_vmdk_extent_t *)pfs_mallocz(
            (size_t)p_desc->extent_count * sizeof(pfs_vmdk_extent_t));
    if (NULL == p_priv->p_extents)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->extent_count = p_desc->extent_count;

    uint64_t total_sectors = 0;
    for (int i = 0; i < p_desc->extent_count; i++)
    {
        pfs_vmdk_extent_t *p_ext = &p_priv->p_extents[i];
        const pfs_vmdk_desc_extent_t *p_de = &p_desc->extents[i];
        char full[PFS_VMDK_PATH_MAX];

        p_ext->type = p_de->type;
        p_ext->sectors = p_de->sectors;
        p_ext->flat_offset_sectors = p_de->flat_offset_sectors;
        p_ext->host_open = 0;

        if (total_sectors > UINT64_MAX - p_de->sectors)
        {
            pfs_vmdk_flat_close_extents(p_priv);
            return PFS_BLKIO_RET_STATE;
        }
        total_sectors += p_de->sectors;

        if (PFS_VMDK_EXT_ZERO == p_de->type)
        {
            p_ext->path[0] = '\0';
            continue;
        }

        int rc = pfs_vmdk_path_join(p_priv->base_dir, p_de->path, full, sizeof(full));
        if (PFS_BLKIO_RET_OK != rc)
        {
            pfs_vmdk_flat_close_extents(p_priv);
            return rc;
        }
        memcpy(p_ext->path, full, sizeof(p_ext->path));

        (void)pfs_blkio_init(&p_ext->host);
        p_ext->host.ops = &g_pfs_blkio_raw_ops;
        rc = pfs_blkio_raw_open(&p_ext->host, p_ext->path, flags, sector_size);
        if (PFS_BLKIO_RET_OK != rc)
        {
            pfs_vmdk_flat_close_extents(p_priv);
            return rc;
        }
        p_ext->host_open = 1;
    }

    if (total_sectors > UINT64_MAX / (uint64_t)PFS_VMDK_SECTOR_SIZE)
    {
        pfs_vmdk_flat_close_extents(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->mode = PFS_VMDK_MODE_FLAT;
    p_priv->capacity_bytes = total_sectors * (uint64_t)PFS_VMDK_SECTOR_SIZE;
    return PFS_BLKIO_RET_OK;
}

static bool pfs_vmdk_flat_range_ok(const pfs_vmdk_priv_t *p_priv, int64_t offset, int size)
{
    if (offset < 0 || size < 0 || size > PFS_BLKIO_IO_SIZE_MAX)
    {
        return false;
    }
    if (0 == size)
    {
        return true;
    }

    uint64_t off_u = (uint64_t)offset;
    if (off_u >= p_priv->capacity_bytes)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->capacity_bytes)
    {
        return false;
    }
    return true;
}

static int pfs_vmdk_flat_find(pfs_vmdk_priv_t *p_priv, uint64_t sector,
        pfs_vmdk_extent_t **pp_ext, uint64_t *p_sec_in_ext)
{
    uint64_t base = 0;

    for (int i = 0; i < p_priv->extent_count; i++)
    {
        pfs_vmdk_extent_t *p_ext = &p_priv->p_extents[i];
        if (sector < base + p_ext->sectors)
        {
            *pp_ext = p_ext;
            *p_sec_in_ext = sector - base;
            return PFS_BLKIO_RET_OK;
        }
        base += p_ext->sectors;
    }
    return PFS_BLKIO_RET_PARAM;
}

static int pfs_vmdk_flat_io_chunk(pfs_vmdk_priv_t *p_priv, void *buf, const void *cbuf,
        int size, int64_t offset, bool is_write)
{
    uint64_t sector = (uint64_t)offset / (uint64_t)PFS_VMDK_SECTOR_SIZE;
    uint64_t sec_off = (uint64_t)offset % (uint64_t)PFS_VMDK_SECTOR_SIZE;
    pfs_vmdk_extent_t *p_ext;
    uint64_t sec_in_ext;

    int rc = pfs_vmdk_flat_find(p_priv, sector, &p_ext, &sec_in_ext);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    // 本 extent 内剩余字节
    int chunk;
    {
        uint64_t left_in_ext = (p_ext->sectors - sec_in_ext) * (uint64_t)PFS_VMDK_SECTOR_SIZE -
                sec_off;
        if (left_in_ext > (uint64_t)INT32_MAX)
        {
            left_in_ext = (uint64_t)INT32_MAX;
        }
        chunk = (int)left_in_ext;
        if (chunk > size)
        {
            chunk = size;
        }
    }

    if (PFS_VMDK_EXT_ZERO == p_ext->type)
    {
        if (is_write)
        {
            return PFS_BLKIO_RET_STATE;
        }
        memset(buf, 0, (size_t)chunk);
        return chunk;
    }

    if (!p_ext->host_open)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_off = (int64_t)(p_ext->flat_offset_sectors + sec_in_ext) * (int64_t)PFS_VMDK_SECTOR_SIZE +
            (int64_t)sec_off;
    if (is_write)
    {
        rc = pfs_blkio_raw_pwrite(&p_ext->host, cbuf, chunk, host_off);
    }
    else
    {
        rc = pfs_blkio_raw_pread(&p_ext->host, buf, chunk, host_off);
    }
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != chunk)
    {
        return PFS_BLKIO_RET_IO;
    }
    return chunk;
}

int pfs_vmdk_flat_pread(pfs_vmdk_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_flat_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint8_t *p = (uint8_t *)buf;
    int left = size;
    int64_t off = offset;
    while (left > 0)
    {
        int n = pfs_vmdk_flat_io_chunk(p_priv, p, NULL, left, off, false);
        if (0 > n)
        {
            return n;
        }
        else if (0 == n)
        {
            return PFS_BLKIO_RET_IO;
        }
        p += n;
        off += (int64_t)n;
        left -= n;
    }
    return size;
}

int pfs_vmdk_flat_pwrite(pfs_vmdk_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_flat_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    const uint8_t *p = (const uint8_t *)buf;
    int left = size;
    int64_t off = offset;
    while (left > 0)
    {
        int n = pfs_vmdk_flat_io_chunk(p_priv, NULL, p, left, off, true);
        if (0 > n)
        {
            return n;
        }
        else if (0 == n)
        {
            return PFS_BLKIO_RET_IO;
        }
        p += n;
        off += (int64_t)n;
        left -= n;
    }
    return size;
}

int pfs_vmdk_flat_fsync(pfs_vmdk_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int first_err = PFS_BLKIO_RET_OK;
    for (int i = 0; i < p_priv->extent_count; i++)
    {
        pfs_vmdk_extent_t *p_ext = &p_priv->p_extents[i];
        if (!p_ext->host_open)
        {
            continue;
        }
        int rc = pfs_blkio_raw_fsync(&p_ext->host);
        if (PFS_BLKIO_RET_OK != rc && PFS_BLKIO_RET_OK == first_err)
        {
            first_err = rc;
        }
    }
    return first_err;
}

// end
