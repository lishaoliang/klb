// Doc Encode : UTF-8 BOM, Unix(LF)
// VMDK 后端: monolithicSparse 读写 + 描述符 FLAT/ZERO; 宿主经 raw
#include "blkio_vmdk/pfs_vmdk.h"
#include "blkio_vmdk/pfs_vmdk_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define PFS_VMDK_PRIV(p) ((pfs_vmdk_priv_t *)((p)->priv))

static bool pfs_vmdk_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static int pfs_vmdk_sector_size_norm(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return PFS_VMDK_SECTOR_SIZE;
}

static pfs_vmdk_priv_t *pfs_vmdk_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_VMDK_PRIV(p_blkio);
}

static bool pfs_vmdk_io_ready(const pfs_blkio_t *p_blkio)
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
    pfs_vmdk_priv_t *p_priv = pfs_vmdk_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return false;
    }
    if (PFS_VMDK_MODE_SPARSE == p_priv->mode)
    {
        return pfs_blkio_is_open(&p_priv->host);
    }
    else if (PFS_VMDK_MODE_FLAT == p_priv->mode)
    {
        return (NULL != p_priv->p_extents && p_priv->extent_count > 0);
    }
    return false;
}

static void pfs_vmdk_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_vmdk_priv_t *p_priv = PFS_VMDK_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL == p_priv)
    {
        return;
    }
    if (PFS_VMDK_MODE_SPARSE == p_priv->mode)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
    else if (PFS_VMDK_MODE_FLAT == p_priv->mode && NULL != p_priv->p_extents)
    {
        for (int i = 0; i < p_priv->extent_count; i++)
        {
            if (p_priv->p_extents[i].host_open && 0 != p_priv->p_extents[i].host.last_error)
            {
                p_blkio->last_error = p_priv->p_extents[i].host.last_error;
                break;
            }
        }
    }
}

static void pfs_vmdk_open_fail(pfs_vmdk_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_vmdk_grain_free(p_priv);
    pfs_vmdk_flat_close_extents(p_priv);
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_raw_close(&p_priv->host);
    }
    pfs_free(p_priv);
}

static int pfs_vmdk_open_sparse(pfs_vmdk_priv_t *p_priv)
{
    pfs_vmdk_sparse_header_t hdr;

    int rc = pfs_vmdk_probe_sparse_header(&p_priv->host, &hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        // 已识别 sparse magic 但元数据不支持/损坏: 勿回退 raw
        if (PFS_BLKIO_RET_PROBE == rc)
        {
            return PFS_BLKIO_RET_STATE;
        }
        return rc;
    }
    return pfs_vmdk_sparse_open(p_priv, &hdr);
}

static int pfs_vmdk_open_descriptor(pfs_vmdk_priv_t *p_priv, const char *path)
{
    pfs_vmdk_desc_t desc;
    int64_t host_size;
    uint64_t load_len;

    int rc = pfs_vmdk_path_dirname(path, p_priv->base_dir, sizeof(p_priv->base_dir));
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size <= 0)
    {
        return PFS_BLKIO_RET_STATE;
    }
    load_len = (uint64_t)host_size;
    if (load_len > (uint64_t)PFS_VMDK_DESC_MAX)
    {
        load_len = (uint64_t)PFS_VMDK_DESC_MAX;
    }

    rc = pfs_vmdk_desc_load(&p_priv->host, 0, load_len, &desc);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    // 描述符文件本身可关闭; extent 另开
    (void)pfs_blkio_raw_close(&p_priv->host);

    rc = pfs_vmdk_flat_open_from_desc(p_priv, &desc, p_priv->open_flags, p_priv->sector_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    pfs_vmdk_priv_t *p_priv;
    pfs_vmdk_sparse_header_t hdr;
    int kind;
    int ss;

    if (NULL == p_blkio || NULL == path || !pfs_vmdk_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    ss = pfs_vmdk_sector_size_norm(sector_size);
    p_priv = (pfs_vmdk_priv_t *)pfs_mallocz(sizeof(*p_priv));
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->open_flags = flags;
    p_priv->sector_size = ss;

    (void)pfs_blkio_init(&p_priv->host);
    p_priv->host.ops = &g_pfs_blkio_raw_ops;
    int rc = pfs_blkio_raw_open(&p_priv->host, path, flags, ss);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_free(p_priv);
        return rc;
    }

    rc = pfs_vmdk_probe(&p_priv->host, &kind, &hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_vmdk_open_fail(p_priv);
        return rc;
    }

    if (PFS_VMDK_KIND_SPARSE == kind)
    {
        rc = pfs_vmdk_open_sparse(p_priv);
    }
    else if (PFS_VMDK_KIND_DESCRIPTOR == kind)
    {
        rc = pfs_vmdk_open_descriptor(p_priv, path);
    }
    else
    {
        rc = PFS_BLKIO_RET_PROBE;
    }

    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_vmdk_open_fail(p_priv);
        return rc;
    }

    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_vmdk_close(pfs_blkio_t *p_blkio)
{
    pfs_vmdk_priv_t *p_priv;
    int rc = PFS_BLKIO_RET_OK;

    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_priv = pfs_vmdk_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    pfs_vmdk_grain_free(p_priv);
    if (PFS_VMDK_MODE_FLAT == p_priv->mode)
    {
        pfs_vmdk_flat_close_extents(p_priv);
    }
    if (pfs_blkio_is_open(&p_priv->host))
    {
        rc = pfs_blkio_raw_close(&p_priv->host);
        if (PFS_BLKIO_RET_OK != rc)
        {
            p_blkio->status |= PFS_BLKIO_ERR;
            p_blkio->last_error = p_priv->host.last_error;
        }
    }

    pfs_free(p_priv);
    p_blkio->priv = NULL;
    p_blkio->status = PFS_BLKIO_CLOSED;
    p_blkio->align_size = 0;
    if (PFS_BLKIO_RET_OK == rc)
    {
        p_blkio->last_error = 0;
    }
    return rc;
}

int pfs_vmdk_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    pfs_vmdk_priv_t *p_priv;

    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv = PFS_VMDK_PRIV(p_blkio);
    int rc;
    if (PFS_VMDK_MODE_SPARSE == p_priv->mode)
    {
        rc = pfs_vmdk_sparse_pread(p_priv, buf, size, offset);
    }
    else if (PFS_VMDK_MODE_FLAT == p_priv->mode)
    {
        rc = pfs_vmdk_flat_pread(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vmdk_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
    }
    return rc;
}

int pfs_vmdk_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    pfs_vmdk_priv_t *p_priv;

    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv = PFS_VMDK_PRIV(p_blkio);
    int rc;
    if (PFS_VMDK_MODE_SPARSE == p_priv->mode)
    {
        rc = pfs_vmdk_sparse_pwrite(p_priv, buf, size, offset);
    }
    else if (PFS_VMDK_MODE_FLAT == p_priv->mode)
    {
        rc = pfs_vmdk_flat_pwrite(p_priv, buf, size, offset);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vmdk_fail_from_host(p_blkio, PFS_BLKIO_ERR_WRITE);
    }
    return rc;
}

int pfs_vmdk_fsync(pfs_blkio_t *p_blkio)
{
    pfs_vmdk_priv_t *p_priv;

    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_vmdk_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv = PFS_VMDK_PRIV(p_blkio);
    int rc;
    if (PFS_VMDK_MODE_SPARSE == p_priv->mode)
    {
        rc = pfs_blkio_raw_fsync(&p_priv->host);
    }
    else if (PFS_VMDK_MODE_FLAT == p_priv->mode)
    {
        rc = pfs_vmdk_flat_fsync(p_priv);
    }
    else
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (rc < 0 && PFS_BLKIO_RET_PARAM != rc && PFS_BLKIO_RET_STATE != rc)
    {
        pfs_vmdk_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
    }
    return rc;
}

int64_t pfs_vmdk_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_vmdk_priv_t *p_priv;

    p_priv = pfs_vmdk_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    if (p_priv->capacity_bytes > (uint64_t)INT64_MAX)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return (int64_t)p_priv->capacity_bytes;
}

int pfs_vmdk_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == pfs_vmdk_priv_or_null(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_VMDK_SECTOR_SIZE;
}

const pfs_blkio_ops_t g_pfs_vmdk_ops = {
    "vmdk",
    PFS_BLKIO_VMDK,
    pfs_vmdk_open,
    NULL,
    pfs_vmdk_close,
    pfs_vmdk_pread,
    pfs_vmdk_pwrite,
    pfs_vmdk_fsync,
    pfs_vmdk_get_size,
    pfs_vmdk_logical_sector_size,
};

// end
