﻿///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_hmap.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "klbutil/klb_hmap.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hash.h"
#include <math.h>
#include <assert.h>


/// @def   KLB_HMAP_DOUBLE_HASH
/// @brief 是否采用双hash降低冲突概率
#define KLB_HMAP_DOUBLE_HASH    1


/// @def   KLB_HMAP_IDX_TABLE_MIN
/// @brief 容量/掩码表序号的最小值
#define KLB_HMAP_IDX_TABLE_MIN  8       // 2^8 = 256


/// @def   KLB_HMAP_IDX_TABLE_MAX
/// @brief 容量/掩码表序号的最大值
#define KLB_HMAP_IDX_TABLE_MAX  26      // 2^26 * sizeof(klb_hmap_node_t) <= 2^32


/// @struct klb_hmap_talbe_t
/// @brief  容量/掩码表
typedef struct klb_hmap_talbe_t_
{
    uint32_t    max;            ///< 容量最大值
    uint32_t    mask;           ///< 掩码值
}klb_hmap_talbe_t;


///< 容量/掩码表
static klb_hmap_talbe_t s_klb_hmap_talbe[32] = {
    {0x00000001, 0x00000001-1}, {0x00000002, 0x00000002-1}, {0x00000004, 0x00000004-1}, {0x00000008, 0x00000008-1},
    {0x00000010, 0x00000010-1}, {0x00000020, 0x00000020-1}, {0x00000040, 0x00000040-1}, {0x00000080, 0x00000080-1},
    {0x00000100, 0x00000100-1}, {0x00000200, 0x00000200-1}, {0x00000400, 0x00000400-1}, {0x00000800, 0x00000800-1},
    {0x00001000, 0x00001000-1}, {0x00002000, 0x00002000-1}, {0x00004000, 0x00004000-1}, {0x00008000, 0x00008000-1},
    {0x00010000, 0x00010000-1}, {0x00020000, 0x00020000-1}, {0x00040000, 0x00040000-1}, {0x00080000, 0x00080000-1},
    {0x00100000, 0x00100000-1}, {0x00200000, 0x00200000-1}, {0x00400000, 0x00400000-1}, {0x00800000, 0x00800000-1},
    {0x01000000, 0x01000000-1}, {0x02000000, 0x02000000-1}, {0x04000000, 0x04000000-1}, {0x08000000, 0x08000000-1},
    {0x10000000, 0x10000000-1}, {0x20000000, 0x20000000-1}, {0x40000000, 0x40000000-1}, {0x80000000, 0x80000000-1},
};


/// @struct klb_hmap_t
/// @brief  hash table查找
typedef struct klb_hmap_t_
{
    klb_hmap_node_t** p_idx;    ///< hash table节点数组

    int         auto_malloc;    ///< KLB_HTAB_AUTO_MALLOC.自动分配节点内存
    uint32_t    size;           ///< 当前节点数数
    uint32_t    idx_talbe;      ///< hash table 序号最大数
}klb_hmap_t;


klb_hmap_t* klb_hmap_create(int auto_malloc)
{
    assert((pow(2, KLB_HMAP_IDX_TABLE_MAX) * sizeof(klb_hmap_node_t)) <= pow(2, 32));

    klb_hmap_t* p_hmap = KLB_MALLOC(klb_hmap_t, 1, 0);
    KLB_MEMSET(p_hmap, 0, sizeof(klb_hmap_t));

    p_hmap->auto_malloc = auto_malloc;
    p_hmap->idx_talbe = KLB_HMAP_IDX_TABLE_MIN;

    // 常驻内存量: sizeof(void*) * ht_max
    p_hmap->p_idx = KLB_MALLOC(klb_hmap_node_t*, s_klb_hmap_talbe[p_hmap->idx_talbe].max, 0);
    KLB_MEMSET(p_hmap->p_idx, 0, s_klb_hmap_talbe[p_hmap->idx_talbe].max * sizeof(klb_hmap_node_t*));

    return p_hmap;
}

void klb_hmap_destroy(klb_hmap_t* p_hmap)
{
    assert(NULL != p_hmap);
    assert(0 == p_hmap->size);

    KLB_FREE(p_hmap->p_idx);
    KLB_FREE(p_hmap);
}

int klb_hmap_clean(klb_hmap_t* p_hmap, klb_hmap_clean_cb cb_clean, void* p_obj)
{
    assert(NULL != p_hmap);

    for (uint32_t i = 0; i < s_klb_hmap_talbe[p_hmap->idx_talbe].max; i++)
    {
        klb_hmap_node_t* p_cur = p_hmap->p_idx[i];

        while (p_cur)
        {
            klb_hmap_node_t* p_tmp = p_cur;
            p_cur = p_cur->p_next;

            if (NULL != cb_clean)
            {
                cb_clean(p_obj, p_tmp->p_data);
            }

            if (KLB_HMAP_AUTO_MALLOC == p_hmap->auto_malloc)
            {
                KLB_FREE(p_tmp);
            }
        }

        p_hmap->p_idx[i] = NULL;
    }

    p_hmap->size = 0;

    return 0;
}

static bool klb_hmap_check_resize_push(klb_hmap_t* p_hmap, klb_hmap_node_t* p_node)
{
    // 扩大容量,缩小容量时, 插入单个节点
    void* p_key = p_node->p_key;
    uint32_t key_len = p_node->key_len;
    uint32_t dx = p_node->hash_idx;
    uint32_t idx = dx & s_klb_hmap_talbe[p_hmap->idx_talbe].mask;

#if KLB_HMAP_DOUBLE_HASH
    uint32_t hash = p_node->hash_cmp;
#else
    uint32_t hash = dx;
#endif

    klb_hmap_node_t* p_cur = p_hmap->p_idx[idx];
    klb_hmap_node_t* p_prev = NULL;

    while (p_cur)
    {
        if (hash == p_cur->hash_cmp)
        {
            if (key_len < p_cur->key_len)
            {
                break; // 让具有相同 hash值, 但key长度小的排前面
            }
            else if (key_len == p_cur->key_len)
            {
                // hash值相等时, key长度相等, 再比较原始key
                int cmp = memcmp(p_key, p_cur->p_key, key_len);

                if (cmp < 0)
                {
                    break; // 小值排前面
                }
                else if (cmp == 0)
                {
                    assert(false); // 不应该出现这种情况
                    return false; //已存在值, 且完全一致的key
                }
            }
        }
        else if (hash < p_cur->hash_cmp)
        {
            break; // 让结果从小到大排列
        }

        p_prev = p_cur;
        p_cur = p_cur->p_next;
    }

    if (NULL != p_prev)
    {
        // 插入中间/末尾
        p_node->p_next = p_prev->p_next;
        p_prev->p_next = p_node;
    }
    else
    {
        // 插入首位
        // 注意: 首位可能已有节点
        p_node->p_next = p_hmap->p_idx[idx];
        p_hmap->p_idx[idx] = p_node;
    }

    return true;
}

static void klb_hmap_check_resize_inc(klb_hmap_t* p_hmap)
{
    // 检查扩大容量
    // 条件: 当前容量的1/2 <= 当前节点数目; 没到最大值KLB_HMAP_IDX_TABLE_MAX
    if (KLB_HMAP_IDX_TABLE_MAX <= p_hmap->idx_talbe)
    {
        return; // 已经最大容量了
    }

    assert(KLB_HMAP_IDX_TABLE_MIN <= p_hmap->idx_talbe);
    if (p_hmap->size < s_klb_hmap_talbe[p_hmap->idx_talbe - 1].max)
    {
        return; // 在适合的范围内, 无需扩容
    }

    // 扩大容量

    int old_idx = p_hmap->idx_talbe;
    klb_hmap_node_t** p_old_idx = p_hmap->p_idx;

    p_hmap->idx_talbe += 1;
    p_hmap->p_idx = KLB_MALLOC(klb_hmap_node_t*, s_klb_hmap_talbe[p_hmap->idx_talbe].max, 0);
    KLB_MEMSET(p_hmap->p_idx, 0, s_klb_hmap_talbe[p_hmap->idx_talbe].max * sizeof(klb_hmap_node_t*));

    uint32_t num = 0;
    for (uint32_t i = 0; i < s_klb_hmap_talbe[old_idx].max; i++)
    {
        klb_hmap_node_t* p_next = p_old_idx[i];
        while (NULL != p_next)
        {
            klb_hmap_node_t* p_cur = p_next;
            p_next = p_next->p_next;

            p_cur->p_next = NULL;
            if (klb_hmap_check_resize_push(p_hmap, p_cur))
            {
                num += 1;
            }
        }
    }

    assert(num == p_hmap->size);
    KLB_FREE(p_old_idx);
}

static void klb_hmap_check_resize_dec(klb_hmap_t* p_hmap)
{
    // 检查缩小容量
    // 条件: 当前容量的1/4 >= 当前节点数目; 没有到最小值KLB_HMAP_IDX_TABLE_MIN
    if (p_hmap->idx_talbe <= KLB_HMAP_IDX_TABLE_MIN)
    {
        return; // 已经最小了
    }

    if (s_klb_hmap_talbe[p_hmap->idx_talbe - 2].max < p_hmap->size)
    {
        return; // 在适合的范围内, 无需缩小容量
    }

    // 缩小容量

    int old_idx = p_hmap->idx_talbe;
    klb_hmap_node_t** p_old_idx = p_hmap->p_idx;

    p_hmap->idx_talbe -= 1;
    p_hmap->p_idx = KLB_MALLOC(klb_hmap_node_t*, s_klb_hmap_talbe[p_hmap->idx_talbe].max, 0);
    KLB_MEMSET(p_hmap->p_idx, 0, s_klb_hmap_talbe[p_hmap->idx_talbe].max * sizeof(klb_hmap_node_t*));

#if 1
    // 前面的序列是一致的
    memcpy(p_hmap->p_idx, p_old_idx, s_klb_hmap_talbe[p_hmap->idx_talbe].max * sizeof(klb_hmap_node_t*));

    // 只重新插入后面的一部分
    uint32_t num = 0;
    for (uint32_t i = s_klb_hmap_talbe[p_hmap->idx_talbe].max; i < s_klb_hmap_talbe[old_idx].max; i++)
    {
        klb_hmap_node_t* p_next = p_old_idx[i];
        while (NULL != p_next)
        {
            klb_hmap_node_t* p_cur = p_next;
            p_next = p_next->p_next;

            p_cur->p_next = NULL;
            if (klb_hmap_check_resize_push(p_hmap, p_cur))
            {
                num += 1;
            }
        }
    }
#else
    uint32_t num = 0;
    for (uint32_t i = 0; i < s_klb_hmap_talbe[old_idx].max; i++)
    {
        klb_hmap_node_t* p_next = p_old_idx[i];
        while (NULL != p_next)
        {
            klb_hmap_node_t* p_cur = p_next;
            p_next = p_next->p_next;

            p_cur->p_next = NULL;
            if (klb_hmap_check_resize_push(p_hmap, p_cur))
            {
                num += 1;
            }
        }
    }

    assert(num == p_hmap->size);
#endif

    KLB_FREE(p_old_idx);
}

static klb_hmap_node_t* klb_hmap_create_node(void* p_key, uint32_t key_len, uint32_t hash_idx, uint32_t hash_cmp, void* p_data, klb_hmap_node_t* p_next)
{
    klb_hmap_node_t* p_node = KLB_MALLOC(klb_hmap_node_t, 1, 0);
    KLB_MEMSET(p_node, 0, sizeof(klb_hmap_node_t));

    p_node->p_key = p_key;
    p_node->key_len = key_len;
    p_node->hash_idx = hash_idx;
    p_node->hash_cmp = hash_cmp;
    p_node->p_data = p_data;
    p_node->p_next = p_next;

    return p_node;
}

static void klb_hmap_init_node(klb_hmap_node_t* p_node, void* p_key, uint32_t key_len, uint32_t hash_idx, uint32_t hash_cmp, void* p_data, klb_hmap_node_t* p_next)
{
    p_node->p_key = p_key;
    p_node->key_len = key_len;
    p_node->hash_idx = hash_idx;
    p_node->hash_cmp = hash_cmp;
    p_node->p_data = p_data;
    p_node->p_next = p_next;
}

int klb_hmap_push(klb_hmap_t* p_hmap, void* p_key, uint32_t key_len, void* p_data, klb_hmap_node_t* p_hmap_node)
{
    assert(NULL != p_hmap);
    assert(NULL != p_key);
    assert(NULL != p_data);

    uint32_t dx = klb_hash32((const char*)p_key, key_len);
    uint32_t idx = dx & s_klb_hmap_talbe[p_hmap->idx_talbe].mask;

#if KLB_HMAP_DOUBLE_HASH
    uint32_t hash = klb_hash_hgsmi((const char*)p_key, key_len);
#else
    uint32_t hash = dx;
#endif

    klb_hmap_node_t* p_cur = p_hmap->p_idx[idx];
    klb_hmap_node_t* p_prev = NULL;

    while (p_cur)
    {
        if (hash == p_cur->hash_cmp)
        {
            if (key_len < p_cur->key_len)
            {
                break; // 让具有相同 hash值, 但key长度小的排前面
            }
            else if (key_len == p_cur->key_len)
            {
                // hash值相等时, key长度相等, 再比较原始key
                int cmp = memcmp(p_key, p_cur->p_key, key_len);

                if (cmp < 0)
                {
                    break; // 小值排前面
                }
                else if (cmp == 0)
                {
                    return 1; //已存在值, 且完全一致的key
                }
            }
        }
        else if (hash < p_cur->hash_cmp)
        {
            break; // 让结果从小到大排列
        }

        p_prev = p_cur;
        p_cur = p_cur->p_next;
    }

    if (NULL != p_prev)
    {
        // 插入中间/末尾
        if (KLB_HMAP_AUTO_MALLOC == p_hmap->auto_malloc)
        {
            assert(NULL == p_hmap_node);
            p_prev->p_next = klb_hmap_create_node(p_key, key_len, dx, hash, p_data, p_prev->p_next);
        }
        else
        {
            assert(NULL != p_hmap_node);
            klb_hmap_init_node(p_hmap_node, p_key, key_len, dx, hash, p_data, p_prev->p_next);
            p_prev->p_next = p_hmap_node;
        }
    }
    else
    {
        // 插入首位
        // 注意: 首位可能已有数据
        if (KLB_HMAP_AUTO_MALLOC == p_hmap->auto_malloc)
        {
            assert(NULL == p_hmap_node);
            p_hmap->p_idx[idx] = klb_hmap_create_node(p_key, key_len, dx, hash, p_data, p_hmap->p_idx[idx]);
        }
        else
        {
            assert(NULL != p_hmap_node);
            klb_hmap_init_node(p_hmap_node, p_key, key_len, dx, hash, p_data, p_hmap->p_idx[idx]);
            p_hmap->p_idx[idx] = p_hmap_node;
        }
    }

    p_hmap->size += 1;

    // 插入成功之后, 检查是否需要扩大容量
    klb_hmap_check_resize_inc(p_hmap);

    return 0;
}

void* klb_hmap_remove(klb_hmap_t* p_hmap, void* p_key, uint32_t key_len)
{
    assert(NULL != p_hmap);
    assert(NULL != p_key);

    uint32_t dx = klb_hash32((const char*)p_key, key_len);
    uint32_t idx = dx & s_klb_hmap_talbe[p_hmap->idx_talbe].mask;

#if KLB_HMAP_DOUBLE_HASH
    uint32_t hash = klb_hash_hgsmi((const char*)p_key, key_len);
#else
    uint32_t hash = dx;
#endif

    klb_hmap_node_t* p_cur = p_hmap->p_idx[idx];
    klb_hmap_node_t* p_prev = NULL;

    while (p_cur)
    {
        if (hash == p_cur->hash_cmp)
        {
            if (key_len == p_cur->key_len &&
                0 == memcmp(p_key, p_cur->p_key, key_len))
            {
                void* p_data = p_cur->p_data;

                if (NULL != p_prev)
                {
                    p_prev->p_next = p_cur->p_next;
                }
                else
                {
                    // 需要删除的在第一个坑位
                    p_hmap->p_idx[idx] = p_cur->p_next;
                }

                if (KLB_HMAP_AUTO_MALLOC == p_hmap->auto_malloc)
                {
                    KLB_FREE(p_cur);
                }

                p_hmap->size -= 1;

                // 移除成功之后, 检查是否需要缩小容量
                klb_hmap_check_resize_dec(p_hmap);

                return p_data;
            }
        }
        else if (hash < p_cur->hash_cmp)
        {
            break;
        }

        p_prev = p_cur;
        p_cur = p_cur->p_next;
    }

    return NULL;
}

void* klb_hmap_find(klb_hmap_t* p_hmap, const void* p_key, uint32_t key_len)
{
    assert(NULL != p_hmap);
    assert(NULL != p_key);

    uint32_t dx = klb_hash32((const char*)p_key, key_len);
    uint32_t idx = dx & s_klb_hmap_talbe[p_hmap->idx_talbe].mask;

#if KLB_HMAP_DOUBLE_HASH
    uint32_t hash = klb_hash_hgsmi((const char*)p_key, key_len);
#else
    uint32_t hash = dx;
#endif

    klb_hmap_node_t* p_cur = p_hmap->p_idx[idx];

    while (p_cur)
    {
        if (hash == p_cur->hash_cmp)
        {
            if (key_len == p_cur->key_len &&
                0 == memcmp(p_key, p_cur->p_key, key_len))
            {
                return p_cur->p_data;
            }
        }
        else if (hash < p_cur->hash_cmp)
        {
            break;
        }

        p_cur = p_cur->p_next;
    }

    return NULL;
}

uint32_t klb_hmap_size(klb_hmap_t* p_hmap)
{
    assert(NULL != p_hmap);

    return p_hmap->size;
}
