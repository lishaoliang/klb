#include "klbutil/klb_adt.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_map.h"
#include <assert.h>


void klb_adt_int(klb_adt_t* p_adt)
{
    KLB_MEMSET(p_adt, 0, sizeof(klb_adt_t));
    p_adt->type = KLB_ADT_null;
}

void klb_adt_quit(klb_adt_t* p_adt)
{
    switch (p_adt->type)
    {
    case KLB_ADT_string:
        {
            KLB_FREE_BY(p_adt->str, sdsfree);
        }
        break;
    case KLB_ADT_map:
        {
            KLB_FREE_BY(p_adt->p_map, klb_map_destroy);
        }
        break;
    case KLB_ADT_null:
    case KLB_ADT_ptr:
    case KLB_ADT_bool:
    case KLB_ADT_double:
    case KLB_ADT_uint64:
    case KLB_ADT_int64:
        break;
    default:
        assert(false);
        break;
    }

    KLB_MEMSET(p_adt, 0, sizeof(klb_adt_t));
    p_adt->type = KLB_ADT_null;
}

klb_adt_t* klb_adt_create()
{
    klb_adt_t* p_adt = KLB_MALLOC(klb_adt_t, 1, 0);
    klb_adt_int(p_adt);

    return p_adt;
}

void klb_adt_destroy(klb_adt_t* p_adt)
{
    klb_adt_quit(p_adt);
    KLB_FREE(p_adt);
}

void klb_adt_set_null(klb_adt_t* p_adt)
{
    klb_adt_quit(p_adt);
}

void klb_adt_set_bool(klb_adt_t* p_adt, bool b)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_bool;
    p_adt->b = b;
}

void klb_adt_set_string(klb_adt_t* p_adt, const char* p_str)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_string;
    p_adt->str = sdsnew(p_str);
}

void klb_adt_set_lstring(klb_adt_t* p_adt, const char* p_str, int str_len)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_string;
    p_adt->str = sdsnewlen(p_str, str_len);
}

void klb_adt_set_ptr(klb_adt_t* p_adt, const void* ptr1, const void* ptr2)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_ptr;
    p_adt->ptr1 = (void*)ptr1;
    p_adt->ptr2 = (void*)ptr2;
}

void klb_adt_set_double(klb_adt_t* p_adt, double d)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_double;
    p_adt->d = d;
}

void klb_adt_set_uint64(klb_adt_t* p_adt, uint64_t u64)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_uint64;
    p_adt->u64 = u64;
}

void klb_adt_set_int64(klb_adt_t* p_adt, int64_t i64)
{
    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_int64;
    p_adt->i64 = i64;
}

void klb_adt_set_map(klb_adt_t* p_adt, klb_map_t* ptr)
{
    klb_map_t* p_tmp = (NULL != ptr) ? ptr : klb_map_create();

    klb_adt_quit(p_adt);

    p_adt->type = KLB_ADT_map;
    p_adt->p_map = p_tmp; // ptr 交给 map 托管
}

void klb_adt_set_map_clone(klb_adt_t* p_adt, const klb_map_t* p_src)
{
    klb_map_t* p_tmp = klb_map_create();
    klb_map_copy(p_tmp, p_src);

    klb_adt_set_map(p_adt, p_tmp);
}

klb_adt_type_e klb_adt_type(klb_adt_t* p_adt)
{
    return (klb_adt_type_e)p_adt->type;
}

bool klb_adt_to_bool(klb_adt_t* p_adt)
{
    return (KLB_ADT_bool == p_adt->type) ? p_adt->b : false;
}

const char* klb_adt_to_string(klb_adt_t* p_adt)
{
    return (KLB_ADT_string == p_adt->type) ? p_adt->str : "";
}

const char* klb_adt_to_lstring(klb_adt_t* p_adt, int* p_out_len)
{
    if (KLB_ADT_string == p_adt->type)
    {
        if (NULL != p_out_len)
        {
            *p_out_len = sdslen(p_adt->str);
        }

        return p_adt->str;
    }

    if (NULL != p_out_len)
    {
        *p_out_len = 0;
    }
    return "";
}

const void* klb_adt_to_ptr(klb_adt_t* p_adt, const void** p_out_ptr2)
{
    if (KLB_ADT_ptr == p_adt->type)
    {
        if (NULL != p_out_ptr2)
        {
            *p_out_ptr2 = p_adt->ptr2;
        }
        return p_adt->ptr1;
    }

    if (NULL != p_out_ptr2)
    {
        *p_out_ptr2 = NULL;
    }
    return NULL;
}

double klb_adt_to_double(klb_adt_t* p_adt)
{
    return (KLB_ADT_double == p_adt->type) ? p_adt->d : 0.0;
}

uint64_t klb_adt_to_uint64(klb_adt_t* p_adt)
{
    return (KLB_ADT_uint64 == p_adt->type) ? p_adt->u64 : 0;
}

int64_t klb_adt_to_int64(klb_adt_t* p_adt)
{
    return (KLB_ADT_int64 == p_adt->type) ? p_adt->i64 : 0;
}

klb_map_t* klb_adt_to_map(klb_adt_t* p_adt)
{
    return (KLB_ADT_map == p_adt->type) ? p_adt->p_map : NULL;
}

bool klb_adt_get_bool(klb_adt_t* p_adt, bool default_b)
{
    return (KLB_ADT_bool == p_adt->type) ? p_adt->b : default_b;
}

const char* klb_adt_get_string(klb_adt_t* p_adt, const char* p_default_str)
{
    return (KLB_ADT_string == p_adt->type) ? p_adt->str : p_default_str;
}

const char* klb_adt_get_lstring(klb_adt_t* p_adt, const char* p_default_str, int* p_in_out_len)
{
    if (KLB_ADT_string == p_adt->type)
    {
        if (NULL != p_in_out_len)
        {
            *p_in_out_len = sdslen(p_adt->str);
        }

        return p_adt->str;
    }

    return p_default_str;
}

double klb_adt_get_double(klb_adt_t* p_adt, double default_d)
{
    return (KLB_ADT_double == p_adt->type) ? p_adt->d : default_d;
}

uint64_t klb_adt_get_uint64(klb_adt_t* p_adt, uint64_t default_u64)
{
    return (KLB_ADT_uint64 == p_adt->type) ? p_adt->u64 : default_u64;
}

int64_t klb_adt_get_int64(klb_adt_t* p_adt, int64_t default_i64)
{
    return (KLB_ADT_int64 == p_adt->type) ? p_adt->i64 : default_i64;
}

void klb_adt_copy(klb_adt_t* p_dst, const klb_adt_t* p_src)
{
    if (NULL != p_src)
    {
        switch (p_src->type)
        {
        case KLB_ADT_null:
            klb_adt_set_null(p_dst);
            break;
        case KLB_ADT_bool:
            klb_adt_set_bool(p_dst, p_src->b);
            break;
        case KLB_ADT_string:
            klb_adt_set_lstring(p_dst, p_src->str, sdslen(p_src->str));
            break;
        case KLB_ADT_ptr:
            klb_adt_set_ptr(p_dst, p_src->ptr1, p_src->ptr2);
            break;
        case KLB_ADT_double:
            klb_adt_set_double(p_dst, p_src->d);
            break;
        case KLB_ADT_uint64:
            klb_adt_set_uint64(p_dst, p_src->u64);
            break;
        case KLB_ADT_int64:
            klb_adt_set_int64(p_dst, p_src->i64);
            break;
        case KLB_ADT_map:
            {
                klb_map_t* p_map = klb_map_create();
                klb_map_copy(p_map, p_src->p_map);

                klb_adt_set_map(p_dst, p_map);
            }
            break;
        default:
            assert(false);
            break;
        }
    }
    else
    {
        klb_adt_quit(p_dst);
    }
}
