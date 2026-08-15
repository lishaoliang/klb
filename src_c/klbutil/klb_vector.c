#include "klbutil/klb_vector.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <limits.h>


void klb_vector_init(klb_vector_t* p_vector)
{
    p_vector->p_nvector = klb_nvector_create();
}

void klb_vector_quit(klb_vector_t* p_vector)
{
    klb_vector_clear(p_vector);
    KLB_FREE_BY(p_vector->p_nvector, klb_nvector_destroy);
}

klb_vector_t* klb_vector_create()
{
    klb_vector_t* ptr = KLB_MALLOCZ(klb_vector_t, 1, 0);
    klb_vector_init(ptr);
    return ptr;
}

void klb_vector_destroy(klb_vector_t* p_vector)
{
    klb_vector_quit(p_vector);
    KLB_FREE(p_vector);
}

void klb_vector_append_null(klb_vector_t* p_vector)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_nvector_push_tail(p_vector->p_nvector, p_adt);
}

void klb_vector_append_int64(klb_vector_t* p_vector, int64_t i64)
{
    klb_adt_t* p_adt = klb_adt_create();
    klb_adt_set_int64(p_adt, i64);
    klb_nvector_push_tail(p_vector->p_nvector, p_adt);
}

static void check_idx_klb_vector(klb_vector_t* p_vector, int idx)
{
    if (idx < 0 || INT_MAX <= idx) return; // idx+1 溢出

    int u_size = idx + 1;
    int size = klb_nvector_size(p_vector->p_nvector);

    if (size < u_size)
    {
        int count = u_size - size;

        while (0 < count)
        {
            klb_vector_append_null(p_vector);
            count--;
        }
    }
}

void klb_vector_set_int64(klb_vector_t* p_vector, int idx, int64_t i64)
{
    if (idx < 0)
    {
        klb_vector_append_int64(p_vector, i64);
    }
    else
    {
        check_idx_klb_vector(p_vector, idx);
        assert(idx < klb_nvector_size(p_vector->p_nvector));

        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_vector->p_nvector, idx);
        assert(NULL != p_adt);
        klb_adt_set_int64(p_adt, i64);
    }
}

static klb_adt_t* find_adt_klb_vector(klb_vector_t* p_vector, int idx)
{
    assert(NULL != p_vector);

    if (0 <= idx && idx < klb_nvector_size(p_vector->p_nvector))
    {
        klb_adt_t* p_adt = (klb_adt_t*)klb_nvector_get(p_vector->p_nvector, idx);
        assert(NULL != p_adt);

        return p_adt;
    }

    return NULL;
}

int klb_vector_type(klb_vector_t* p_vector, int idx)
{
    klb_adt_t* p_adt = find_adt_klb_vector(p_vector, idx);
    if (NULL != p_adt)
    {
        return klb_adt_type(p_adt);
    }

    return KLB_ADT_null;
}

int64_t klb_vector_to_int64(klb_vector_t* p_vector, int idx)
{
    return 0;
}

const klb_adt_t* klb_vector_to_adt(klb_vector_t* p_vector, int idx)
{
    return NULL;
}

bool klb_vector_remove(klb_vector_t* p_vector, int idx)
{
    return false;
}

bool klb_vector_remove_tail(klb_vector_t* p_vector)
{
    return false;
}

int klb_vector_size(klb_vector_t* p_vector)
{
    return klb_nvector_size(p_vector->p_nvector);
}

static int cb_clean_klb_vector(void* p_obj, void* p_data)
{
    klb_vector_t* p_vector = (klb_vector_t*)p_obj;
    klb_adt_t* p_adt = (klb_adt_t*)p_data;
    assert(NULL != p_adt);

    klb_adt_quit(p_adt);
    KLB_FREE(p_adt);

    return 0;
}

static void clean_klb_vector(klb_vector_t* p_vector)
{
    // 清空 vector
    klb_nvector_clear(p_vector->p_nvector, cb_clean_klb_vector, p_vector);
}

void klb_vector_clear(klb_vector_t* p_vector)
{
    clean_klb_vector(p_vector);
}

//end
