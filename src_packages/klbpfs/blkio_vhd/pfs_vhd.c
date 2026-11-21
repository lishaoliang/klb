// Doc Encode : UTF-8 BOM, Unix(LF)
// VHD 后端: Fixed + Dynamic; 宿主经 raw; facade 探测序 vhd -> raw
#include "blkio_vhd/pfs_vhd.h"
#include "blkio_vhd/pfs_vhd_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdbool.h>

#define PFS_VHD_PRIV(p) ((pfs_vhd_priv_t *)((p)->priv))

static bool pfs_vhd_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static int pfs_vhd_sector_size_norm(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return PFS_VHD_SECTOR_SIZE;
}

static pfs_vhd_priv_t *pfs_vhd_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_VHD_PRIV(p_blkio);
}

static int pfs_vhd_io_ready(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return 0;
    }
    if (0 != (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED |
            PFS_BLKIO_ERR | PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)))
    {
        return 0;
    }

    pfs_vhd_priv_t *p_priv = pfs_vhd_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return 0;
    }
    return pfs_blkio_is_open(&p_priv->host);
}

static void pfs_vhd_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_vhd_priv_t *p_priv = PFS_VHD_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL != p_priv)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
}

static void pfs_vhd_open_fail(pfs_vhd_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_vhd_bat_free(p_priv);
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_raw_close(&p_priv->host);
    }
    pfs_free(p_priv);
}

int pfs_vhd_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == path || !pfs_vhd_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int ss = pfs_vhd_sector_size_norm(sector_size);
    pfs_vhd_priv_t *p_priv = (pfs_vhd_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_IO;
    }

    (void)pfs_blkio_init(&p_priv->host);
    p_priv->host.ops = &g_pfs_blkio_raw_ops;
    int rc = pfs_blkio_raw_open(&p_priv->host, path, flags, ss);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_priv);
        return rc;
    }

    pfs_vhd_footer_t footer;

    rc = pfs_vhd_probe_footer(&p_priv->host, &footer);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_vhd_open_fail(p_priv);
        return rc;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < (int64_t)PFS_VHD_FOOTER_SIZE)
    {
        pfs_vhd_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    if (PFS_VHD_DISK_TYPE_FIXED == footer.disk_type)
    {
        if ((uint64_t)host_size < footer.current_size + (uint64_t)PFS_VHD_FOOTER_SIZE)
        {
            pfs_vhd_open_fail(p_priv);
            return PFS_BLKIO_RET_STATE;
        }
        p_priv->disk_type = footer.disk_type;
        p_priv->current_size = footer.current_size;
        p_priv->table_offset = 0;
        p_priv->block_size = 0;
        p_priv->max_table_entries = 0;
        p_priv->bitmap_sectors = 0;
        p_priv->p_bat = NULL;
    }
    else if (PFS_VHD_DISK_TYPE_DYNAMIC == footer.disk_type)
    {
        pfs_vhd_dyn_header_t dyn;

        rc = pfs_vhd_probe_dyn_header(&p_priv->host, footer.data_offset, &dyn);
        if (PFS_BLKIO_RET_OK != rc)
        {
            pfs_vhd_open_fail(p_priv);
            // 已识别 Dynamic 但元数据坏, 勿回退 raw
            if (PFS_BLKIO_RET_PROBE == rc)
            {
                return PFS_BLKIO_RET_STATE;
            }
            return rc;
        }

        uint64_t need_blocks = (footer.current_size + (uint64_t)dyn.block_size - 1ull) / (uint64_t)dyn.block_size;

        if (need_blocks > (uint64_t)dyn.max_table_entries)
        {
            pfs_vhd_open_fail(p_priv);
            return PFS_BLKIO_RET_STATE;
        }

        p_priv->disk_type = footer.disk_type;
        p_priv->current_size = footer.current_size;
        p_priv->table_offset = dyn.table_offset;
        p_priv->block_size = dyn.block_size;
        p_priv->max_table_entries = dyn.max_table_entries;
        p_priv->bitmap_sectors = pfs_vhd_dyn_bitmap_sectors(dyn.block_size);
        if (0 == p_priv->bitmap_sectors)
        {
            pfs_vhd_open_fail(p_priv);
            return PFS_BLKIO_RET_STATE;
        }

        rc = pfs_vhd_bat_load(p_priv);
        if (PFS_BLKIO_RET_OK != rc)
        {
            pfs_vhd_open_fail(p_priv);
            return rc;
        }
    }
    else
    {
        // Differencing 尚未实现
        pfs_vhd_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vhd_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_vhd_priv_t *p_priv = pfs_vhd_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    // close 前刷盘: 动态盘 BAT/块常在页缓存
    (void)pfs_blkio_raw_fsync(&p_priv->host);

    pfs_vhd_bat_free(p_priv);
    int rc = pfs_blkio_raw_close(&p_priv->host);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->status |= PFS_BLKIO_ERR;
        p_blkio->last_error = p_priv->host.last_error;
        return rc;
    }

    pfs_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->align_size = 0;
    p_blkio->last_error = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vhd_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhd_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhd_priv_t *p_priv = PFS_VHD_PRIV(p_blkio);
    int rc;
    if (PFS_VHD_DISK_TYPE_FIXED == p_priv->disk_type)
    {
        rc = pfs_vhd_fixed_pread(p_priv, buf, size, offset);
    }
    else if (PFS_VHD_DISK_TYPE_DYNAMIC == p_priv->disk_type)
    {
        rc = pfs_vhd_dynamic_pread(p_priv, buf, size, offset);
    }
    else if (PFS_VHD_DISK_TYPE_DIFFERENCING == p_priv->disk_type)
    {
        rc = pfs_vhd_diff_pread(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vhd_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
    }
    return rc;
}

int pfs_vhd_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhd_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhd_priv_t *p_priv = PFS_VHD_PRIV(p_blkio);
    int rc;
    if (PFS_VHD_DISK_TYPE_FIXED == p_priv->disk_type)
    {
        rc = pfs_vhd_fixed_pwrite(p_priv, buf, size, offset);
    }
    else if (PFS_VHD_DISK_TYPE_DYNAMIC == p_priv->disk_type)
    {
        rc = pfs_vhd_dynamic_pwrite(p_priv, buf, size, offset);
    }
    else if (PFS_VHD_DISK_TYPE_DIFFERENCING == p_priv->disk_type)
    {
        rc = pfs_vhd_diff_pwrite(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vhd_fail_from_host(p_blkio, PFS_BLKIO_ERR_WRITE);
    }
    return rc;
}

int pfs_vhd_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhd_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhd_priv_t *p_priv = PFS_VHD_PRIV(p_blkio);
    int rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vhd_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
    }
    return rc;
}

int64_t pfs_vhd_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_vhd_priv_t *p_priv = pfs_vhd_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    if (p_priv->current_size > (uint64_t)INT64_MAX)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return (int64_t)p_priv->current_size;
}

int pfs_vhd_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == pfs_vhd_priv_or_null(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_VHD_SECTOR_SIZE;
}

const pfs_blkio_ops_t g_pfs_vhd_ops = {
    "vhd",
    PFS_BLKIO_VHD,
    pfs_vhd_open,
    NULL,                       // open_direct: 镜像后端不提供
    pfs_vhd_close,
    pfs_vhd_pread,
    pfs_vhd_pwrite,
    pfs_vhd_fsync,
    pfs_vhd_get_size,
    pfs_vhd_logical_sector_size,
};

// end
