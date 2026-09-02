// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbutil/klb_list.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


static int cb_clear_klb_list(void* p_obj, void* p_data)
{
    klb_adt_t* p_adt = (klb_adt_t*)p_data;

    (void)p_obj;
    KLB_FREE_BY(p_adt, klb_adt_destroy);

    return 0;
}

void klb_list_init(klb_list_t* p_list)
{
    assert(NULL != p_list);
    assert(NULL == p_list->p_nlist);

    p_list->p_nlist = klb_nlist_create();
}

void klb_list_quit(klb_list_t* p_list)
{
    assert(NULL != p_list);

    // step1. 清空托管 adt
    klb_list_clear(p_list);

    // step2. 销毁底层 nlist
    KLB_FREE_BY(p_list->p_nlist, klb_nlist_destroy);
}

klb_list_t* klb_list_create()
{
    klb_list_t* p_list = KLB_MALLOCZ(klb_list_t, 1, 0);
    klb_list_init(p_list);

    return p_list;
}

void klb_list_destroy(klb_list_t* p_list)
{
    assert(NULL != p_list);

    klb_list_quit(p_list);
    KLB_FREE(p_list);
}

void klb_list_push_head(klb_list_t* p_list, klb_adt_t* p_adt)
{
    assert(NULL != p_list);

    if (NULL == p_adt)
    {
        p_adt = klb_adt_create();
    }

    klb_nlist_push_head(p_list->p_nlist, p_adt);
}

void klb_list_push_tail(klb_list_t* p_list, klb_adt_t* p_adt)
{
    assert(NULL != p_list);

    if (NULL == p_adt)
    {
        p_adt = klb_adt_create();
    }

    klb_nlist_push_tail(p_list->p_nlist, p_adt);
}

void klb_list_pop_head(klb_list_t* p_list)
{
    assert(NULL != p_list);

    klb_adt_t* p_adt = (klb_adt_t*)klb_nlist_pop_head(p_list->p_nlist);
    KLB_FREE_BY(p_adt, klb_adt_destroy);
}

void klb_list_pop_tail(klb_list_t* p_list)
{
    assert(NULL != p_list);

    klb_adt_t* p_adt = (klb_adt_t*)klb_nlist_pop_tail(p_list->p_nlist);
    KLB_FREE_BY(p_adt, klb_adt_destroy);
}

klb_adt_t* klb_list_head(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return (klb_adt_t*)klb_nlist_head(p_list->p_nlist);
}

klb_adt_t* klb_list_tail(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return (klb_adt_t*)klb_nlist_tail(p_list->p_nlist);
}

int klb_list_size(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return klb_nlist_size(p_list->p_nlist);
}

void klb_list_remove(klb_list_t* p_list, klb_list_iter_t* p_iter)
{
    assert(NULL != p_list);
    assert(NULL != p_iter);

    klb_adt_t* p_adt = (klb_adt_t*)klb_nlist_remove(p_list->p_nlist, p_iter);
    KLB_FREE_BY(p_adt, klb_adt_destroy);
}

klb_adt_t* klb_list_data(klb_list_iter_t* p_iter)
{
    assert(NULL != p_iter);

    return (klb_adt_t*)klb_nlist_data(p_iter);
}

klb_list_iter_t* klb_list_begin(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return klb_nlist_begin(p_list->p_nlist);
}

klb_list_iter_t* klb_list_next(klb_list_iter_t* p_iter)
{
    return klb_nlist_next(p_iter);
}

klb_list_iter_t* klb_list_end(klb_list_t* p_list)
{
    assert(NULL != p_list);

    return klb_nlist_end(p_list->p_nlist);
}

klb_list_iter_t* klb_list_prev(klb_list_iter_t* p_iter)
{
    return klb_nlist_prev(p_iter);
}

void klb_list_clear(klb_list_t* p_list)
{
    assert(NULL != p_list);

    klb_nlist_clear(p_list->p_nlist, cb_clear_klb_list, p_list);
}

// end
