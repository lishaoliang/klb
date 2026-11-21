// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK sparse extent: grain 映射读写; 写时按需分配 grain
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

int pfs_vmdk_sparse_open(pfs_vmdk_priv_t *p_priv, const pfs_vmdk_sparse_header_t *p_hdr)
{
    if (NULL == p_priv || NULL == p_hdr)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    if (p_hdr->capacity > (UINT64_MAX / (uint64_t)PFS_VMDK_SECTOR_SIZE))
    {
        return PFS_BLKIO_RET_STATE;
    }
    uint64_t cap_bytes = p_hdr->capacity * (uint64_t)PFS_VMDK_SECTOR_SIZE;

    p_priv->mode = PFS_VMDK_MODE_SPARSE;
    p_priv->capacity_bytes = cap_bytes;
    p_priv->sparse_flags = p_hdr->flags;
    p_priv->grain_size_sectors = p_hdr->grain_size;
    p_priv->num_gtes_per_gt = p_hdr->num_gtes_per_gt;
    p_priv->gd_offset_sectors = p_hdr->gd_offset;
    p_priv->rgd_offset_sectors = p_hdr->rgd_offset;
    p_priv->overhead_sectors = p_hdr->overhead;

    int rc = pfs_vmdk_grain_load_gd(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

static bool pfs_vmdk_sparse_range_ok(const pfs_vmdk_priv_t *p_priv, int64_t offset, int size)
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

static int pfs_vmdk_sparse_read_chunk(pfs_vmdk_priv_t *p_priv, uint8_t *buf, int size,
        int64_t offset)
{
    uint64_t grain_bytes = p_priv->grain_size_sectors * (uint64_t)PFS_VMDK_SECTOR_SIZE;
    uint32_t grain_idx = (uint32_t)((uint64_t)offset / grain_bytes);
    uint32_t off_in_grain = (uint32_t)((uint64_t)offset % grain_bytes);
    int chunk = (int)(grain_bytes - (uint64_t)off_in_grain);
    if (chunk > size)
    {
        chunk = size;
    }

    uint32_t host_sector = 0;
    int rc = pfs_vmdk_grain_lookup(p_priv, grain_idx, &host_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (PFS_VMDK_GTE_UNALLOC == host_sector || PFS_VMDK_GTE_ZEROED == host_sector)
    {
        memset(buf, 0, (size_t)chunk);
        return chunk;
    }

    int64_t host_off = (int64_t)host_sector * (int64_t)PFS_VMDK_SECTOR_SIZE + (int64_t)off_in_grain;
    rc = pfs_blkio_raw_pread(&p_priv->host, buf, chunk, host_off);
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

static int pfs_vmdk_sparse_write_chunk(pfs_vmdk_priv_t *p_priv, const uint8_t *buf, int size,
        int64_t offset)
{
    uint64_t grain_bytes = p_priv->grain_size_sectors * (uint64_t)PFS_VMDK_SECTOR_SIZE;
    uint32_t grain_idx = (uint32_t)((uint64_t)offset / grain_bytes);
    uint32_t off_in_grain = (uint32_t)((uint64_t)offset % grain_bytes);
    int chunk = (int)(grain_bytes - (uint64_t)off_in_grain);
    if (chunk > size)
    {
        chunk = size;
    }

    uint32_t host_sector = 0;
    int rc = pfs_vmdk_grain_ensure(p_priv, grain_idx, &host_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    int64_t host_off = (int64_t)host_sector * (int64_t)PFS_VMDK_SECTOR_SIZE + (int64_t)off_in_grain;
    rc = pfs_blkio_raw_pwrite(&p_priv->host, buf, chunk, host_off);
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

int pfs_vmdk_sparse_pread(pfs_vmdk_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_sparse_range_ok(p_priv, offset, size))
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
        int n = pfs_vmdk_sparse_read_chunk(p_priv, p, left, off);
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

int pfs_vmdk_sparse_pwrite(pfs_vmdk_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_sparse_range_ok(p_priv, offset, size))
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
        int n = pfs_vmdk_sparse_write_chunk(p_priv, p, left, off);
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

// end
