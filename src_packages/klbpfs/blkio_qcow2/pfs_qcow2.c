// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 后端: 标准 cluster 读写 + 写时分配; 宿主经 raw
#include "blkio_qcow2/pfs_qcow2.h"
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "base/pfs_mem.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define PFS_QCOW2_PRIV(p) ((pfs_qcow2_priv_t *)((p)->priv))

static bool pfs_qcow2_flags_valid(pfs_blkio_oflags_e flags)
{
    return (PFS_BLKIO_O_RDONLY == flags || PFS_BLKIO_O_WRONLY == flags || PFS_BLKIO_O_RDWR == flags);
}

static int pfs_qcow2_sector_size_norm(int sector_size)
{
    if (PFS_BLKIO_ALIGN_512 == sector_size || PFS_BLKIO_ALIGN_1K == sector_size ||
            PFS_BLKIO_ALIGN_2K == sector_size || PFS_BLKIO_ALIGN_4K == sector_size)
    {
        return sector_size;
    }
    return PFS_QCOW2_SECTOR_SIZE;
}

static pfs_qcow2_priv_t *pfs_qcow2_priv_or_null(const pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio || NULL == p_blkio->priv)
    {
        return NULL;
    }
    return PFS_QCOW2_PRIV(p_blkio);
}

static bool pfs_qcow2_io_ready(const pfs_blkio_t *p_blkio)
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
    pfs_qcow2_priv_t *p_priv = pfs_qcow2_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return false;
    }
    return pfs_blkio_is_open(&p_priv->host);
}

static void pfs_qcow2_fail_from_host(pfs_blkio_t *p_blkio, int err_bit)
{
    pfs_qcow2_priv_t *p_priv = PFS_QCOW2_PRIV(p_blkio);

    p_blkio->status |= err_bit;
    if (NULL != p_priv)
    {
        p_blkio->last_error = p_priv->host.last_error;
    }
}

static void pfs_qcow2_open_fail(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    pfs_qcow2_l1_free(p_priv);
    pfs_qcow2_refcount_free(p_priv);
    if (NULL != p_priv->p_l2_cache)
    {
        pfs_free(p_priv->p_l2_cache);
        p_priv->p_l2_cache = NULL;
    }
    if (pfs_blkio_is_open(&p_priv->host))
    {
        (void)pfs_blkio_raw_close(&p_priv->host);
    }
    pfs_free(p_priv);
}

static bool pfs_qcow2_header_supported(const pfs_qcow2_header_t *p_hdr, bool want_write)
{
    if (NULL == p_hdr)
    {
        return false;
    }
    if (p_hdr->cluster_bits < PFS_QCOW2_CLUSTER_BITS_MIN ||
            p_hdr->cluster_bits > PFS_QCOW2_CLUSTER_BITS_MAX)
    {
        return false;
    }
    if (PFS_QCOW2_CRYPT_NONE != p_hdr->crypt_method)
    {
        return false;
    }
    if (0 != p_hdr->backing_file_offset || 0 != p_hdr->backing_file_size)
    {
        return false;
    }
    if (0 == p_hdr->l1_size || 0 == p_hdr->l1_table_offset)
    {
        return false;
    }
    if (0 == p_hdr->refcount_table_clusters || 0 == p_hdr->refcount_table_offset)
    {
        return false;
    }
    if (0 != (p_hdr->l1_table_offset & ((1ull << p_hdr->cluster_bits) - 1ull)))
    {
        return false;
    }
    if (0 != (p_hdr->refcount_table_offset & ((1ull << p_hdr->cluster_bits) - 1ull)))
    {
        return false;
    }

    uint64_t incompat_mask = PFS_QCOW2_INCOMPAT_DIRTY | PFS_QCOW2_INCOMPAT_CORRUPT |
            PFS_QCOW2_INCOMPAT_EXTDATA | PFS_QCOW2_INCOMPAT_COMPRESS |
            PFS_QCOW2_INCOMPAT_EXTL2;
    if (0 != (p_hdr->incompatible_features & incompat_mask))
    {
        return false;
    }
    // 未知 incompatible 位也拒绝
    if (0 != (p_hdr->incompatible_features & ~incompat_mask))
    {
        return false;
    }

    if (want_write && p_hdr->nb_snapshots > 0u)
    {
        return false;
    }
    return true;
}

int pfs_qcow2_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size)
{
    if (NULL == p_blkio || NULL == path || !pfs_qcow2_flags_valid(flags))
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (NULL != p_blkio->priv)
    {
        return PFS_BLKIO_RET_STATE;
    }

    bool want_write = (PFS_BLKIO_O_RDONLY != flags);
    int ss = pfs_qcow2_sector_size_norm(sector_size);
    pfs_qcow2_priv_t *p_priv = (pfs_qcow2_priv_t *)pfs_mallocz(sizeof(*p_priv));
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

    pfs_qcow2_header_t hdr;
    rc = pfs_qcow2_probe_header(&p_priv->host, &hdr);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_qcow2_open_fail(p_priv);
        return rc;
    }

    if (!pfs_qcow2_header_supported(&hdr, want_write))
    {
        pfs_qcow2_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    int64_t host_size = pfs_blkio_raw_get_size(&p_priv->host);
    if (host_size < (int64_t)PFS_QCOW2_HEADER_SIZE_MIN)
    {
        pfs_qcow2_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    uint32_t cluster_size = 1u << hdr.cluster_bits;
    p_priv->version = hdr.version;
    p_priv->virtual_size = hdr.size;
    p_priv->cluster_bits = hdr.cluster_bits;
    p_priv->cluster_size = cluster_size;
    p_priv->l1_size = hdr.l1_size;
    p_priv->l1_table_offset = hdr.l1_table_offset;
    p_priv->refcount_table_offset = hdr.refcount_table_offset;
    p_priv->refcount_table_clusters = hdr.refcount_table_clusters;
    p_priv->refcount_order = hdr.refcount_order;
    p_priv->refcount_bits = 1u << hdr.refcount_order;
    p_priv->l2_entries = cluster_size / 8u;
    p_priv->refcount_block_entries = (uint32_t)(((uint64_t)cluster_size * 8ull) /
            (uint64_t)p_priv->refcount_bits);
    p_priv->host_file_size = host_size;
    p_priv->allow_write = want_write ? 1 : 0;

    if (0 == p_priv->l2_entries || 0 == p_priv->refcount_block_entries)
    {
        pfs_qcow2_open_fail(p_priv);
        return PFS_BLKIO_RET_STATE;
    }

    rc = pfs_qcow2_reftable_load(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_qcow2_open_fail(p_priv);
        return rc;
    }

    rc = pfs_qcow2_l1_load(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        pfs_qcow2_open_fail(p_priv);
        return rc;
    }

    p_blkio->priv = p_priv;
    p_blkio->status = 0;
    p_blkio->last_error = 0;
    p_blkio->align_size = 0;
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_close(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    pfs_qcow2_priv_t *p_priv = pfs_qcow2_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        p_blkio->status = PFS_BLKIO_CLOSED;
        p_blkio->align_size = 0;
        return PFS_BLKIO_RET_OK;
    }

    pfs_qcow2_l1_free(p_priv);
    pfs_qcow2_refcount_free(p_priv);
    if (NULL != p_priv->p_l2_cache)
    {
        pfs_free(p_priv->p_l2_cache);
        p_priv->p_l2_cache = NULL;
    }

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

int pfs_qcow2_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_qcow2_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_qcow2_priv_t *p_priv = PFS_QCOW2_PRIV(p_blkio);
    int rc = pfs_qcow2_cluster_pread(p_priv, buf, size, offset);
    if (rc < 0)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_qcow2_fail_from_host(p_blkio, PFS_BLKIO_ERR_READ);
        }
        return rc;
    }
    return rc;
}

int pfs_qcow2_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset)
{
    if (NULL == p_blkio || NULL == buf)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }
    if (!pfs_qcow2_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_qcow2_priv_t *p_priv = PFS_QCOW2_PRIV(p_blkio);
    if (0 == p_priv->allow_write)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int rc = pfs_qcow2_cluster_pwrite(p_priv, buf, size, offset);
    if (rc < 0)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_qcow2_fail_from_host(p_blkio, PFS_BLKIO_ERR_WRITE);
        }
        return rc;
    }
    return rc;
}

int pfs_qcow2_fsync(pfs_blkio_t *p_blkio)
{
    if (NULL == p_blkio)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (!pfs_qcow2_io_ready(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_qcow2_priv_t *p_priv = PFS_QCOW2_PRIV(p_blkio);
    int rc = pfs_blkio_raw_fsync(&p_priv->host);
    if (PFS_BLKIO_RET_OK != rc)
    {
        if (PFS_BLKIO_RET_IO == rc)
        {
            pfs_qcow2_fail_from_host(p_blkio, PFS_BLKIO_ERR_FSYNC);
        }
        return rc;
    }
    return PFS_BLKIO_RET_OK;
}

int64_t pfs_qcow2_get_size(const pfs_blkio_t *p_blkio)
{
    pfs_qcow2_priv_t *p_priv = pfs_qcow2_priv_or_null(p_blkio);
    if (NULL == p_priv)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    if (p_priv->virtual_size > (uint64_t)INT64_MAX)
    {
        return (int64_t)PFS_BLKIO_RET_STATE;
    }
    return (int64_t)p_priv->virtual_size;
}

int pfs_qcow2_logical_sector_size(const pfs_blkio_t *p_blkio)
{
    if (NULL == pfs_qcow2_priv_or_null(p_blkio))
    {
        return PFS_BLKIO_RET_STATE;
    }
    return PFS_QCOW2_SECTOR_SIZE;
}

const pfs_blkio_ops_t g_pfs_qcow2_ops = {
    "qcow2",
    PFS_BLKIO_QCOW2,
    pfs_qcow2_open,
    NULL,                       // open_direct: 镜像后端不提供
    pfs_qcow2_close,
    pfs_qcow2_pread,
    pfs_qcow2_pwrite,
    pfs_qcow2_fsync,
    pfs_qcow2_get_size,
    pfs_qcow2_logical_sector_size,
};

// end
