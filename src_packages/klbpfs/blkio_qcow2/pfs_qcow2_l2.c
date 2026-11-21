// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 L2 表: 单簇缓存; 分配/COW; 读写 entry
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_qcow2_l2_cache_invalidate(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    p_priv->l2_cache_offset = 0;
}

static int pfs_qcow2_l2_guest_index(const pfs_qcow2_priv_t *p_priv, uint64_t guest_off,
        uint32_t *p_l1_idx, uint32_t *p_l2_idx)
{
    if (NULL == p_priv || NULL == p_l1_idx || NULL == p_l2_idx || 0 == p_priv->l2_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint64_t clu = guest_off >> p_priv->cluster_bits;
    uint64_t l2 = clu % (uint64_t)p_priv->l2_entries;
    uint64_t l1 = clu / (uint64_t)p_priv->l2_entries;
    if (l1 >= (uint64_t)p_priv->l1_size)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_l1_idx = (uint32_t)l1;
    *p_l2_idx = (uint32_t)l2;
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_l2_cache_load(pfs_qcow2_priv_t *p_priv, uint64_t l2_offset)
{
    if (NULL == p_priv || 0 == l2_offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != (l2_offset & ((uint64_t)p_priv->cluster_size - 1ull)))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_priv->l2_cache_offset == l2_offset && NULL != p_priv->p_l2_cache)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint32_t bytes = p_priv->cluster_size;
    if (bytes > (uint32_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if ((int64_t)(l2_offset + bytes) > p_priv->host_file_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (NULL == p_priv->p_l2_cache)
    {
        p_priv->p_l2_cache = (uint64_t *)pfs_malloc((size_t)p_priv->l2_entries * sizeof(uint64_t));
        if (NULL == p_priv->p_l2_cache)
        {
            return PFS_BLKIO_RET_IO;
        }
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)l2_offset);
    if (rc < 0)
    {
        pfs_free(p_raw);
        return rc;
    }
    if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < p_priv->l2_entries; i++)
    {
        p_priv->p_l2_cache[i] = pfs_read_be64(p_raw + i * 8u);
    }
    pfs_free(p_raw);
    p_priv->l2_cache_offset = l2_offset;
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_l2_cache_store(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv || NULL == p_priv->p_l2_cache || 0 == p_priv->l2_cache_offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t bytes = p_priv->cluster_size;
    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    for (uint32_t i = 0; i < p_priv->l2_entries; i++)
    {
        pfs_write_be64(p_raw + i * 8u, p_priv->p_l2_cache[i]);
    }
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->l2_cache_offset);
    pfs_free(p_raw);
    if (rc < 0)
    {
        return rc;
    }
    if (rc != (int)bytes)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_l2_get(pfs_qcow2_priv_t *p_priv, uint64_t guest_off, uint64_t *p_entry)
{
    if (NULL == p_priv || NULL == p_entry || NULL == p_priv->p_l1)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t l1_idx;
    uint32_t l2_idx;
    int rc = pfs_qcow2_l2_guest_index(p_priv, guest_off, &l1_idx, &l2_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t l1_entry = p_priv->p_l1[l1_idx];
    uint64_t l2_off = l1_entry & PFS_QCOW2_OFFSET_MASK;
    if (0 == l2_off)
    {
        *p_entry = 0;
        return PFS_BLKIO_RET_OK;
    }

    rc = pfs_qcow2_l2_cache_load(p_priv, l2_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    *p_entry = p_priv->p_l2_cache[l2_idx];
    return PFS_BLKIO_RET_OK;
}

// 确保 L1[l1_idx] 指向可写 L2 (refcount==1 / COPIED); 必要时分配或 COW
static int pfs_qcow2_l2_ensure_writable(pfs_qcow2_priv_t *p_priv, uint32_t l1_idx)
{
    uint64_t l1_entry = p_priv->p_l1[l1_idx];
    uint64_t old_off = l1_entry & PFS_QCOW2_OFFSET_MASK;

    if (0 == old_off)
    {
        uint64_t new_off;
        int rc = pfs_qcow2_alloc_cluster(p_priv, &new_off);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        // 新 L2 已由 alloc 填 0
        rc = pfs_qcow2_l1_set(p_priv, l1_idx, new_off | PFS_QCOW2_OFLAG_COPIED);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        pfs_qcow2_l2_cache_invalidate(p_priv);
        return PFS_BLKIO_RET_OK;
    }

    if (0 != (l1_entry & PFS_QCOW2_OFLAG_COPIED))
    {
        return PFS_BLKIO_RET_OK;
    }

    // 需 COW: 复制旧 L2 到新簇
    uint64_t old_rc;
    int rc = pfs_qcow2_refcount_get(p_priv, old_off, &old_rc);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_qcow2_l2_cache_load(p_priv, old_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t new_off;
    rc = pfs_qcow2_alloc_cluster(p_priv, &new_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    // alloc 会 invalidate 并可能改 host; 重新指向新簇写当前缓存内容
    p_priv->l2_cache_offset = new_off;
    rc = pfs_qcow2_l2_cache_store(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_qcow2_l1_set(p_priv, l1_idx, new_off | PFS_QCOW2_OFLAG_COPIED);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (old_rc > 0)
    {
        rc = pfs_qcow2_refcount_set(p_priv, old_off, old_rc - 1ull);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_l2_set(pfs_qcow2_priv_t *p_priv, uint64_t guest_off, uint64_t entry)
{
    if (NULL == p_priv || NULL == p_priv->p_l1)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t l1_idx;
    uint32_t l2_idx;
    int rc = pfs_qcow2_l2_guest_index(p_priv, guest_off, &l1_idx, &l2_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    rc = pfs_qcow2_l2_ensure_writable(p_priv, l1_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t l2_off = p_priv->p_l1[l1_idx] & PFS_QCOW2_OFFSET_MASK;
    rc = pfs_qcow2_l2_cache_load(p_priv, l2_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    p_priv->p_l2_cache[l2_idx] = entry;
    return pfs_qcow2_l2_cache_store(p_priv);
}

// end
