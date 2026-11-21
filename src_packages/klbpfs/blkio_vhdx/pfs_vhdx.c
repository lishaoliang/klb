// Doc Encode : UTF-8 BOM, Unix(LF)
// VHDX 后端门面: Fixed/Dynamic 读写; 宿主经 raw; BAT 更新经最小 log
#include "blkio_vhdx/pfs_vhdx.h"
#include "blkio_vhdx/pfs_vhdx_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdbool.h>

#define PFS_VHDX_PRIV(p) ((pfs_vhdx_priv_t *)((p)->priv))

static bool pfs_vhdx_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static pfs_vhdx_priv_t *pfs_vhdx_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_VHDX_PRIV(p_blkio);
}

static int pfs_vhdx_io_ready(const pfs_blkio_t *p_blkio)
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

    pfs_vhdx_priv_t *p_priv = pfs_vhdx_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return 0;
    }
    return pfs_blkio_is_open(&p_priv->host);
}

static void pfs_vhdx_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_vhdx_priv_t *p_priv = PFS_VHDX_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL != p_priv)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
}

static void pfs_vhdx_open_fail(pfs_vhdx_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_vhdx_bat_free(p_priv);
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_raw_close(&p_priv->host);
    }
    pfs_free(p_priv);
}

int pfs_vhdx_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == path || !pfs_vhdx_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    (void)sector_size;
    int ss = PFS_BLKIO_ALIGN_512;

    pfs_vhdx_priv_t *p_priv = (pfs_vhdx_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_IO;
    }

    (void)pfs_blkio_init(&p_priv->host);
    p_priv->host.ops = &g_pfs_blkio_raw_ops;
    p_priv->open_flags = flags;
    int rc = pfs_blkio_raw_open(&p_priv->host, path, flags, ss);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_priv);
        return rc;
    }

    rc = pfs_vhdx_probe_open(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        // PROBE=非 VHDX 可回退; 已识别失败用 STATE (probe_open 已区分)
        pfs_vhdx_open_fail(p_priv);
        return rc;
    }

    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vhdx_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_vhdx_priv_t *p_priv = pfs_vhdx_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    pfs_vhdx_bat_free(p_priv);
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

int pfs_vhdx_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhdx_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhdx_priv_t *p_priv = PFS_VHDX_PRIV(p_blkio);
    int rc;
    if (PFS_VHDX_DISK_FIXED == p_priv->disk_type)
    {
        rc = pfs_vhdx_fixed_pread(p_priv, buf, size, offset);
    }
    else if (PFS_VHDX_DISK_DYNAMIC == p_priv->disk_type)
    {
        rc = pfs_vhdx_dynamic_pread(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (0 > rc && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vhdx_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
    }
    return rc;
}

int pfs_vhdx_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhdx_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhdx_priv_t *p_priv = PFS_VHDX_PRIV(p_blkio);
    int rc;
    if (PFS_VHDX_DISK_FIXED == p_priv->disk_type)
    {
        rc = pfs_vhdx_fixed_pwrite(p_priv, buf, size, offset);
    }
    else if (PFS_VHDX_DISK_DYNAMIC == p_priv->disk_type)
    {
        rc = pfs_vhdx_dynamic_pwrite(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }
    return rc;
}

int pfs_vhdx_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vhdx_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vhdx_priv_t *p_priv = PFS_VHDX_PRIV(p_blkio);
    int rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (0 > rc && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vhdx_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
    }
    return rc;
}

int64_t pfs_vhdx_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_vhdx_priv_t *p_priv = pfs_vhdx_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    if (p_priv->virtual_disk_size > (uint64_t)INT64_MAX)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return (int64_t)p_priv->virtual_disk_size;
}

int pfs_vhdx_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    pfs_vhdx_priv_t *p_priv = pfs_vhdx_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return (int)p_priv->logical_sector_size;
}

const pfs_blkio_ops_t g_pfs_vhdx_ops = {
    "vhdx",
    PFS_BLKIO_VHDX,
    pfs_vhdx_open,
    NULL,
    pfs_vhdx_close,
    pfs_vhdx_pread,
    pfs_vhdx_pwrite,
    pfs_vhdx_fsync,
    pfs_vhdx_get_size,
    pfs_vhdx_logical_sector_size,
};

// end
