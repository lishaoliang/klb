// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX BAT 加载
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

void pfs_vhdx_bat_free(pfs_vhdx_priv_t *p_priv)
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

int pfs_vhdx_bat_load(pfs_vhdx_priv_t *p_priv)
{
    if (NULL == p_priv || 0 == p_priv->bat_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_vhdx_bat_free(p_priv);

    uint64_t bytes = (uint64_t)p_priv->bat_entries * 8ull;
    if (bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (0 > host_size || p_priv->bat_offset > (uint64_t)host_size ||
            (uint64_t)host_size - p_priv->bat_offset < bytes)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_bat = (uint64_t *)pfs_malloc((size_t)p_priv->bat_entries * sizeof(uint64_t));
    if (NULL == p_priv->p_bat)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->bat_offset);
    if (0 > rc)
    {
        pfs_free(p_raw);
        pfs_vhdx_bat_free(p_priv);
        return rc;
    }
    else if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_vhdx_bat_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < p_priv->bat_entries; i++)
    {
        p_priv->p_bat[i] = pfs_read_le64(p_raw + i * 8u);
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

int pfs_vhdx_bat_set_entry(pfs_vhdx_priv_t *p_priv, uint32_t bat_idx, uint64_t entry)
{
    if (NULL == p_priv || NULL == p_priv->p_bat || bat_idx >= p_priv->bat_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_priv->p_bat[bat_idx] = entry;

    uint64_t sec_off = p_priv->bat_offset + ((uint64_t)bat_idx * 8ull / (uint64_t)PFS_VHDX_LOG_SECTOR_SIZE) *
            (uint64_t)PFS_VHDX_LOG_SECTOR_SIZE;
    uint32_t off_in_sec = (uint32_t)(((uint64_t)bat_idx * 8ull) % (uint64_t)PFS_VHDX_LOG_SECTOR_SIZE);
    uint8_t sec[PFS_VHDX_LOG_SECTOR_SIZE];

    int rc = pfs_blkio_raw_pread(&p_priv->host, sec, (int)PFS_VHDX_LOG_SECTOR_SIZE, (int64_t)sec_off);
    if (0 > rc)
    {
        return rc;
    }
    else if (rc != (int)PFS_VHDX_LOG_SECTOR_SIZE)
    {
        return PFS_BLKIO_RET_IO;
    }
    pfs_write_le64(sec + off_in_sec, entry);

    return pfs_vhdx_log_commit_4k(p_priv, sec_off, sec);
}

// end
