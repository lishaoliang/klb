// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 L1 表: 加载/释放/写回
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_qcow2_l1_free(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    if (NULL != p_priv->p_l1)
    {
        pfs_free(p_priv->p_l1);
        p_priv->p_l1 = NULL;
    }
}

int pfs_qcow2_l1_load(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->l1_size || 0 == p_priv->l1_table_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_qcow2_l1_free(p_priv);

    uint64_t bytes = (uint64_t)p_priv->l1_size * 8ull;
    if (0 == bytes || bytes > (uint64_t)PFS_QCOW2_L1_SIZE_MAX_BYTES ||
            bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_priv->l1_table_offset + bytes < p_priv->l1_table_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if ((int64_t)(p_priv->l1_table_offset + bytes) > p_priv->host_file_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_l1 = (uint64_t *)pfs_malloc((size_t)p_priv->l1_size * sizeof(uint64_t));
    if (NULL == p_priv->p_l1)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->l1_table_offset);
    if (rc < 0)
    {
        pfs_free(p_raw);
        pfs_qcow2_l1_free(p_priv);
        return rc;
    }
    if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_qcow2_l1_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < p_priv->l1_size; i++)
    {
        p_priv->p_l1[i] = pfs_read_be64(p_raw + i * 8u);
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_l1_set(pfs_qcow2_priv_t *p_priv, uint32_t idx, uint64_t entry)
{
    if (NULL == p_priv || NULL == p_priv->p_l1 || idx >= p_priv->l1_size)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    p_priv->p_l1[idx] = entry;
    uint8_t be[8];
    pfs_write_be64(be, entry);
    int64_t off = (int64_t)p_priv->l1_table_offset + (int64_t)idx * 8;
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, be, 8, off);
    if (rc < 0)
    {
        return rc;
    }
    if (rc != 8)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

// end
