// Doc Encode : UTF-8 BOM, Unix(LF)
// VDI 后端: DYNAMIC / FIXED 标准块读写; 宿主经 raw
#include "blkio_vdi/pfs_vdi.h"
#include "blkio_vdi/pfs_vdi_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <string.h>
#include <stdbool.h>

#define PFS_VDI_PRIV(p) ((pfs_vdi_priv_t *)((p)->priv))

static bool pfs_vdi_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static int pfs_vdi_sector_size_norm(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return PFS_VDI_SECTOR_SIZE;
}

static pfs_vdi_priv_t *pfs_vdi_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_VDI_PRIV(p_blkio);
}

static bool pfs_vdi_io_ready(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return false;
    }
    if (0 != (p_blkio->status & (PFS_BLKIO_NOT_OPEN | PFS_BLKIO_CLOSED |
            PFS_BLKIO_ERR | PFS_BLKIO_ERR_READ | PFS_BLKIO_ERR_WRITE | PFS_BLKIO_ERR_FSYNC)))
    {
        return false;
    }

    pfs_vdi_priv_t *p_priv = pfs_vdi_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return false;
    }
    return pfs_blkio_is_open(&p_priv->host);
}

static void pfs_vdi_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_vdi_priv_t *p_priv = PFS_VDI_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL != p_priv)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
}

static void pfs_vdi_open_fail(pfs_vdi_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_vdi_map_free(p_priv);
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_raw_close(&p_priv->host);
    }
    pfs_free(p_priv);
}

int pfs_vdi_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == path || !pfs_vdi_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    bool want_write = (PFS_BLKIO_O_RDONLY != flags);
    int ss = pfs_vdi_sector_size_norm(sector_size);
    pfs_vdi_priv_t *p_priv = (pfs_vdi_priv_t *)pfs_mallocz(sizeof(*p_priv));
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

    pfs_vdi_header_t hdr;
    rc = pfs_vdi_probe_header(&p_priv->host, &hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_vdi_open_fail(p_priv);
        return rc;
    }

    p_priv->image_type = hdr.image_type;
    p_priv->disk_size = hdr.disk_size;
    p_priv->block_size = hdr.block_size;
    p_priv->offset_blocks = hdr.offset_blocks;
    p_priv->offset_data = hdr.offset_data;
    p_priv->blocks_in_hdd = hdr.blocks_in_hdd;
    p_priv->blocks_allocated = hdr.blocks_allocated;
    p_priv->want_write = want_write ? 1 : 0;

    rc = pfs_vdi_map_load(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_vdi_open_fail(p_priv);
        return rc;
    }

    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vdi_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_vdi_priv_t *p_priv = pfs_vdi_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    pfs_vdi_map_free(p_priv);
    int rc = pfs_blkio_raw_close(&p_priv->host);
    if (PFS_BLKIO_RET_OK != rc)
    {
        p_blkio->status |= PFS_BLKIO_ERR;
        p_blkio->last_error = p_priv->host.last_error;
        pfs_free(p_priv);
        p_blkio->priv = NULL;
        return rc;
    }

    pfs_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->align_size = 0;
    p_blkio->last_error = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vdi_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }
    if (!pfs_vdi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vdi_priv_t *p_priv = PFS_VDI_PRIV(p_blkio);
    int rc = pfs_vdi_io_pread(p_priv, buf, size, offset);
    if (rc < 0)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_vdi_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
        }
        return rc;
    }
    return rc;
}

int pfs_vdi_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return 0;
    }
    if (!pfs_vdi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vdi_priv_t *p_priv = PFS_VDI_PRIV(p_blkio);
    int rc = pfs_vdi_io_pwrite(p_priv, buf, size, offset);
    if (rc < 0)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_vdi_fail_from_host(p_blkio, PFS_BLKIO_ERR_WRITE);
        }
        return rc;
    }
    return rc;
}

int pfs_vdi_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vdi_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_vdi_priv_t *p_priv = PFS_VDI_PRIV(p_blkio);
    int rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (PFS_BLKIO_RET_OK != rc)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_vdi_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
        }
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

int64_t pfs_vdi_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_vdi_priv_t *p_priv = pfs_vdi_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    if (!pfs_vdi_io_ready(p_blkio))
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return (int64_t)p_priv->disk_size;
}

int pfs_vdi_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_VDI_SECTOR_SIZE;
}

const pfs_blkio_ops_t g_pfs_vdi_ops = {
    "vdi",
    PFS_BLKIO_VDI,
    pfs_vdi_open,
    NULL,
    pfs_vdi_close,
    pfs_vdi_pread,
    pfs_vdi_pwrite,
    pfs_vdi_fsync,
    pfs_vdi_get_size,
    pfs_vdi_logical_sector_size,
};

// end
