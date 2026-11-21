// Doc Encode : UTF-8 BOM, Unix(LF)
// 公共扇区数据缓存 (按扇区号 LRU; slab 对齐分配, 每块多槽; 经 p_blkio 读扇区)
#include "base/pfs_cache_sector.h"
#include "base/pfs_mem.h"
#include "base/pfs_time.h"
#include "pfs_blkio.h"

#include <string.h>

// 返回值数值对齐 pfs_ret_e
#define PFS_CS_OK           0
#define PFS_CS_EIO          (-5)
#define PFS_CS_ENOMEM       (-12)
#define PFS_CS_EBUSY        (-16)
#define PFS_CS_EINVAL       (-22)
#define PFS_CS_ERANGE       (-34)

// 单槽步长: max(sector_size, 4K) 再向上 4K 对齐
static uint32_t pfs_cache_sector_slot_stride(uint32_t sector_size)
{
    uint32_t stride = sector_size;

    if (stride < PFS_CACHE_SECTOR_PAGE_4K)
    {
        stride = PFS_CACHE_SECTOR_PAGE_4K;
    }
    stride = (stride + PFS_CACHE_SECTOR_PAGE_4K - 1u) & ~(PFS_CACHE_SECTOR_PAGE_4K - 1u);
    return stride;
}

static void pfs_cache_sector_touch(pfs_cache_sector_slot_t *p_ent)
{
    p_ent->tick_count = pfs_get_tickcount();
}

static pfs_cache_sector_slot_t *pfs_cache_sector_find_lru(pfs_cache_sector_t *p_cache)
{
    uint32_t now = pfs_get_tickcount();
    pfs_cache_sector_slot_t *p_lru = NULL;
    uint32_t oldest_age = 0;

    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        pfs_cache_sector_slot_t *p_ent = &p_cache->slots[i];

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

static pfs_cache_sector_slot_t *pfs_cache_sector_find_invalid(pfs_cache_sector_t *p_cache)
{
    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        if (!p_cache->slots[i].valid)
        {
            return &p_cache->slots[i];
        }
    }
    return NULL;
}

static int pfs_cache_sector_load(pfs_cache_sector_t *p_cache, pfs_cache_sector_slot_t *p_ent,
        uint64_t sector_id, int64_t sector_offset)
{
    if (NULL == p_cache->p_blkio || NULL == p_ent->p_data)
    {
        return PFS_CS_EINVAL;
    }
    if (sector_offset < 0)
    {
        return PFS_CS_EINVAL;
    }
    if (0 == p_cache->sector_size ||
            p_cache->sector_size > (uint32_t)PFS_BLKIO_IO_SIZE_MAX)
    {
        return PFS_CS_EINVAL;
    }

    int nread = pfs_blkio_pread(p_cache->p_blkio, p_ent->p_data, (int)p_cache->sector_size,
            sector_offset);
    if (nread < 0 || nread != (int)p_cache->sector_size)
    {
        p_ent->valid = false;
        return PFS_CS_EIO;
    }

    p_ent->sector_id = sector_id;
    p_ent->sector_offset = sector_offset;
    p_ent->valid = true;
    return PFS_CS_OK;
}

static void pfs_cache_sector_free_slabs(pfs_cache_sector_t *p_cache)
{
    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLAB; i++)
    {
        if (NULL != p_cache->p_slab[i])
        {
            pfs_aligned_free(p_cache->p_slab[i]);
            p_cache->p_slab[i] = NULL;
        }
    }
    p_cache->slab_count = 0;
}

int pfs_cache_sector_init(pfs_cache_sector_t *p_cache, pfs_blkio_t *p_blkio, uint32_t sector_size)
{
    if (NULL == p_cache || NULL == p_blkio || 0 == sector_size)
    {
        return PFS_CS_EINVAL;
    }
    // 已初始化则须先 deinit, 避免 slab 泄漏
    if (0 != p_cache->slab_count || NULL != p_cache->p_slab[0])
    {
        return PFS_CS_EBUSY;
    }

    uint32_t slot_stride = pfs_cache_sector_slot_stride(sector_size);
    if (0 == slot_stride ||
            slot_stride > (UINT32_MAX / PFS_CACHE_SECTOR_SLOTS_PER_SLAB))
    {
        return PFS_CS_EINVAL;
    }
    size_t slab_bytes = (size_t)slot_stride * (size_t)PFS_CACHE_SECTOR_SLOTS_PER_SLAB;

    memset(p_cache, 0, sizeof(*p_cache));
    p_cache->p_blkio = p_blkio;
    p_cache->sector_size = sector_size;
    p_cache->slot_stride = slot_stride;

    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLAB; i++)
    {
        p_cache->p_slab[i] = (uint8_t *)pfs_aligned_malloc((size_t)PFS_CACHE_SECTOR_PAGE_4K, slab_bytes);
        if (NULL == p_cache->p_slab[i])
        {
            pfs_cache_sector_free_slabs(p_cache);
            memset(p_cache, 0, sizeof(*p_cache));
            return PFS_CS_ENOMEM;
        }
        p_cache->slab_count++;
    }

    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        uint32_t slab_idx = i / PFS_CACHE_SECTOR_SLOTS_PER_SLAB;
        uint32_t slot_idx = i % PFS_CACHE_SECTOR_SLOTS_PER_SLAB;
        p_cache->slots[i].p_data =
                p_cache->p_slab[slab_idx] + (size_t)slot_idx * (size_t)slot_stride;
        p_cache->slots[i].valid = false;
        p_cache->slots[i].tick_count = 0;
        p_cache->slots[i].sector_id = 0;
        p_cache->slots[i].sector_offset = 0;
    }

    return PFS_CS_OK;
}

int pfs_cache_sector_deinit(pfs_cache_sector_t *p_cache)
{
    if (NULL == p_cache)
    {
        return PFS_CS_EINVAL;
    }

    pfs_cache_sector_free_slabs(p_cache);

    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        p_cache->slots[i].valid = false;
        p_cache->slots[i].tick_count = 0;
        p_cache->slots[i].sector_id = 0;
        p_cache->slots[i].sector_offset = 0;
        p_cache->slots[i].p_data = NULL;
    }

    p_cache->p_blkio = NULL;
    p_cache->sector_size = 0;
    p_cache->slot_stride = 0;
    return PFS_CS_OK;
}

pfs_cache_sector_slot_t *pfs_cache_sector_get(pfs_cache_sector_t *p_cache,
        uint64_t sector_id, int64_t sector_offset)
{
    if (NULL == p_cache || NULL == p_cache->p_blkio || 0 == p_cache->slab_count ||
            NULL == p_cache->p_slab[0] || sector_offset < 0)
    {
        return NULL;
    }

    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        pfs_cache_sector_slot_t *p_ent = &p_cache->slots[i];
        if (p_ent->valid && p_ent->sector_id == sector_id)
        {
            pfs_cache_sector_touch(p_ent);
            return p_ent;
        }
    }

    pfs_cache_sector_slot_t *p_ent = pfs_cache_sector_find_invalid(p_cache);
    if (NULL == p_ent)
    {
        p_ent = pfs_cache_sector_find_lru(p_cache);
        if (NULL == p_ent)
        {
            return NULL;
        }
    }

    p_ent->valid = false;
    if (PFS_CS_OK != pfs_cache_sector_load(p_cache, p_ent, sector_id, sector_offset))
    {
        return NULL;
    }
    pfs_cache_sector_touch(p_ent);
    return p_ent;
}

void pfs_cache_sector_invalidate(pfs_cache_sector_t *p_cache, uint64_t sector_id)
{
    if (NULL == p_cache)
    {
        return;
    }
    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        if (p_cache->slots[i].valid && p_cache->slots[i].sector_id == sector_id)
        {
            p_cache->slots[i].valid = false;
            return;
        }
    }
}

void pfs_cache_sector_invalidate_all(pfs_cache_sector_t *p_cache)
{
    if (NULL == p_cache)
    {
        return;
    }
    for (uint32_t i = 0; i < PFS_CACHE_SECTOR_MAX_SLOTS; i++)
    {
        p_cache->slots[i].valid = false;
    }
}

// end
