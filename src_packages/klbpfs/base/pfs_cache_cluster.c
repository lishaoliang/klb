// Doc Encode : UTF-8 BOM, Unix(LF)
// 公共簇数据缓存 (按簇号 LRU; slab 对齐分配, 每块多槽; 经 p_blkio 读簇)
#include "base/pfs_cache_cluster.h"
#include "base/pfs_mem.h"
#include "base/pfs_time.h"
#include "pfs_blkio.h"

#include <string.h>

// 返回值数值对齐 pfs_ret_e
#define PFS_CC_OK           0
#define PFS_CC_EIO          (-5)
#define PFS_CC_ENOMEM       (-12)
#define PFS_CC_EBUSY        (-16)
#define PFS_CC_EINVAL       (-22)
#define PFS_CC_ERANGE       (-34)

// 单槽步长: max(cluster_size, 4K) 再向上 4K 对齐
static uint32_t pfs_cache_cluster_slot_stride(uint32_t cluster_size)
{
    uint32_t stride = cluster_size;
    if (stride < PFS_CACHE_CLUSTER_PAGE_4K)
    {
        stride = PFS_CACHE_CLUSTER_PAGE_4K;
    }
    stride = (stride + PFS_CACHE_CLUSTER_PAGE_4K - 1u) & ~(PFS_CACHE_CLUSTER_PAGE_4K - 1u);
    return stride;
}

static void pfs_cache_cluster_touch(pfs_cache_cluster_slot_t *p_ent)
{
    p_ent->tick_count = pfs_get_tickcount();
}

static pfs_cache_cluster_slot_t *pfs_cache_cluster_find_lru(pfs_cache_cluster_t *p_cache)
{
    uint32_t oldest_age = 0;
    pfs_cache_cluster_slot_t *p_lru = NULL;

    uint32_t now = pfs_get_tickcount();
    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        pfs_cache_cluster_slot_t *p_ent = &p_cache->slots[i];
        if (!p_ent->valid)
        {
            continue;
        }

        uint32_t age = now - p_ent->tick_count;
        if (NULL == p_lru || age > oldest_age)
        {
            oldest_age = age;
            p_lru = p_ent;
        }
    }
    return p_lru;
}

static pfs_cache_cluster_slot_t *pfs_cache_cluster_find_invalid(pfs_cache_cluster_t *p_cache)
{
    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        if (!p_cache->slots[i].valid)
        {
            return &p_cache->slots[i];
        }
    }
    return NULL;
}

static int pfs_cache_cluster_load(pfs_cache_cluster_t *p_cache, pfs_cache_cluster_slot_t *p_ent, uint32_t cluster_id, int64_t cluster_offset)
{
    if (NULL == p_cache->p_blkio || NULL == p_ent->p_data)
    {
        return PFS_CC_EINVAL;
    }

    if (cluster_offset < 0)
    {
        return PFS_CC_EINVAL;
    }

    if (0 == p_cache->cluster_size || p_cache->cluster_size > (uint32_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_CC_EINVAL;
    }

    int nread = pfs_blkio_pread(p_cache->p_blkio, p_ent->p_data, (int)p_cache->cluster_size, cluster_offset);
    if (nread < 0 || nread != (int)p_cache->cluster_size)
    {
        p_ent->valid = false;
        return PFS_CC_EIO;
    }

    p_ent->cluster_id = cluster_id;
    p_ent->cluster_offset = cluster_offset;
    p_ent->valid = true;
    return PFS_CC_OK;
}

static void pfs_cache_cluster_free_slabs(pfs_cache_cluster_t *p_cache)
{
    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLAB; i++)
    {
        if (NULL != p_cache->p_slab[i])
        {
            pfs_aligned_free(p_cache->p_slab[i]);
            p_cache->p_slab[i] = NULL;
        }
    }
    p_cache->slab_count = 0;
}

int pfs_cache_cluster_init(pfs_cache_cluster_t *p_cache, pfs_blkio_t *p_blkio, uint32_t cluster_size)
{
    if (NULL == p_cache || NULL == p_blkio || 0 == cluster_size)
    {
        return PFS_CC_EINVAL;
    }

    // 已初始化则须先 deinit, 避免 slab 泄漏
    if (0 != p_cache->slab_count || NULL != p_cache->p_slab[0])
    {
        return PFS_CC_EBUSY;
    }

    uint32_t slot_stride = pfs_cache_cluster_slot_stride(cluster_size);
    if (0 == slot_stride || slot_stride > (UINT32_MAX / PFS_CACHE_CLUSTER_SLOTS_PER_SLAB))
    {
        return PFS_CC_EINVAL;
    }
    size_t slab_bytes = (size_t)slot_stride * (size_t)PFS_CACHE_CLUSTER_SLOTS_PER_SLAB;

    memset(p_cache, 0, sizeof(*p_cache));
    p_cache->p_blkio = p_blkio;
    p_cache->cluster_size = cluster_size;
    p_cache->slot_stride = slot_stride;

    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLAB; i++)
    {
        p_cache->p_slab[i] = (uint8_t *)pfs_aligned_malloc((size_t)PFS_CACHE_CLUSTER_PAGE_4K, slab_bytes);
        if (NULL == p_cache->p_slab[i])
        {
            pfs_cache_cluster_free_slabs(p_cache);
            memset(p_cache, 0, sizeof(*p_cache));
            return PFS_CC_ENOMEM;
        }
        p_cache->slab_count++;
    }

    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        uint32_t slab_idx = i / PFS_CACHE_CLUSTER_SLOTS_PER_SLAB;
        uint32_t slot_idx = i % PFS_CACHE_CLUSTER_SLOTS_PER_SLAB;
        p_cache->slots[i].p_data = p_cache->p_slab[slab_idx] + (size_t)slot_idx * (size_t)slot_stride;
        p_cache->slots[i].valid = false;
        p_cache->slots[i].tick_count = 0;
        p_cache->slots[i].cluster_id = 0;
        p_cache->slots[i].cluster_offset = 0;
    }

    return PFS_CC_OK;
}

int pfs_cache_cluster_deinit(pfs_cache_cluster_t *p_cache)
{
    if (NULL == p_cache)
    {
        return PFS_CC_EINVAL;
    }

    pfs_cache_cluster_free_slabs(p_cache);

    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        p_cache->slots[i].valid = false;
        p_cache->slots[i].tick_count = 0;
        p_cache->slots[i].cluster_id = 0;
        p_cache->slots[i].cluster_offset = 0;
        p_cache->slots[i].p_data = NULL;
    }

    p_cache->p_blkio = NULL;
    p_cache->cluster_size = 0;
    p_cache->slot_stride = 0;
    return PFS_CC_OK;
}

pfs_cache_cluster_slot_t *pfs_cache_cluster_get(pfs_cache_cluster_t *p_cache, uint32_t cluster_id, int64_t cluster_offset)
{
    if (NULL == p_cache || NULL == p_cache->p_blkio || 0 == p_cache->slab_count ||
            NULL == p_cache->p_slab[0] || cluster_offset < 0)
    {
        return NULL;
    }

    pfs_cache_cluster_slot_t *p_ent = NULL; 
    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        p_ent = &p_cache->slots[i];
        if (p_ent->valid && p_ent->cluster_id == cluster_id)
        {
            pfs_cache_cluster_touch(p_ent);
            return p_ent;
        }
    }

    p_ent = pfs_cache_cluster_find_invalid(p_cache);
    if (NULL == p_ent)
    {
        p_ent = pfs_cache_cluster_find_lru(p_cache);
        if (NULL == p_ent)
        {
            return NULL;
        }
    }

    p_ent->valid = false;
    if (PFS_CC_OK != pfs_cache_cluster_load(p_cache, p_ent, cluster_id, cluster_offset))
    {
        return NULL;
    }

    pfs_cache_cluster_touch(p_ent);
    return p_ent;
}

void pfs_cache_cluster_invalidate(pfs_cache_cluster_t *p_cache, uint32_t cluster_id)
{
    if (NULL == p_cache)
    {
        return;
    }

    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        if (p_cache->slots[i].valid && p_cache->slots[i].cluster_id == cluster_id)
        {
            p_cache->slots[i].valid = false;
            return;
        }
    }
}

void pfs_cache_cluster_invalidate_all(pfs_cache_cluster_t *p_cache)
{
    if (NULL == p_cache)
    {
        return;
    }

    for (uint32_t i = 0; i < PFS_CACHE_CLUSTER_MAX_SLOTS; i++)
    {
        p_cache->slots[i].valid = false;
    }
}

// end
