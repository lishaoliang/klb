#include "klbutil/klb_map.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nvector.h"
#include "klbutil/klb_adt.h"
#include "klbthird/sds.h"
#include <assert.h>


klb_map_t* klb_map_create()
{
    klb_map_t* p_map = KLB_MALLOCZ(klb_map_t, 1, 0);
    klb_map_init(p_map);

    return p_map;
}

void klb_map_destroy(klb_map_t* p_map)
{
    assert(NULL != p_map);
    klb_map_quit(p_map);
    KLB_FREE(p_map);
}

void klb_map_init(klb_map_t* p_map)
{
    assert(NULL != p_map);
    assert(NULL == p_map->p_nvector);
    assert(NULL == p_map->p_hlist);

    p_map->p_nvector = klb_nvector_create();
    p_map->p_hlist = klb_hlist_create(0);
}

static int cb_clear_klb_map(void* p_obj, void* p_data)
{
    klb_map_t* p_map = (klb_map_t*)p_obj;
    klb_adt_t* p_adt = (klb_adt_t*)p_data;
    assert(NULL != p_adt);

    klb_adt_quit(p_adt);
    KLB_FREE(p_adt);

    return 0;
}

static void clear_klb_map(klb_map_t* p_map)
{
    // 清空 vector
    klb_nvector_clear(p_map->p_nvector, cb_clear_klb_map, p_map);

    // 清空 hlist
    klb_hlist_clear(p_map->p_hlist, cb_clear_klb_map, p_map);
}

void klb_map_quit(klb_map_t* p_map)
{
    // 清空
    clear_klb_map(p_map);

    KLB_FREE_BY(p_map->p_nvector, klb_nvector_destroy);
    KLB_FREE_BY(p_map->p_hlist, klb_hlist_destroy);
}

//////////////////////////////////////////
static klb_adt_t* get_insert_adt_klb_map_2(klb_map_t* p_map, void* p_key, uint32_t key_len)
{
    klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_find(p_map->p_hlist, p_key, key_len);
    if (NULL == p_adt)
    {
        p_adt = klb_adt_create();
        klb_hlist_push_tail(p_map->p_hlist, p_key, key_len, p_adt);
    }

    return p_adt;
}

static klb_adt_t* get_insert_adt_klb_map(klb_map_t* p_map, const char* p_key)
{
    int key_len = strlen(p_key);
    klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_find(p_map->p_hlist, p_key, key_len);
    if (NULL == p_adt)
    {
        p_adt = klb_adt_create();
        klb_hlist_push_tail(p_map->p_hlist, p_key, key_len, p_adt);
    }

    return p_adt;
}

void klb_map_set_null(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_null(p_adt);
}

void klb_map_set_bool(klb_map_t* p_map, const char* p_key, bool b)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_bool(p_adt, b);
}

void klb_map_set_int64(klb_map_t* p_map, const char* p_key, int64_t i64)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_int64(p_adt, i64);
}

void klb_map_set_uint64(klb_map_t* p_map, const char* p_key, uint64_t u64)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_uint64(p_adt, u64);
}

void klb_map_set_double(klb_map_t* p_map, const char* p_key, double d)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_double(p_adt, d);
}

void klb_map_set_string(klb_map_t* p_map, const char* p_key, const char* p_str)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_string(p_adt, p_str);
}

void klb_map_set_lstring(klb_map_t* p_map, const char* p_key, const char* p_str, int str_len)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_lstring(p_adt, p_str, str_len);
}

void klb_map_set_ptr(klb_map_t* p_map, const char* p_key, const void* ptr1, const void* ptr2)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_ptr(p_adt, ptr1, ptr2);
}

void klb_map_set_map(klb_map_t* p_map, const char* p_key, klb_map_t* ptr)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_map(p_adt, ptr);
}

void klb_map_set_map_clone(klb_map_t* p_map, const char* p_key, const klb_map_t* p_src)
{
    klb_adt_t* p_adt = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_set_map_clone(p_adt, p_src);
}

void klb_map_set_adt(klb_map_t* p_map, const char* p_key, klb_adt_t* p_adt)
{
    if (NULL == p_adt)
    {
        klb_map_set_null(p_map, p_key);
    }
    else
    {
        int key_len = strlen(p_key);
        klb_adt_t* p_old = (klb_adt_t*)klb_hlist_update(p_map->p_hlist, p_key, key_len, p_adt);
        if (NULL == p_old)
        {
            // 未更新成功, 插入新节点
            klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_map->p_hlist, p_key, key_len, p_adt);
            assert(NULL != p_iter);
        }

        KLB_FREE_BY(p_old, klb_adt_destroy);
    }
}

void klb_map_set_adt_clone(klb_map_t* p_map, const char* p_key, const klb_adt_t* p_adt)
{
    klb_adt_t* p_tmp = get_insert_adt_klb_map(p_map, p_key);
    klb_adt_copy(p_tmp, p_adt);
}

int klb_map_type(klb_map_t* p_map, const char* p_key)
{
    int key_len = strlen(p_key);
    klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_find(p_map->p_hlist, p_key, key_len);
    if (NULL != p_adt)
    {
        return p_adt->type;
    }

    return KLB_ADT_null;
}

int klb_map_key_value_size(klb_map_t* p_map)
{
    return klb_hlist_size(p_map->p_hlist);
}

static klb_adt_t* find_hlist_adt_klb_map(klb_map_t* p_map, const char* p_key)
{
    int key_len = strlen(p_key);
    return (klb_adt_t*)klb_hlist_find(p_map->p_hlist, p_key, key_len);
}

bool klb_map_to_bool(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_bool(p_adt);
    }

    return false;
}

int64_t klb_map_to_int64(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_int64(p_adt);
    }

    return 0;
}

uint64_t klb_map_to_uint64(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_uint64(p_adt);
    }

    return 0;
}

double klb_map_to_double(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_double(p_adt);
    }

    return 0.0;
}

const char* klb_map_to_string(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_string(p_adt);
    }

    return "";
}

const char* klb_map_to_lstring(klb_map_t* p_map, const char* p_key, int* p_out_len)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_lstring(p_adt, p_out_len);
    }

    if (NULL != p_out_len)
    {
        *p_out_len = 0;
    }
    return "";
}

const void* klb_map_to_ptr(klb_map_t* p_map, const char* p_key, const void** p_out_ptr2)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_ptr(p_adt, p_out_ptr2);
    }

    if (NULL != p_out_ptr2)
    {
        *p_out_ptr2 = NULL;
    }
    return NULL;
}

klb_map_t* klb_map_to_map(klb_map_t* p_map, const char* p_key)
{
    klb_adt_t* p_adt = find_hlist_adt_klb_map(p_map, p_key);
    if (NULL != p_adt)
    {
        return klb_adt_to_map(p_adt);
    }

    return NULL;
}

const klb_adt_t* klb_map_to_adt(klb_map_t* p_map, const char* p_key)
{
    return find_hlist_adt_klb_map(p_map, p_key);
}

bool klb_map_remove_by_key(klb_map_t* p_map, const char* p_key)
{
    int key_len = strlen(p_key);
    klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_remove_bykey(p_map->p_hlist, p_key, key_len);
    if (NULL != p_adt)
    {
        klb_adt_destroy(p_adt);
        return true;
    }

    return false;
}

klb_adt_t* klb_map_data(klb_map_iter_t* p_iter)
{
    return (klb_adt_t*)klb_hlist_data(p_iter);
}

const char* klb_map_key(klb_map_iter_t* p_iter)
{
    return (const char*)klb_hlist_key(p_iter, NULL);
}

klb_map_iter_t* klb_map_begin(klb_map_t* p_map)
{
    return klb_hlist_begin(p_map->p_hlist);
}

klb_map_iter_t* klb_map_end(klb_map_t* p_map)
{
    return klb_hlist_end(p_map->p_hlist);
}

klb_map_iter_t* klb_map_next(klb_map_iter_t* p_iter)
{
    return klb_hlist_next(p_iter);
}

klb_map_iter_t* klb_map_prev(klb_map_iter_t* p_iter)
{
    return klb_hlist_prev(p_iter);
}

void klb_map_remove(klb_map_t* p_map, klb_map_iter_t* p_iter)
{
    klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_remove(p_map->p_hlist, p_iter);
    assert(NULL != p_adt);

    KLB_FREE_BY(p_adt, klb_adt_destroy);
}

//////////////////////////////////////////////////////////////////////////

void klb_map_append_null(klb_map_t* p_map)
{
    klb_adt_t* p_adt = klb_adt_create();

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_bool(klb_map_t* p_map, bool b)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_bool(p_adt, b);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_int64(klb_map_t* p_map, int64_t i64)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_int64(p_adt, i64);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_uint64(klb_map_t* p_map, uint64_t u64)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_uint64(p_adt, u64);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_double(klb_map_t* p_map, double d)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_double(p_adt, d);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_string(klb_map_t* p_map, const char* p_str)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_string(p_adt, p_str);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_lstring(klb_map_t* p_map, const char* p_str, int str_len)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_lstring(p_adt, p_str, str_len);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_ptr(klb_map_t* p_map, const void* ptr1, const void* ptr2)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_ptr(p_adt, ptr1, ptr2);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);

}

void klb_map_append_map(klb_map_t* p_map, klb_map_t* ptr)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_map(p_adt, ptr);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_map_clone(klb_map_t* p_map, const klb_map_t* p_src)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_map_clone(p_adt, p_src);

    klb_nvector_push_tail(p_map->p_nvector, p_adt);
}

void klb_map_append_adt(klb_map_t* p_map, klb_adt_t* p_adt)
{
    if (NULL == p_adt)
    {
        klb_map_append_null(p_map);
    }
    else
    {
        klb_nvector_push_tail(p_map->p_nvector, p_adt);
    }
}

void klb_map_append_adt_clone(klb_map_t* p_map, const klb_adt_t* p_adt)
{
    klb_adt_t* p_tmp = klb_adt_create();
    klb_adt_copy(p_tmp, p_adt);

    klb_map_append_adt(p_map, p_tmp);
}

///////////////////////////////////////////////

static void check_array_idx_klb_map(klb_map_t* p_map, int idx)
{
    if (idx < 0) return;

    int u_size = (uint32_t)idx + 1;
    int size = klb_nvector_size(p_map->p_nvector);
    
    if (size < u_size)
    {
        int count = u_size - size;

        while (0 < count)
        {
            klb_map_append_null(p_map);
            count--;
        }
    }
}

void klb_map_set_idx_null(klb_map_t* p_map, int idx)
{
    if (idx < 0)
    {
        klb_map_append_null(p_map);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_null(p_adt);
    }
}

void klb_map_set_idx_bool(klb_map_t* p_map, int idx, bool b)
{
    if (idx < 0)
    {
        klb_map_append_bool(p_map, b);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_bool(p_adt, b);
    }
}

void klb_map_set_idx_int64(klb_map_t* p_map, int idx, int64_t i64)
{
    if (idx < 0)
    {
        klb_map_append_int64(p_map, i64);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_int64(p_adt, i64);
    }
}

void klb_map_set_idx_uint64(klb_map_t* p_map, int idx, uint64_t u64)
{
    if (idx < 0)
    {
        klb_map_append_uint64(p_map, u64);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_uint64(p_adt, u64);
    }
}

void klb_map_set_idx_double(klb_map_t* p_map, int idx, double d)
{
    if (idx < 0)
    {
        klb_map_append_double(p_map, d);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_double(p_adt, d);
    }
}

void klb_map_set_idx_string(klb_map_t* p_map, int idx, const char* p_str)
{
    if (idx < 0)
    {
        klb_map_append_string(p_map, p_str);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_string(p_adt, p_str);
    }
}

void klb_map_set_idx_lstring(klb_map_t* p_map, int idx, const char* p_str, int str_len)
{
    if (idx < 0)
    {
        klb_map_append_lstring(p_map, p_str, str_len);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_lstring(p_adt, p_str, str_len);
    }
}

void klb_map_set_idx_ptr(klb_map_t* p_map, int idx, const void* ptr1, const void* ptr2)
{
    if (idx < 0)
    {
        klb_map_append_ptr(p_map, ptr1, ptr2);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_ptr(p_adt, ptr1, ptr2);
    }
}

void klb_map_set_idx_map(klb_map_t* p_map, int idx, klb_map_t* ptr)
{
    if (idx < 0)
    {
        klb_map_append_map(p_map, ptr);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_map(p_adt, ptr);
    }
}

void klb_map_set_idx_map_clone(klb_map_t* p_map, int idx, const klb_map_t* p_src)
{
    klb_map_t* p_tmp = klb_map_create();
    klb_map_copy(p_tmp, p_src);

    klb_map_set_idx_map(p_map, idx, p_tmp);
}

void klb_map_set_idx_adt(klb_map_t* p_map, int idx, klb_adt_t* p_adt)
{
    if (idx < 0)
    {
        klb_map_append_adt(p_map, p_adt);
    }
    else
    {
        check_array_idx_klb_map(p_map, idx);
        assert(idx < klb_nvector_size(p_map->p_nvector));

        klb_adt_t* p_old = (klb_adt_t*)klb_nvector_update(p_map->p_nvector, idx, p_adt);
        assert(NULL != p_old);
        KLB_FREE_BY(p_old, klb_adt_destroy);
    }
}

void klb_map_set_idx_adt_clone(klb_map_t* p_map, int idx, const klb_adt_t* p_adt)
{
    klb_adt_t* p_tmp = klb_adt_create();
    klb_adt_copy(p_tmp, p_adt);

    klb_map_set_idx_adt(p_map, idx, p_tmp);
}

static klb_adt_t* find_vector_adt_klb_map(klb_map_t* p_map, int idx)
{
    assert(NULL != p_map);

    if (0 <= idx && idx < klb_nvector_size(p_map->p_nvector))
    {
        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_map->p_nvector, idx);
        assert(NULL != p_adt);

        return p_adt;
    }

    return NULL;
}

int klb_map_array_type(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);

    if(NULL != p_adt)
    {
        return klb_adt_type(p_adt);
    }

    return KLB_ADT_null;
}

int klb_map_array_size(klb_map_t* p_map)
{
    return klb_nvector_size(p_map->p_nvector);
}

bool klb_map_idx_to_bool(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_bool(p_adt);
    }
    return false;
}

int64_t klb_map_idx_to_int64(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_int64(p_adt);
    }
    return 0;
}

uint64_t klb_map_idx_to_uint64(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_uint64(p_adt);
    }
    return 0;
}

double klb_map_idx_to_double(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_double(p_adt);
    }
    return 0.0;
}

const char* klb_map_idx_to_string(const klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_string(p_adt);
    }
    return "";
}

const char* klb_map_idx_to_lstring(klb_map_t* p_map, int idx, int* p_out_len)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_lstring(p_adt, p_out_len);
    }

    if (NULL != p_out_len)
    {
        *p_out_len = 0;
    }
    return "";
}

const void* klb_map_idx_to_ptr(klb_map_t* p_map, int idx, const void** p_out_ptr2)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_ptr(p_adt, p_out_ptr2);
    }

    if (NULL != p_out_ptr2)
    {
        *p_out_ptr2 = NULL;
    }
    return NULL;
}

klb_map_t* klb_map_idx_to_map(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = find_vector_adt_klb_map(p_map, idx);
    if (NULL != p_adt)
    {
        return klb_adt_to_map(p_adt);
    }
    return NULL;
}

const klb_adt_t* klb_map_idx_to_adt(klb_map_t* p_map, int idx)
{
    return find_vector_adt_klb_map(p_map, idx);
}

bool klb_map_idx_remove(klb_map_t* p_map, int idx)
{
    klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_remove(p_map->p_nvector, idx);
    if (NULL != p_adt)
    {
        klb_adt_destroy(p_adt);
        return true;
    }

    return false;
}

bool klb_map_idx_remove_tail(klb_map_t* p_map)
{
    klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_pop_tail(p_map->p_nvector);
    if (NULL != p_adt)
    {
        klb_adt_destroy(p_adt);
        return true;
    }

    return false;
}

////////////////////////////////////////

static void copy_klb_map(klb_map_t* p_dst, const klb_map_t* p_src)
{
    // vector
    for (int i = 0; i < klb_nvector_size(p_src->p_nvector); i++)
    {
        klb_adt_t* p_adt_vec = (klb_adt_t*)klb_nvector_get(p_src->p_nvector, i);

        check_array_idx_klb_map(p_dst, i);
        assert(i < klb_nvector_size(p_dst->p_nvector));

        klb_adt_t* p_tmp_vec = (klb_adt_t*)klb_nvector_get(p_dst->p_nvector, i);
        assert(NULL != p_tmp_vec);
        
        klb_adt_copy(p_tmp_vec, p_adt_vec);
    }

    // h list
    klb_hlist_iter_t* iter = klb_hlist_begin(p_src->p_hlist);
    while (NULL != iter)
    {
        klb_adt_t* p_adt = (klb_adt_t*)klb_hlist_data(iter);
        
        uint32_t key_len = 0;
        void* p_key = klb_hlist_key(iter, &key_len);
        klb_adt_t* p_tmp = get_insert_adt_klb_map_2(p_dst, p_key, key_len);
        assert(NULL != p_tmp);

        klb_adt_copy(p_tmp, p_adt);

        iter = klb_hlist_next(iter);
    }
}

int klb_map_size(klb_map_t* p_map)
{
    return (int)klb_hlist_size(p_map->p_hlist) + klb_nvector_size(p_map->p_nvector);
}

void klb_map_clear(klb_map_t* p_map)
{
    clear_klb_map(p_map);
}

void klb_map_copy(klb_map_t* p_dst, const klb_map_t* p_src)
{
    assert(NULL != p_dst);

    // 清空
    clear_klb_map(p_dst);

    if (NULL == p_src)
    {
        return;
    }

    copy_klb_map(p_dst, p_src);
}

//////////////////////////////////////////////////////////////////////////

int klb_map_test()
{

    klb_map_t* p_a = klb_map_create();
    klb_map_t* p_b = klb_map_create();

    klb_map_set_bool(p_a, "1", true);
    klb_map_set_int64(p_a, "2", 775);
    klb_map_set_uint64(p_a, "3", 777);
    klb_map_set_double(p_a, "4", 3.1415926);
    klb_map_set_string(p_a, "5", "55555");
    klb_map_set_ptr(p_a, "6", (const void*)klb_map_test, NULL);

    klb_map_set_idx_bool(p_a, 1, true);
    klb_map_set_idx_int64(p_a, 2, 775);
    klb_map_set_idx_uint64(p_a, 3, 777);
    klb_map_set_idx_double(p_a, 4, 3.1415926);
    klb_map_set_idx_string(p_a, 5, "55555");
    klb_map_set_idx_ptr(p_a, 6, (const void*)klb_map_test, NULL);

    klb_map_set_map_clone(p_a, "9", p_a);
    klb_map_set_idx_map_clone(p_a, 9, p_a);

    klb_map_copy(p_b, p_a);
    klb_map_destroy(p_a);

    bool a_1 = klb_map_to_bool(p_b, "1");
    int64_t a_2 = klb_map_to_int64(p_b, "2");
    uint64_t a_3 = klb_map_to_uint64(p_b, "3");
    double a_4 = klb_map_to_double(p_b, "4");
    const char* a_5 = klb_map_to_string(p_b, "5");
    const void* a_6 = klb_map_to_ptr(p_b, "6", NULL);

    int ti_0 = klb_map_array_type(p_b, 0);

    bool i_1 = klb_map_idx_to_bool(p_b, 1);
    int64_t i_2 = klb_map_idx_to_int64(p_b, 2);
    uint64_t i_3 = klb_map_idx_to_uint64(p_b, 3);
    double i_4 = klb_map_idx_to_double(p_b, 4);
    const char* i_5 = klb_map_idx_to_string(p_b, 5);
    const void* i_6 = klb_map_idx_to_ptr(p_b, 6, NULL);

    klb_map_remove_by_key(p_b, "9");
    klb_map_idx_remove(p_b, 5);

    klb_map_destroy(p_b);
    return 0;
}
