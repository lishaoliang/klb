#include "klbutil/klb_vector.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klb_vector_t
/// @brief  vector对象
typedef struct klb_vector_t_
{
    uint32_t            size;           ///< 节点成员数目
}klb_vector_t;


klb_vector_t* klb_vector_create()
{
    return NULL;
}


void klb_vector_destroy(klb_vector_t* p_vector)
{

}


void klb_vector_clean(klb_vector_t* p_vector, klb_vector_clean_cb cb_clean, void* p_obj)
{

}

void klb_vector_push_head(klb_vector_t* p_vector, void* p_data)
{

}


void klb_vector_push_tail(klb_vector_t* p_vector, void* p_data)
{

}


void* klb_vector_pop_head(klb_vector_t* p_vector)
{
    return NULL;
}

void* klb_vector_pop_tail(klb_vector_t* p_vector)
{
    return NULL;
}

void* klb_vector_head(klb_vector_t* p_vector)
{
    return NULL;
}

void* klb_vector_tail(klb_vector_t* p_vector)
{
    return NULL;
}


void* klb_vector_get(klb_vector_t* p_vector, int index)
{
    return NULL;
}

void* klb_vector_remove(klb_vector_t* p_vector, int index)
{
    return NULL;
}


uint32_t klb_vector_size(klb_vector_t* p_vector)
{
    return 0;
}
