// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_buf.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_fpool.h"
#include "klbmem/klb_buf_atom.h"
#include <assert.h>


klb_buf_t* klb_buf_malloc(int buf_len, bool zero)
{
    assert(0 <= buf_len);

    klb_buf_t* p_buf = (klb_buf_t*)KLB_MALLOC(char, sizeof(klb_buf_t) + buf_len, 0);
    KLB_MEMSET(p_buf, 0, sizeof(klb_buf_t));

    p_buf->p_buf = (char*)p_buf + sizeof(klb_buf_t);
    p_buf->buf_len = buf_len;
    p_buf->type = KLB_BUF_EXTRA;

    if (zero && 0 < p_buf->buf_len)
    {
        KLB_MEMSET(p_buf->p_buf, 0, p_buf->buf_len);
    }

    return p_buf;
}

klb_buf_t* klb_buf_join(klb_buf_t* p_head)
{
    return NULL;
}

int klb_buf_ref(klb_buf_t* p_buf)
{
    switch (p_buf->type)
    {
    case KLB_BUF_ATOM:
        return klb_buf_atom_ref(p_buf);
        break;
    case KLB_BUF_FIX_POOL:
        return klb_fpool_ref(p_buf);
        break;
    default:
        assert(false);
        break;
    }

    return 0;
}

int klb_buf_unref(klb_buf_t* p_buf)
{
    switch (p_buf->type)
    {
    case KLB_BUF_NORMAL:
        KLB_FREE(p_buf->p_buf);
        KLB_FREE(p_buf);
        break;
    case KLB_BUF_EXTRA:
        KLB_FREE(p_buf);
        break;
    case KLB_BUF_ATOM:
        return klb_buf_atom_unref(p_buf);
        break;
    case KLB_BUF_FIX_POOL:
        return klb_fpool_unref(p_buf);
        break;
    default:
        assert(false);
        break;
    }

    return 0;
}

int klb_buf_ref_next(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);

    klb_buf_t* p_cur = p_buf;
    while (NULL != p_cur)
    {
        klb_buf_t* p_next = p_cur->p_next;

        klb_buf_ref(p_cur);

        // next
        p_cur = p_next;
    }

    return 0;
}

int klb_buf_unref_next(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);

    klb_buf_t* p_cur = p_buf;
    while (NULL != p_cur)
    {
        klb_buf_t* p_next = p_cur->p_next;

        klb_buf_unref(p_cur);

        // next
        p_cur = p_next;
    }

    return 0;
}
