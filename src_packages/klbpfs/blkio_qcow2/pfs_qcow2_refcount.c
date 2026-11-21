// Doc Encode : UTF-8 BOM, Unix(LF)
// qcow2 refcount 表/块: 查询/更新; 宿主 cluster 分配
#include "blkio_qcow2/pfs_qcow2_internal.h"
#include "base/pfs_blkio_raw.h"
#include "pfs_endian.h"
#include "base/pfs_mem.h"

#include <string.h>

void pfs_qcow2_refcount_free(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return;
    }
    if (NULL != p_priv->p_reftable)
    {
        pfs_free(p_priv->p_reftable);
        p_priv->p_reftable = NULL;
    }
    if (NULL != p_priv->p_rb_cache)
    {
        pfs_free(p_priv->p_rb_cache);
        p_priv->p_rb_cache = NULL;
    }
    p_priv->rb_cache_offset = 0;
}

int pfs_qcow2_host_write_zeros(pfs_qcow2_priv_t *p_priv, int64_t host_off, uint64_t size)
{
    if (NULL == p_priv || host_off < 0)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == size)
    {
        return PFS_BLKIO_RET_OK;
    }

    uint32_t chunk = 64u * 1024u;
    if ((uint64_t)chunk > size)
    {
        chunk = (uint32_t)size;
    }
    if (chunk > p_priv->cluster_size)
    {
        chunk = p_priv->cluster_size;
    }
    uint8_t *p_zero = (uint8_t *)pfs_mallocz(chunk);
    if (NULL == p_zero)
    {
        return PFS_BLKIO_RET_IO;
    }

    uint64_t left = size;
    int64_t cur = host_off;
    while (left > 0)
    {
        uint32_t n = chunk;
        if ((uint64_t)n > left)
        {
            n = (uint32_t)left;
        }
        int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_zero, (int)n, cur);
        if (rc < 0)
        {
            pfs_free(p_zero);
            return rc;
        }
        if (rc != (int)n)
        {
            pfs_free(p_zero);
            return PFS_BLKIO_RET_IO;
        }
        cur += (int64_t)n;
        left -= (uint64_t)n;
    }
    pfs_free(p_zero);

    if (cur > p_priv->host_file_size)
    {
        p_priv->host_file_size = cur;
    }
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_reftable_load(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->refcount_table_clusters || 0 == p_priv->refcount_table_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }

    pfs_qcow2_refcount_free(p_priv);

    uint64_t bytes = (uint64_t)p_priv->refcount_table_clusters * (uint64_t)p_priv->cluster_size;
    if (0 == bytes || bytes > (uint64_t)PFS_QCOW2_REFTABLE_SIZE_MAX_BYTES ||
            bytes > (uint64_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_BLKIO_RET_STATE;
    }
    p_priv->refcount_table_entries = (uint32_t)(bytes / 8ull);
    if (0 == p_priv->refcount_table_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }

    if (p_priv->refcount_table_offset + bytes < p_priv->refcount_table_offset)
    {
        return PFS_BLKIO_RET_STATE;
    }
    if ((int64_t)(p_priv->refcount_table_offset + bytes) > p_priv->host_file_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint8_t *p_raw = (uint8_t *)pfs_malloc((size_t)bytes);
    if (NULL == p_raw)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->p_reftable = (uint64_t *)pfs_malloc(
            (size_t)p_priv->refcount_table_entries * sizeof(uint64_t));
    if (NULL == p_priv->p_reftable)
    {
        pfs_free(p_raw);
        return PFS_BLKIO_RET_IO;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_raw, (int)bytes, (int64_t)p_priv->refcount_table_offset);
    if (rc < 0)
    {
        pfs_free(p_raw);
        pfs_qcow2_refcount_free(p_priv);
        return rc;
    }
    if (rc != (int)bytes)
    {
        pfs_free(p_raw);
        pfs_qcow2_refcount_free(p_priv);
        return PFS_BLKIO_RET_IO;
    }

    for (uint32_t i = 0; i < p_priv->refcount_table_entries; i++)
    {
        // 规范: bits 9-63 为偏移; 存清标志后的偏移
        p_priv->p_reftable[i] = pfs_read_be64(p_raw + i * 8u) & ~0x1ffull;
    }
    pfs_free(p_raw);
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_rb_entry_get(const uint8_t *p_block, uint32_t idx,
        uint32_t bits, uint64_t *p_count)
{
    if (NULL == p_block || NULL == p_count || 0 == bits || bits > 64u)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    if (16u == bits)
    {
        *p_count = pfs_read_be16(p_block + idx * 2u);
        return PFS_BLKIO_RET_OK;
    }
    if (32u == bits)
    {
        *p_count = pfs_read_be32(p_block + idx * 4u);
        return PFS_BLKIO_RET_OK;
    }
    if (64u == bits)
    {
        *p_count = pfs_read_be64(p_block + idx * 8u);
        return PFS_BLKIO_RET_OK;
    }
    if (8u == bits)
    {
        *p_count = p_block[idx];
        return PFS_BLKIO_RET_OK;
    }

    // 亚字节: 自 LSB 起连续 bits
    {
        uint64_t bit_off = (uint64_t)idx * (uint64_t)bits;
        uint64_t byte_off = bit_off / 8ull;
        uint32_t bit_in_byte = (uint32_t)(bit_off % 8ull);
        uint64_t v = 0;
        uint32_t got = 0;

        while (got < bits)
        {
            uint32_t take;
            uint8_t b = p_block[byte_off];

            take = bits - got;
            if (take > 8u - bit_in_byte)
            {
                take = 8u - bit_in_byte;
            }
            v |= ((uint64_t)((b >> bit_in_byte) & ((1u << take) - 1u))) << got;
            got += take;
            bit_in_byte = 0;
            byte_off++;
        }
        *p_count = v;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_rb_entry_set(uint8_t *p_block, uint32_t idx, uint32_t bits, uint64_t count)
{
    if (NULL == p_block || 0 == bits || bits > 64u)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (bits < 64u)
    {
        uint64_t maxv = (1ull << bits) - 1ull;
        if (count > maxv)
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    if (16u == bits)
    {
        pfs_write_be16(p_block + idx * 2u, (uint16_t)count);
        return PFS_BLKIO_RET_OK;
    }
    if (32u == bits)
    {
        pfs_write_be32(p_block + idx * 4u, (uint32_t)count);
        return PFS_BLKIO_RET_OK;
    }
    if (64u == bits)
    {
        pfs_write_be64(p_block + idx * 8u, count);
        return PFS_BLKIO_RET_OK;
    }
    if (8u == bits)
    {
        p_block[idx] = (uint8_t)count;
        return PFS_BLKIO_RET_OK;
    }

    {
        uint64_t bit_off = (uint64_t)idx * (uint64_t)bits;
        uint64_t byte_off = bit_off / 8ull;
        uint32_t bit_in_byte = (uint32_t)(bit_off % 8ull);
        uint32_t put = 0;

        while (put < bits)
        {
            uint32_t take;
            uint8_t mask;
            uint8_t piece;

            take = bits - put;
            if (take > 8u - bit_in_byte)
            {
                take = 8u - bit_in_byte;
            }
            mask = (uint8_t)(((1u << take) - 1u) << bit_in_byte);
            piece = (uint8_t)(((count >> put) & ((1u << take) - 1u)) << bit_in_byte);
            p_block[byte_off] = (uint8_t)((p_block[byte_off] & (uint8_t)~mask) | piece);
            put += take;
            bit_in_byte = 0;
            byte_off++;
        }
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_rb_cache_load(pfs_qcow2_priv_t *p_priv, uint64_t rb_offset)
{
    if (NULL == p_priv || 0 == rb_offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != (rb_offset & ((uint64_t)p_priv->cluster_size - 1ull)))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (p_priv->rb_cache_offset == rb_offset && NULL != p_priv->p_rb_cache)
    {
        return PFS_BLKIO_RET_OK;
    }

    if (NULL == p_priv->p_rb_cache)
    {
        p_priv->p_rb_cache = (uint8_t *)pfs_malloc((size_t)p_priv->cluster_size);
        if (NULL == p_priv->p_rb_cache)
        {
            return PFS_BLKIO_RET_IO;
        }
    }

    if ((int64_t)(rb_offset + p_priv->cluster_size) > p_priv->host_file_size)
    {
        return PFS_BLKIO_RET_STATE;
    }

    int rc = pfs_blkio_raw_pread(&p_priv->host, p_priv->p_rb_cache, (int)p_priv->cluster_size,
            (int64_t)rb_offset);
    if (rc < 0)
    {
        return rc;
    }
    if (rc != (int)p_priv->cluster_size)
    {
        return PFS_BLKIO_RET_IO;
    }
    p_priv->rb_cache_offset = rb_offset;
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_rb_cache_store(pfs_qcow2_priv_t *p_priv)
{
    if (NULL == p_priv || NULL == p_priv->p_rb_cache || 0 == p_priv->rb_cache_offset)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    int rc = pfs_blkio_raw_pwrite(&p_priv->host, p_priv->p_rb_cache, (int)p_priv->cluster_size,
            (int64_t)p_priv->rb_cache_offset);
    if (rc < 0)
    {
        return rc;
    }
    if (rc != (int)p_priv->cluster_size)
    {
        return PFS_BLKIO_RET_IO;
    }
    return PFS_BLKIO_RET_OK;
}

static int pfs_qcow2_reftable_set(pfs_qcow2_priv_t *p_priv, uint32_t idx, uint64_t rb_offset)
{
    if (NULL == p_priv || NULL == p_priv->p_reftable || idx >= p_priv->refcount_table_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != (rb_offset & 0x1ffull))
    {
        return PFS_BLKIO_RET_STATE;
    }

    p_priv->p_reftable[idx] = rb_offset;
    uint8_t be[8];
    pfs_write_be64(be, rb_offset);
    int64_t off = (int64_t)p_priv->refcount_table_offset + (int64_t)idx * 8;
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

// 在该 table 槽覆盖的首簇放置新 refcount block (自身计入 refcount=1)
static int pfs_qcow2_alloc_refcount_block(pfs_qcow2_priv_t *p_priv, uint32_t table_idx)
{
    if (NULL == p_priv || table_idx >= p_priv->refcount_table_entries)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != p_priv->p_reftable[table_idx])
    {
        return PFS_BLKIO_RET_OK;
    }

    uint64_t self_idx = (uint64_t)table_idx * (uint64_t)p_priv->refcount_block_entries;
    uint64_t new_off = self_idx << p_priv->cluster_bits;

    int rc = pfs_qcow2_host_write_zeros(p_priv, (int64_t)new_off, p_priv->cluster_size);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    if (NULL == p_priv->p_rb_cache)
    {
        p_priv->p_rb_cache = (uint8_t *)pfs_mallocz((size_t)p_priv->cluster_size);
        if (NULL == p_priv->p_rb_cache)
        {
            return PFS_BLKIO_RET_IO;
        }
    }
    else
    {
        memset(p_priv->p_rb_cache, 0, (size_t)p_priv->cluster_size);
    }
    p_priv->rb_cache_offset = new_off;

    // 首簇即本 block, refcount=1
    rc = pfs_qcow2_rb_entry_set(p_priv->p_rb_cache, 0, p_priv->refcount_bits, 1ull);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_qcow2_rb_cache_store(p_priv);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    return pfs_qcow2_reftable_set(p_priv, table_idx, new_off);
}

static int pfs_qcow2_refcount_index(const pfs_qcow2_priv_t *p_priv, uint64_t host_off,
        uint32_t *p_table_idx, uint32_t *p_block_idx)
{
    if (NULL == p_priv || NULL == p_table_idx || NULL == p_block_idx)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 != (host_off & ((uint64_t)p_priv->cluster_size - 1ull)))
    {
        return PFS_BLKIO_RET_STATE;
    }
    if (0 == p_priv->refcount_block_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t clu = host_off >> p_priv->cluster_bits;
    uint64_t b = clu % (uint64_t)p_priv->refcount_block_entries;
    uint64_t t = clu / (uint64_t)p_priv->refcount_block_entries;
    if (t >= (uint64_t)p_priv->refcount_table_entries)
    {
        return PFS_BLKIO_RET_STATE;
    }
    *p_table_idx = (uint32_t)t;
    *p_block_idx = (uint32_t)b;
    return PFS_BLKIO_RET_OK;
}

int pfs_qcow2_refcount_get(pfs_qcow2_priv_t *p_priv, uint64_t host_off, uint64_t *p_count)
{
    if (NULL == p_priv || NULL == p_count || NULL == p_priv->p_reftable)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t table_idx;
    uint32_t block_idx;
    int rc = pfs_qcow2_refcount_index(p_priv, host_off, &table_idx, &block_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t rb_off = p_priv->p_reftable[table_idx];
    if (0 == rb_off)
    {
        *p_count = 0;
        return PFS_BLKIO_RET_OK;
    }

    rc = pfs_qcow2_rb_cache_load(p_priv, rb_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return pfs_qcow2_rb_entry_get(p_priv->p_rb_cache, block_idx, p_priv->refcount_bits, p_count);
}

int pfs_qcow2_refcount_set(pfs_qcow2_priv_t *p_priv, uint64_t host_off, uint64_t count)
{
    if (NULL == p_priv || NULL == p_priv->p_reftable)
    {
        return PFS_BLKIO_RET_PARAM;
    }

    uint32_t table_idx;
    uint32_t block_idx;
    int rc = pfs_qcow2_refcount_index(p_priv, host_off, &table_idx, &block_idx);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }

    uint64_t rb_off = p_priv->p_reftable[table_idx];
    if (0 == rb_off)
    {
        if (0 == count)
        {
            return PFS_BLKIO_RET_OK;
        }
        rc = pfs_qcow2_alloc_refcount_block(p_priv, table_idx);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        rb_off = p_priv->p_reftable[table_idx];
        if (0 == rb_off)
        {
            return PFS_BLKIO_RET_STATE;
        }
    }

    rc = pfs_qcow2_rb_cache_load(p_priv, rb_off);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    rc = pfs_qcow2_rb_entry_set(p_priv->p_rb_cache, block_idx, p_priv->refcount_bits, count);
    if (PFS_BLKIO_RET_OK != rc)
    {
        return rc;
    }
    return pfs_qcow2_rb_cache_store(p_priv);
}

int pfs_qcow2_alloc_cluster(pfs_qcow2_priv_t *p_priv, uint64_t *p_host_off)
{
    if (NULL == p_priv || NULL == p_host_off)
    {
        return PFS_BLKIO_RET_PARAM;
    }
    if (0 == p_priv->refcount_block_entries || 0 == p_priv->refcount_table_entries ||
            NULL == p_priv->p_reftable)
    {
        return PFS_BLKIO_RET_STATE;
    }

    uint64_t max_clu = (uint64_t)p_priv->refcount_table_entries *
            (uint64_t)p_priv->refcount_block_entries;

    for (uint64_t i = 0; i < max_clu; i++)
    {
        uint32_t table_idx = (uint32_t)(i / (uint64_t)p_priv->refcount_block_entries);
        if (0 == p_priv->p_reftable[table_idx])
        {
            // 先建 rb (占用该槽首簇), 再继续找数据空洞
            int rc = pfs_qcow2_alloc_refcount_block(p_priv, table_idx);
            if (PFS_BLKIO_RET_OK != rc)
            {
                return rc;
            }
        }

        uint64_t off = i << p_priv->cluster_bits;
        uint64_t count;
        int rc = pfs_qcow2_refcount_get(p_priv, off, &count);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        if (0 != count)
        {
            continue;
        }

        if ((int64_t)(off + p_priv->cluster_size) > p_priv->host_file_size)
        {
            rc = pfs_qcow2_host_write_zeros(p_priv, (int64_t)off, p_priv->cluster_size);
            if (PFS_BLKIO_RET_OK != rc)
            {
                return rc;
            }
        }
        rc = pfs_qcow2_refcount_set(p_priv, off, 1ull);
        if (PFS_BLKIO_RET_OK != rc)
        {
            return rc;
        }
        pfs_qcow2_l2_cache_invalidate(p_priv);
        *p_host_off = off;
        return PFS_BLKIO_RET_OK;
    }

    return PFS_BLKIO_RET_STATE;
}

// end
