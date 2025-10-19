// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_buf_atom.h"
#include "klbplatform/klb_atomic.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#pragma pack(4)

typedef struct klb_buf_atom_t_
{
    klb_buf_t               buf;
    klb_atomic_t volatile   atomic_count; ///< klb_buf_t 使用计数
}klb_buf_atom_t;

#pragma pack()


klb_buf_t* klb_buf_atom_malloc(void* p_pool, size_t size)
{
    assert(NULL == p_pool);

    klb_buf_atom_t* p_buf_ex = KLB_MALLOC(klb_buf_atom_t, 1, size);
    memset(p_buf_ex, 0, sizeof(klb_buf_atom_t));

    p_buf_ex->buf.p_buf = (char*)p_buf_ex + sizeof(klb_buf_atom_t);
    p_buf_ex->buf.buf_len = size;
    p_buf_ex->buf.type = KLB_BUF_ATOM;

    klb_atomic_set_value(&p_buf_ex->atomic_count, 1);

    return (klb_buf_t*)p_buf_ex;
}

klb_buf_t* klb_buf_atom_malloc2(size_t size)
{
    return klb_buf_atom_malloc(NULL, size);
}

int klb_buf_atom_ref(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);
    assert(KLB_BUF_ATOM == p_buf->type);

    klb_buf_atom_t* p_buf_ex = (klb_buf_atom_t*)p_buf;

    int n = klb_atomic_add(&p_buf_ex->atomic_count);

    return n + 1;
}

int klb_buf_atom_unref(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);
    assert(KLB_BUF_ATOM == p_buf->type);

    klb_buf_atom_t* p_buf_ex = (klb_buf_atom_t*)p_buf;

    int n = klb_atomic_sub(&p_buf_ex->atomic_count);

    if (n <= 1)
    {
        assert(1 == n);

        // 释放
        KLB_MEMSET(p_buf_ex, 0, sizeof(klb_buf_atom_t));
        KLB_FREE(p_buf);
    }

    return n - 1;
}
