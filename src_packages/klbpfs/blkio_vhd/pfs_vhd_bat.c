// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD BAT: Dynamic / Differencing 块分配表
#include "blkio_vhd/pfs_vhd_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

uint32_t pfs_vhd_dyn_bitmap_sectors(uint32_t block_size)
{
    if (0 == block_size || 0 != (block_size % (uint32_t)PFS_VHD_SECTOR_SIZE))
    {
        return 0;
    }

    uint32_t sectors = block_size / (uint32_t)PFS_VHD_SECTOR_SIZE;
    uint32_t bitmap_bytes = (sectors + 7u) / 8u;
    return (bitmap_bytes + (uint32_t)PFS_VHD_SECTOR_SIZE - 1u) / (uint32_t)PFS_VHD_SECTOR_SIZE;
}

int pfs_vhd_bat_load(pfs_vhd_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->max_table_entries || 0 == p_priv->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhd_bat_free(p_priv);

    uint32_t n = p_priv->max_table_entries;
    uint64_t bytes = (uint64_t)n * 4ull;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size || p_priv->table_offset > (uint64_t)host_size ||
            (uint64_t)host_size - p_priv->table_offset < bytes)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_bat = (uint32_t *)pfs_malloc((size_t)n * sizeof(uint32_t));
    if (NULL == p_priv->p_bat)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->table_offset);
    if (0 > rc)
    {
        pfs_free(p_raw);
        pfs_vhd_bat_free(p_priv);
        return rc;
    }
    else if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_vhd_bat_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < n; i++)
    {
        p_priv->p_bat[i] = pfs_read_be32(p_raw + i * 4u);
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

void pfs_vhd_bat_free(pfs_vhd_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    if (NULL != p_priv->p_bat)
    {
        pfs_free(p_priv->p_bat);
        p_priv->p_bat = NULL;
    }
}

int pfs_vhd_bat_lookup(const pfs_vhd_priv_t *p_priv, uint32_t block_idx, uint32_t *p_sector)
{
    if (NULL == p_priv || NULL == p_sector)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_bat || block_idx >= p_priv->max_table_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_sector = p_priv->p_bat[block_idx];
    return PFS_BLKIO_RET_OK;
}

int pfs_vhd_bat_set(pfs_vhd_priv_t *p_priv, uint32_t block_idx, uint32_t sector)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_bat || block_idx >= p_priv->max_table_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->p_bat[block_idx] = sector;
    uint8_t be[4];
    pfs_write_be32(be, sector);
    int64_t off = (int64_t)p_priv->table_offset + (int64_t)block_idx * 4;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, be, 4, off);
    if (0 > rc)
    {
        return rc;
    }
    else if (4 != rc)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

// end
