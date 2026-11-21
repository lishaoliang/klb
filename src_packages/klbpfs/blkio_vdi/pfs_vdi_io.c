// Doc Encode : UTF-8 BOM, Unix(LF)
// VDI 块读写: 未分配读 0; DYNAMIC 写时分配 (对齐 QEMU vdi)
#include "blkio_vdi/pfs_vdi_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdbool.h>
#include <string.h>

static bool pfs_vdi_io_range_ok(const pfs_vdi_priv_t *p_priv, int64_t offset, int size)
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
    if (off_u >= p_priv->disk_size)
    {
        return false;
    }
    uint64_t end_u = off_u + (uint64_t)size;
    if (end_u < off_u || end_u > p_priv->disk_size)
    {
        return false;
    }
    return true;
}

static int64_t pfs_vdi_host_data_off(const pfs_vdi_priv_t *p_priv, uint32_t map_entry,
        uint32_t offset_in_block)
{
    return (int64_t)p_priv->offset_data +
            (int64_t)map_entry * (int64_t)p_priv->block_size +
            (int64_t)offset_in_block;
}

// 写时分配: 新物理块序号 = blocks_allocated; 先写整块再改 map/header
static int pfs_vdi_alloc_block(pfs_vdi_priv_t *p_priv, uint32_t block_idx,
        uint32_t offset_in_block, const uint8_t *p_src, uint32_t n_bytes,
        uint32_t *p_entry)
{
    if (NULL == p_priv || NULL == p_entry || NULL == p_src)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (block_idx >= p_priv->blocks_in_hdd)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (p_priv->blocks_allocated >= p_priv->blocks_in_hdd)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint32_t entry = p_priv->blocks_allocated;
    uint8_t *p_blk = (uint8_t *)pfs_malloc(p_priv->block_size);
    if (NULL == p_blk)
    {
        return PFS_BLKIO_RET_IO;
    }

    memset(p_blk, 0, p_priv->block_size);
    memcpy(p_blk + offset_in_block, p_src, n_bytes);

    int64_t host_off = pfs_vdi_host_data_off(p_priv, entry, 0u);
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_blk, (int)p_priv->block_size, host_off);
    pfs_free(p_blk);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != (int)p_priv->block_size)
    {
        return PFS_BLKIO_RET_IO;
    }

    rc = pfs_vdi_map_set(p_priv, block_idx, entry);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    p_priv->blocks_allocated++;
    rc = pfs_vdi_header_set_blocks_allocated(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    *p_entry = entry;
    return PFS_BLKIO_RET_OK;
}

int pfs_vdi_io_pread(pfs_vdi_priv_t *p_priv, void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_map || 0u == p_priv->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vdi_io_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }

    uint8_t *p_dst = (uint8_t *)buf;
    int64_t cur = offset;
    int left = size;
    int got = 0;

    while (left > 0)
    {
        uint32_t block_idx;
        uint32_t offset_in_block;
        uint32_t n;
        uint32_t entry;

        block_idx = (uint32_t)((uint64_t)cur / (uint64_t)p_priv->block_size);
        offset_in_block = (uint32_t)((uint64_t)cur % (uint64_t)p_priv->block_size);
        n = p_priv->block_size - offset_in_block;
        if (n > (uint32_t)left)
        {
            n = (uint32_t)left;
        }
        if (block_idx >= p_priv->blocks_in_hdd)
        {
            return PFS_BLKIO_RET_PARAM;
        }

        entry = p_priv->p_map[block_idx];
        if (0 == PFS_VDI_MAP_IS_ALLOCATED(entry))
        {
            memset(p_dst, 0, n);
        }
        else
        {
            int64_t host_off = pfs_vdi_host_data_off(p_priv, entry, offset_in_block);
            int rc = pfs_blkio_raw_pread(&p_priv->host, p_dst, (int)n, host_off);
            if (0 > rc)
            {
                return rc;
            }
            else if (rc != (int)n)
            {
                return PFS_BLKIO_RET_IO;
            }
        }

        p_dst += n;
        cur += (int64_t)n;
        left -= (int)n;
        got += (int)n;
    }

    return got;
}

int pfs_vdi_io_pwrite(pfs_vdi_priv_t *p_priv, const void *buf, int size, int64_t offset)
{
    if (NULL == p_priv || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_map || 0u == p_priv->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == p_priv->want_write)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vdi_io_range_ok(p_priv, offset, size))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }

    const uint8_t *p_src = (const uint8_t *)buf;
    int64_t cur = offset;
    int left = size;
    int put = 0;

    while (left > 0)
    {
        uint32_t block_idx;
        uint32_t offset_in_block;
        uint32_t n;
        uint32_t entry;

        block_idx = (uint32_t)((uint64_t)cur / (uint64_t)p_priv->block_size);
        offset_in_block = (uint32_t)((uint64_t)cur % (uint64_t)p_priv->block_size);
        n = p_priv->block_size - offset_in_block;
        if (n > (uint32_t)left)
        {
            n = (uint32_t)left;
        }
        if (block_idx >= p_priv->blocks_in_hdd)
        {
            return PFS_BLKIO_RET_PARAM;
        }

        entry = p_priv->p_map[block_idx];
        if (0 == PFS_VDI_MAP_IS_ALLOCATED(entry))
        {
            int rc = pfs_vdi_alloc_block(p_priv, block_idx, offset_in_block, p_src, n, &entry);
            if (PFS_BLKIO_RET_OK != rc)
            {
                return rc;
            }
        }
        else
        {
            int64_t host_off = pfs_vdi_host_data_off(p_priv, entry, offset_in_block);
            int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_src, (int)n, host_off);
            if (0 > rc)
            {
                return rc;
            }
            else if (rc != (int)n)
            {
                return PFS_BLKIO_RET_IO;
            }
        }

        p_src += n;
        cur += (int64_t)n;
        left -= (int)n;
        put += (int)n;
    }

    return put;
}

// end
