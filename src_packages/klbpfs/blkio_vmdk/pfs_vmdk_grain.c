// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK Grain Directory / Grain Table: 查找与按需分配
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_vmdk_grain_free(pfs_vmdk_priv_t *p_priv)
{

    if (NULL == p_priv)
    {
        return;
    }
    if (NULL != p_priv->pp_gt)
    {
        for (uint32_t i = 0; i < p_priv->gd_count; i++)
        {
            if (NULL != p_priv->pp_gt[i])
            {
                pfs_free(p_priv->pp_gt[i]);
                p_priv->pp_gt[i] = NULL;
            }
        }
        pfs_free(p_priv->pp_gt);
        p_priv->pp_gt = NULL;
    }
    if (NULL != p_priv->p_gd)
    {
        pfs_free(p_priv->p_gd);
        p_priv->p_gd = NULL;
    }
    p_priv->gd_count = 0;
    p_priv->grain_count = 0;
}

int pfs_vmdk_grain_load_gd(pfs_vmdk_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->grain_size_sectors || 0 == p_priv->num_gtes_per_gt ||
            0 == p_priv->gd_offset_sectors)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vmdk_grain_free(p_priv);

    uint64_t grains = p_priv->capacity_bytes / ((uint64_t)PFS_VMDK_SECTOR_SIZE * p_priv->grain_size_sectors);
    if (0 == grains || grains > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    p_priv->grain_count = (uint32_t)grains;

    uint64_t gd_count_u = (grains + (uint64_t)p_priv->num_gtes_per_gt - 1ull) /
            (uint64_t)p_priv->num_gtes_per_gt;
    if (0 == gd_count_u || gd_count_u > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    p_priv->gd_count = (uint32_t)gd_count_u;

    uint64_t bytes = gd_count_u * 4ull;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t off = (int64_t)p_priv->gd_offset_sectors * (int64_t)PFS_VMDK_SECTOR_SIZE;
    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < 0 || off < 0 || (uint64_t)off + bytes > (uint64_t)host_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_gd = (uint32_t *)pfs_malloc((size_t)p_priv->gd_count * sizeof(uint32_t));
    if (NULL == p_priv->p_gd)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }
    p_priv->pp_gt = (uint32_t **)pfs_mallocz((size_t)p_priv->gd_count * sizeof(uint32_t *));
    if (NULL == p_priv->pp_gt)
    {
        pfs_free(p_raw);
        pfs_free(p_priv->p_gd);
        p_priv->p_gd = NULL;
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, off);
    if (0 > rc)
    {
        pfs_free(p_raw);
        pfs_vmdk_grain_free(p_priv);
        return rc;
    }
    else if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_vmdk_grain_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < p_priv->gd_count; i++)
    {
        p_priv->p_gd[i] = pfs_read_le32(p_raw + i * 4u);
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

static int pfs_vmdk_grain_load_gt(pfs_vmdk_priv_t *p_priv, uint32_t gd_idx)
{
    if (NULL == p_priv || gd_idx >= p_priv->gd_count || NULL == p_priv->p_gd ||
            NULL == p_priv->pp_gt)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_priv->pp_gt[gd_idx])
    {
        return PFS_BLKIO_RET_OK;
    }

    uint32_t gt_sector = p_priv->p_gd[gd_idx];
    if (0 == gt_sector)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint32_t n = p_priv->num_gtes_per_gt;
    uint64_t bytes = (uint64_t)n * 4ull;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t off = (int64_t)gt_sector * (int64_t)PFS_VMDK_SECTOR_SIZE;
    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < 0 || off < 0 || (uint64_t)off + bytes > (uint64_t)host_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    uint32_t *p_gt = (uint32_t *)pfs_malloc((size_t)n * sizeof(uint32_t));
    if (NULL == p_gt)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, off);
    if (0 > rc)
    {
        pfs_free(p_raw);
        pfs_free(p_gt);
        return rc;
    }
    else if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_free(p_gt);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < n; i++)
    {
        p_gt[i] = pfs_read_le32(p_raw + i * 4u);
    }
    pfs_free(p_raw);
    p_priv->pp_gt[gd_idx] = p_gt;
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_grain_lookup(pfs_vmdk_priv_t *p_priv, uint32_t grain_idx, uint32_t *p_sector)
{
    if (NULL == p_priv || NULL == p_sector)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_gd || grain_idx >= p_priv->grain_count)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint32_t gd_idx = grain_idx / p_priv->num_gtes_per_gt;
    uint32_t gt_idx = grain_idx % p_priv->num_gtes_per_gt;
    if (gd_idx >= p_priv->gd_count)
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (0 == p_priv->p_gd[gd_idx])
    {
        *p_sector = PFS_VMDK_GTE_UNALLOC;
        return PFS_BLKIO_RET_OK;
    }

    int rc = pfs_vmdk_grain_load_gt(p_priv, gd_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    if (NULL == p_priv->pp_gt[gd_idx])
    {
        return PFS_BLKIO_RET_STATE;
    }

    *p_sector = p_priv->pp_gt[gd_idx][gt_idx];
    return PFS_BLKIO_RET_OK;
}

static int pfs_vmdk_grain_write_gde(pfs_vmdk_priv_t *p_priv, uint32_t gd_idx, uint32_t gt_sector)
{
    p_priv->p_gd[gd_idx] = gt_sector;
    uint8_t le[4];
    pfs_write_le32(le, gt_sector);

    int64_t off = (int64_t)p_priv->gd_offset_sectors * (int64_t)PFS_VMDK_SECTOR_SIZE +
            (int64_t)gd_idx * 4;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, le, 4, off);
    if (0 > rc)
    {
        return rc;
    }
    else if (4 != rc)
    {
        return PFS_BLKIO_RET_IO;
    }

    if (0 != p_priv->rgd_offset_sectors)
    {
        off = (int64_t)p_priv->rgd_offset_sectors * (int64_t)PFS_VMDK_SECTOR_SIZE +
                (int64_t)gd_idx * 4;
        rc = pfs_blkio_raw_pwrite(&p_priv->host, le, 4, off);
        if (0 > rc)
        {
            return rc;
        }
        else if (4 != rc)
        {
            return PFS_BLKIO_RET_IO;
        }
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_vmdk_grain_write_gte(pfs_vmdk_priv_t *p_priv, uint32_t gd_idx, uint32_t gt_idx,
        uint32_t grain_sector)
{
    if (NULL == p_priv->pp_gt[gd_idx])
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->pp_gt[gd_idx][gt_idx] = grain_sector;
    uint32_t gt_sector = p_priv->p_gd[gd_idx];
    if (0 == gt_sector)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t le[4];
    pfs_write_le32(le, grain_sector);
    int64_t off = (int64_t)gt_sector * (int64_t)PFS_VMDK_SECTOR_SIZE + (int64_t)gt_idx * 4;
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

static int pfs_vmdk_grain_append_zero(pfs_vmdk_priv_t *p_priv, uint64_t bytes, uint32_t *p_sector)
{
    if (NULL == p_priv || NULL == p_sector || 0 == bytes)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < 0)
    {
        return PFS_BLKIO_RET_STATE;
    }
    // 对齐到扇区
    int64_t off = host_size;
    if (0 != ((uint64_t)off % (uint64_t)PFS_VMDK_SECTOR_SIZE))
    {
        off = (int64_t)(((uint64_t)off + (uint64_t)PFS_VMDK_SECTOR_SIZE - 1ull) &
                ~((uint64_t)PFS_VMDK_SECTOR_SIZE - 1ull));
    }
    if ((uint64_t)off / (uint64_t)PFS_VMDK_SECTOR_SIZE > (uint64_t)UINT32_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_sector = (uint32_t)((uint64_t)off / (uint64_t)PFS_VMDK_SECTOR_SIZE);

    uint32_t chunk = 64 * 1024;
    if ((uint64_t)chunk > bytes)
    {
        chunk = (uint32_t)bytes;
    }
    uint8_t *p_zero = (uint8_t *)pfs_mallocz(chunk);
    if (NULL == p_zero)
    {
        return PFS_BLKIO_RET_IO;
    }

    uint64_t left = bytes;
    while (left > 0)
    {
        uint32_t n = chunk;
        if ((uint64_t)n > left)
        {
            n = (uint32_t)left;
        }
        int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_zero, (int)n, off);
        if (0 > rc)
        {
            pfs_free(p_zero);
            return rc;
        }
        else if (rc != (int)n)
        {
            pfs_free(p_zero);
            return PFS_BLKIO_RET_IO;
        }
        off += (int64_t)n;
        left -= (uint64_t)n;
    }
    pfs_free(p_zero);
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_grain_ensure(pfs_vmdk_priv_t *p_priv, uint32_t grain_idx, uint32_t *p_sector)
{
    if (NULL == p_priv || NULL == p_sector)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL == p_priv->p_gd || grain_idx >= p_priv->grain_count)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint32_t gd_idx = grain_idx / p_priv->num_gtes_per_gt;
    uint32_t gt_idx = grain_idx % p_priv->num_gtes_per_gt;

    uint32_t cur;
    int rc = pfs_vmdk_grain_lookup(p_priv, grain_idx, &cur);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    if (cur != PFS_VMDK_GTE_UNALLOC && cur != PFS_VMDK_GTE_ZEROED)
    {
        *p_sector = cur;
        return PFS_BLKIO_RET_OK;
    }

    // 需要 GT
    if (0 == p_priv->p_gd[gd_idx])
    {
        uint64_t gt_bytes = (uint64_t)p_priv->num_gtes_per_gt * 4ull;
        if (0 != (gt_bytes % (uint64_t)PFS_VMDK_SECTOR_SIZE))
        {
            gt_bytes = (gt_bytes + (uint64_t)PFS_VMDK_SECTOR_SIZE - 1ull) &
                    ~((uint64_t)PFS_VMDK_SECTOR_SIZE - 1ull);
        }
        uint32_t gt_sector;
        rc = pfs_vmdk_grain_append_zero(p_priv, gt_bytes, &gt_sector);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        uint32_t *p_gt = (uint32_t *)pfs_mallocz((size_t)p_priv->num_gtes_per_gt * sizeof(uint32_t));
        if (NULL == p_gt)
        {
            return PFS_BLKIO_RET_IO;
        }
        if (NULL != p_priv->pp_gt[gd_idx])
        {
            pfs_free(p_priv->pp_gt[gd_idx]);
        }
        p_priv->pp_gt[gd_idx] = p_gt;
        rc = pfs_vmdk_grain_write_gde(p_priv, gd_idx, gt_sector);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }
    else
    {
        rc = pfs_vmdk_grain_load_gt(p_priv, gd_idx);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }

    uint64_t grain_bytes = p_priv->grain_size_sectors * (uint64_t)PFS_VMDK_SECTOR_SIZE;
    uint32_t grain_sector;
    rc = pfs_vmdk_grain_append_zero(p_priv, grain_bytes, &grain_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_vmdk_grain_write_gte(p_priv, gd_idx, gt_idx, grain_sector);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    *p_sector = grain_sector;
    return PFS_BLKIO_RET_OK;
}

// end
