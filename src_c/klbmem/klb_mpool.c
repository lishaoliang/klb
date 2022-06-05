// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mpool.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_fpool.h"
#include <assert.h>


typedef struct klb_mpool_t_
{
    int a;
}klb_mpool_t;


klb_mpool_t* klb_mpool_create()
{
    return NULL;
}

void klb_mpool_destroy(klb_mpool_t* p_pool)
{

}

klb_buf_t* klb_mpool_malloc(klb_mpool_t* p_pool)
{
    return NULL;
}
