// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_bufpoolfix.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_fpool.h"


//////////////////////////////////////////////////////////////////////////


#define KLB_BUFPOOLFIX_32M          (32 * 1024 * 1024)

#define KLB_BUFPOOLFIX_4K           (4096)
#define KLB_BUFPOOLFIX_8K           (8192)
#define KLB_BUFPOOLFIX_16K          (16384)



/// @struct klb_bufpoolfix_t
/// @brief  固定大小缓存池
typedef struct klb_bufpoolfix_t_
{
    klb_fpool_t*        p_fpool;        ///< 内存池
}klb_bufpoolfix_t;


//////////////////////////////////////////////////////////////////////////

/// @brief 创建 缓存池
klb_bufpoolfix_t* klb_bufpoolfix_create()
{
    klb_bufpoolfix_t* p_bufpool = KLB_MALLOCZ(klb_bufpoolfix_t, 1, 0);


    return p_bufpool;
}

/// @brief 销毁 缓存池
void klb_bufpoolfix_destroy(klb_bufpoolfix_t* p_bufpool)
{
    KLB_FREE_BY(p_bufpool->p_fpool, klb_fpool_destroy);
    KLB_FREE(p_bufpool);
}

static void klb_bufpoolfix_init_fpool(klb_bufpoolfix_t* p_bufpool)
{
    assert(NULL == p_bufpool->p_fpool);
    int item_num = KLB_BUFPOOLFIX_32M / KLB_BUFPOOLFIX_4K;

    p_bufpool->p_fpool = klb_fpool_create(KLB_BUFPOOLFIX_4K, item_num, KLB_BUFPOOLFIX_4K);
}

klb_buf_t* klb_bufpoolfix_malloc(void* ptr, size_t size)
{
    klb_bufpoolfix_t* p_bufpool = (klb_bufpoolfix_t*)ptr;

    // 初始化
    if (NULL == p_bufpool->p_fpool)
    {
        klb_bufpoolfix_init_fpool(p_bufpool);
    }

    return klb_fpool_malloc(p_bufpool->p_fpool, size);
}

//end
