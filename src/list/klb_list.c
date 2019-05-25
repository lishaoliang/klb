#include "list/klb_list.h"
#include "mem/klb_mem.h"
#include <assert.h>


/// @struct klb_list_iter_t
/// @brief  list节点 OR 迭代子
typedef struct klb_list_iter_t_
{
    void*               p_data;         ///< 节点数据

    klb_list_iter_t*    p_prev;         ///< 前一个节点
    klb_list_iter_t*    p_next;         ///< 后一个节点
}klb_list_iter_t;


/// @struct klb_list_t
/// @brief  list对象
typedef struct klb_list_t_
{
    int                 size;           ///< 节点成员数目

    klb_list_iter_t*    p_head;         ///< 起始节点
    klb_list_iter_t*    p_tail;         ///< 末尾节点
}klb_list_t;



klb_list_t* klb_list_create()
{
    klb_list_t* p_list = KLB_MALLOC(klb_list_t, 1, 0);
    KLB_MEMSET(p_list, 0, sizeof(klb_list_t));

    return p_list;
}

void klb_list_destroy(klb_list_t* p_list)
{
    assert(NULL != p_list);
    assert(0 == p_list->size);
    assert(NULL == p_list->p_head);
    assert(NULL == p_list->p_tail);

    KLB_FREE(p_list);
}

void klb_list_push_head(klb_list_t* p_list, void* p_data)
{
    assert(NULL != p_list);
}

void klb_list_push_tail(klb_list_t* p_list, void* p_data)
{
    assert(NULL != p_list);
}

void* klb_list_pop_head(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return NULL;
}

void* klb_list_pop_tail(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return NULL;
}

void* klb_list_head(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return (NULL != p_list->p_head) ? p_list->p_head->p_data : NULL;
}

void* klb_list_tail(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return (NULL != p_list->p_tail) ? p_list->p_tail->p_data : NULL;
}

int klb_list_size(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return p_list->size;
}

void* klb_list_remove(klb_list_t* p_list, klb_list_iter_t* p_iter)
{
    assert(NULL != p_list);

    return NULL;
}

klb_list_iter_t* klb_list_begin(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return p_list->p_head;
}

klb_list_iter_t* klb_list_next(klb_list_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return p_iter->p_next;
}

klb_list_iter_t* klb_list_end(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return p_list->p_tail;
}

klb_list_iter_t* klb_list_prev(klb_list_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return p_iter->p_prev;
}
