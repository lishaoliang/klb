// Doc Encode : UTF-8 BOM, Unix(LF)
// VDI block map: 加载 / 落盘单项 / 更新 header.blocks_allocated
#include "blkio_vdi/pfs_vdi_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_vdi_map_free(pfs_vdi_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_free(p_priv->p_map);
    p_priv->p_map = NULL;
}

int pfs_vdi_map_load(pfs_vdi_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0u == p_priv->blocks_in_hdd || 0u == p_priv->block_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vdi_map_free(p_priv);

    uint32_t n = p_priv->blocks_in_hdd;
    uint64_t bytes = (uint64_t)n * 4ull;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size || (uint64_t)p_priv->offset_blocks > (uint64_t)host_size ||
            (uint64_t)host_size - (uint64_t)p_priv->offset_blocks < bytes)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_map = (uint32_t *)pfs_malloc((size_t)n * sizeof(uint32_t));
    if (NULL == p_priv->p_map)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->offset_blocks);
    if (0 > rc)
    {
        pfs_free(p_raw);
        pfs_vdi_map_free(p_priv);
        return rc;
    }
    else if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_vdi_map_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < n; i++)
    {
        p_priv->p_map[i] = pfs_read_le32(p_raw + (size_t)i * 4u);
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

int pfs_vdi_map_set(pfs_vdi_priv_t *p_priv, uint32_t block_idx, uint32_t entry)
{
    if (NULL == p_priv || NULL == p_priv->p_map)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (block_idx >= p_priv->blocks_in_hdd)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_priv->p_map[block_idx] = entry;
    uint8_t le[4];
    pfs_write_le32(le, entry);
    int64_t off = (int64_t)p_priv->offset_blocks + (int64_t)block_idx * 4;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, le, 4, off);
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

int pfs_vdi_header_set_blocks_allocated(pfs_vdi_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint8_t le[4];
    pfs_write_le32(le, p_priv->blocks_allocated);
    int64_t off = (int64_t)PFS_VDI_PRE_HEADER_SIZE + (int64_t)PFS_VDI_OFF_BLOCKS_ALLOCATED;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, le, 4, off);
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
