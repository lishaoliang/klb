// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


/// @struct klb_nlist_iter_t
/// @brief  list节点 OR 迭代子
typedef struct klb_nlist_iter_t_
{
    void*               p_data;         ///< 节点数据

    klb_nlist_iter_t*    p_prev;         ///< 前一个节点
    klb_nlist_iter_t*    p_next;         ///< 后一个节点
}klb_nlist_iter_t;


/// @struct klb_nlist_t
/// @brief  list对象
typedef struct klb_nlist_t_
{
    klb_nlist_iter_t*    p_head;         ///< 起始节点
    klb_nlist_iter_t*    p_tail;         ///< 末尾节点

    uint32_t            size;           ///< 节点成员数目
}klb_nlist_t;



klb_nlist_t* klb_nlist_create()
{
    klb_nlist_t* p_list = KLB_MALLOC(klb_nlist_t, 1, 0);
    KLB_MEMSET(p_list, 0, sizeof(klb_nlist_t));

    return p_list;
}

void klb_nlist_destroy(klb_nlist_t* p_list)
{
    assert(NULL != p_list);
    assert(0 == p_list->size);
    assert(NULL == p_list->p_head);
    assert(NULL == p_list->p_tail);

    // 销毁前必须释放所有节点

    KLB_FREE(p_list);
}


void klb_nlist_clean(klb_nlist_t* p_list, klb_nlist_clean_cb cb_clean, void* p_obj)
{
    assert(NULL != p_list);

    while (true)
    {
        void* p_data = klb_nlist_pop_head(p_list);

        if (NULL != p_data)
        {
            if (NULL != cb_clean)
            {
                cb_clean(p_obj, p_data);
            }
        }
        else
        {
            break;
        }
    }
}


void klb_nlist_push_head(klb_nlist_t* p_list, void* p_data)
{
    assert(NULL != p_list);

    if (NULL != p_data)
    {
        klb_nlist_iter_t* p_iter = KLB_MALLOC(klb_nlist_iter_t, 1, 0);
        KLB_MEMSET(p_iter, 0, sizeof(klb_nlist_iter_t));

        p_iter->p_data = p_data;

        p_iter->p_prev = NULL;
        p_iter->p_next = p_list->p_head;

        // 设置后节点的 p_prev指向
        if (NULL != p_iter->p_next)
        {
            p_iter->p_next->p_prev = p_iter;
        }

        // 如果插入首个元素
        if (NULL == p_list->p_tail)
        {
            assert(0 == p_list->size);
            p_list->p_tail = p_iter;
        }

        p_list->p_head = p_iter;
        p_list->size += 1;
    }
}

void klb_nlist_push_tail(klb_nlist_t* p_list, void* p_data)
{
    assert(NULL != p_list);

    if (NULL != p_data)
    {
        klb_nlist_iter_t* p_iter = KLB_MALLOC(klb_nlist_iter_t, 1, 0);
        KLB_MEMSET(p_iter, 0, sizeof(klb_nlist_iter_t));

        p_iter->p_data = p_data;

        p_iter->p_prev = p_list->p_tail;
        p_iter->p_next = NULL;

        // 设置前节点的 p_next
        if (NULL != p_iter->p_prev)
        {
            p_iter->p_prev->p_next = p_iter;
        }

        // 如果插入首个元素
        if (NULL == p_list->p_head)
        {
            assert(0 == p_list->size);
            p_list->p_head = p_iter;
        }

        p_list->p_tail = p_iter;
        p_list->size += 1;
    }
}

void* klb_nlist_pop_head(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    klb_nlist_iter_t* p_iter = p_list->p_head;

    if (NULL != p_iter)
    {
        assert(0 <= p_list->size);
        p_list->p_head = p_iter->p_next;
        p_list->size -= 1;

        if (NULL != p_iter->p_next)
        {
            p_iter->p_next->p_prev = NULL;
            assert(0 < p_list->size);
        }
        else
        {
            p_list->p_tail = NULL;
            assert(0 == p_list->size);
        }

        void* p_data = p_iter->p_data;

        KLB_FREE(p_iter);
        return p_data;
    }

    return NULL;
}

void* klb_nlist_pop_tail(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    klb_nlist_iter_t* p_iter = p_list->p_tail;

    if (NULL != p_iter)
    {
        assert(0 <= p_list->size);
        p_list->p_tail = p_iter->p_prev;
        p_list->size -= 1;

        if (NULL != p_iter->p_prev)
        {
            p_iter->p_prev->p_next = NULL;
            assert(0 < p_list->size);
        }
        else
        {
            p_list->p_head = NULL;
            assert(0 == p_list->size);
        }

        void* p_data = p_iter->p_data;

        KLB_FREE(p_iter);
        return p_data;
    }

    return NULL;
}

void* klb_nlist_head(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    return (NULL != p_list->p_head) ? p_list->p_head->p_data : NULL;
}

void* klb_nlist_tail(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    return (NULL != p_list->p_tail) ? p_list->p_tail->p_data : NULL;
}

int klb_nlist_size(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    return (int)p_list->size;
}

void* klb_nlist_remove(klb_nlist_t* p_list, klb_nlist_iter_t* p_iter)
{
    assert(NULL != p_list);
    
    if (NULL != p_iter)
    {
        klb_nlist_iter_t* p_prev = p_iter->p_prev;
        klb_nlist_iter_t* p_next = p_iter->p_next;

        if (NULL != p_prev)
        {
            p_prev->p_next = p_next;

            if (NULL != p_next)
            {
                p_next->p_prev = p_prev;
            }
            else
            {
                p_list->p_tail = p_prev;
            }
        }
        else
        {
            p_list->p_head = p_next;

            if (NULL != p_next)
            {
                p_next->p_prev = NULL;
            }
            else
            {
                assert(1 == p_list->size);
                p_list->p_tail = NULL;
            }
        }

        p_list->size -= 1;
        assert(0 <= p_list->size);

        void* p_data = p_iter->p_data;
        KLB_FREE(p_iter);

        return p_data;
    }

    return NULL;
}

void* klb_nlist_data(klb_nlist_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return (NULL != p_iter) ? p_iter->p_data : NULL;
}

klb_nlist_iter_t* klb_nlist_begin(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    return p_list->p_head;
}

klb_nlist_iter_t* klb_nlist_next(klb_nlist_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return p_iter->p_next;
}

klb_nlist_iter_t* klb_nlist_end(klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    return p_list->p_tail;
}

klb_nlist_iter_t* klb_nlist_prev(klb_nlist_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return p_iter->p_prev;
}
