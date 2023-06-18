// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_fpool.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_atomic.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>



#pragma pack(4)

typedef struct klb_fpool_buf_extra_t_
{
    klb_buf_t               buf;                ///< buf
    klb_atomic_t volatile   atomic_count;       ///< klb_buf_t 使用计数
    klb_fpool_t*            p_pool;             ///< 来自哪个内存池
}klb_fpool_buf_extra_t;

#pragma pack()


// 固定缓存大小内存池
typedef struct klb_fpool_t_
{
    long volatile   atomic_lock;        ///< 申请/释放的原子锁: 支持夸线程申请/释放

    int             aligned;            ///< 内存对齐
    size_t          item_size;          ///< 元素项大小(字节)

    // 整体缓存
    struct
    {
        size_t      buf_len;            ///< 数据缓存长度
        char*       p_buf;              ///< 数据缓存
    };

    // 索引
    struct
    {
        size_t      idx_max;            ///< 索引数目
        size_t      idx_buf_len;        ///< 索引内存大小
        char*       p_idx_buf;          ///< 索引
    };

    size_t          idle_num;           ///< 当前空闲的块数目
    klb_buf_t*      p_idle;             ///< 空闲块头指针
}klb_fpool_t;


//////////////////////////////////////////////////////////////////////////

static klb_fpool_buf_extra_t* get_buf_by_idx_klb_fpool(klb_fpool_t* p_pool, int idx)
{
    int offset = idx * sizeof(klb_fpool_buf_extra_t);
    return (klb_fpool_buf_extra_t*)(p_pool->p_idx_buf + offset);
}

klb_fpool_t* klb_fpool_create(int item_size, int item_num, int aligned)
{
    klb_fpool_t* p_pool = (klb_fpool_t*)KLB_MALLOCZ(klb_fpool_t, 1, 0);

    p_pool->aligned = aligned;

    // buf
    p_pool->item_size = item_size;
    p_pool->buf_len = item_num * p_pool->item_size;

    if (0 < aligned)
    {
        p_pool->p_buf = (char*)KLB_MALLOC_ALIGNED(char, p_pool->buf_len, 0, aligned);
    }
    else
    {
        p_pool->p_buf = (char*)KLB_MALLOC(char, p_pool->buf_len, 0);
    }

    // index
    p_pool->idx_max = item_num;
    p_pool->idx_buf_len = p_pool->idx_max * (sizeof(klb_fpool_buf_extra_t));
    p_pool->p_idx_buf = (char*)KLB_MALLOCZ(char, p_pool->idx_buf_len, 0);

    // init
    char* ptr = p_pool->p_buf;

    for (size_t i = 0; i < p_pool->idx_max; i++)
    {
        klb_fpool_buf_extra_t* p_buf_ex = get_buf_by_idx_klb_fpool(p_pool, i);

        p_buf_ex->buf.type = KLB_BUF_FIX_POOL;
        p_buf_ex->buf.p_buf = ptr;
        p_buf_ex->buf.buf_len = p_pool->item_size;

        klb_atomic_set_zero(&p_buf_ex->atomic_count);
        p_buf_ex->p_pool = p_pool;

        //
        p_buf_ex->buf.p_next = p_pool->p_idle;
        p_pool->p_idle = (klb_buf_t*)p_buf_ex;
        p_pool->idle_num += 1;

        // 
        ptr += p_pool->item_size;
    }

    klb_atomic_set_zero(&p_pool->atomic_lock);

    return p_pool;
}

void klb_fpool_destroy(klb_fpool_t* p_pool)
{
    assert(NULL != p_pool);

    if (0 < p_pool->aligned)
    {
        KLB_FREE_ALIGNED(p_pool->p_buf);
    }
    else
    {
        KLB_FREE(p_pool->p_buf);
    }

    KLB_FREE(p_pool->p_idx_buf);
    KLB_FREE(p_pool);
}

size_t klb_fpool_total_mem_size(klb_fpool_t* p_pool)
{
    return p_pool->buf_len + p_pool->idx_buf_len;
}

size_t klb_fpool_item_size(klb_fpool_t* p_pool)
{
    return p_pool->item_size;
}

size_t klb_fpool_idle_num(klb_fpool_t* p_pool)
{
    return p_pool->idle_num;
}

klb_buf_t* klb_fpool_malloc(void* ptr, size_t size)
{
    klb_fpool_t* p_pool = (klb_fpool_t*)ptr;
    assert(NULL != p_pool);

    size_t num = 1;

    if (0 < size)
    {
        num = (size + p_pool->item_size - 1) / p_pool->item_size;
        assert(size <= p_pool->item_size * num);
    }

    if (0 < num)
    {
        klb_atomic_lock(&p_pool->atomic_lock);
        if (p_pool->idle_num < num)
        {
            klb_atomic_unlock(&p_pool->atomic_lock);
            return NULL;
        }

        klb_buf_t* p_buf = p_pool->p_idle;
        klb_buf_t* p_last = p_buf;
        while (0 < num && NULL != p_pool->p_idle)
        {
            p_last = p_pool->p_idle;
            p_pool->p_idle = p_pool->p_idle->p_next;

            p_pool->idle_num--;
            num--;
        }

        p_last->p_next = NULL;

        klb_atomic_unlock(&p_pool->atomic_lock);

        klb_buf_t* p_cur = p_buf;
        while (NULL != p_cur)
        {
            assert(KLB_BUF_FIX_POOL == p_cur->type);

            klb_fpool_buf_extra_t* p_ex = (klb_fpool_buf_extra_t*)p_cur;
            klb_atomic_set_value(&p_ex->atomic_count, 1);

            p_cur->start = 0;
            p_cur->end = 0;

            // next
            p_cur = p_cur->p_next;
        }

        return p_buf;
    }

    return NULL;
}

int klb_fpool_ref(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);
    assert(KLB_BUF_FIX_POOL == p_buf->type);

    klb_fpool_buf_extra_t* p_buf_ex = (klb_fpool_buf_extra_t*)p_buf;

    int n = klb_atomic_add(&p_buf_ex->atomic_count);

    return n + 1;
}

int klb_fpool_unref(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);
    assert(KLB_BUF_FIX_POOL == p_buf->type);

    klb_fpool_buf_extra_t* p_buf_ex = (klb_fpool_buf_extra_t*)p_buf;

    int n = klb_atomic_sub(&p_buf_ex->atomic_count);

    if (n <= 1)
    {
        assert(1 == n);

        klb_fpool_t* p_pool = p_buf_ex->p_pool;
        klb_atomic_set_value(&p_buf_ex->atomic_count, 0);

        klb_atomic_lock(&p_pool->atomic_lock);
        p_buf->p_next = p_pool->p_idle;
        p_pool->p_idle = p_buf;
        p_pool->idle_num += 1;
        klb_atomic_unlock(&p_pool->atomic_lock);
    }

    return n - 1;
}
