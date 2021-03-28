#include "klbmem/klb_buffer.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>

klb_buffer_t* klb_buffer_create(int suggest_len)
{
    int buf_len = KLB_ALIGNED_4(suggest_len);

    klb_buffer_t* p_buffer = KLB_MALLOC(klb_buffer_t, 1, 0);
    KLB_MEMSET(p_buffer, 0, sizeof(klb_buffer_t));

    p_buffer->p_head = klb_buf_malloc(buf_len, false);
    p_buffer->p_tail = p_buffer->p_head;
    p_buffer->p_write = p_buffer->p_head;
    p_buffer->block_num = 1;
    p_buffer->total_buf_len = buf_len;

    return p_buffer;
}

void klb_buffer_destroy(klb_buffer_t* p_buffer)
{
    assert(NULL != p_buffer);

    klb_buf_t* p_next = p_buffer->p_head;
    while (NULL != p_next)
    {
        klb_buf_t* p_cur = p_next;
        p_next = p_next->p_next;

        assert(KLB_BUF_EXTRA == p_cur->type);
        KLB_FREE(p_cur);
    }

    KLB_FREE(p_buffer);
}

void klb_buffer_reset(klb_buffer_t* p_buffer)
{
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->p_head);

    klb_buf_t* p_next = p_buffer->p_head->p_next;
    while (NULL != p_next)
    {
        klb_buf_t* p_cur = p_next;
        p_next = p_next->p_next;

        assert(KLB_BUF_EXTRA == p_cur->type);
        KLB_FREE(p_cur);
    }

    p_buffer->p_head->p_next = NULL;
    p_buffer->p_tail = p_buffer->p_head;
    p_buffer->p_write = p_buffer->p_head;
    p_buffer->block_num = 1;
    p_buffer->total_buf_len = p_buffer->p_head->buf_len;
    p_buffer->total_data_len = 0;
}

static void klb_buffer_expand(klb_buffer_t* p_buffer, int suggest_len)
{
    int expand_len = KLB_ALIGNED_4(suggest_len);
    klb_buf_t* p_buf = klb_buf_malloc(expand_len, false);

    p_buffer->p_tail->p_next = p_buf;
    p_buffer->p_tail = p_buf;

    p_buffer->block_num += 1;
    p_buffer->total_buf_len += expand_len;
}

int klb_buffer_write(klb_buffer_t* p_buffer, const char* p_data, int data_len)
{
    assert(NULL != p_buffer);
    //KLB_LOG("data_len=%d\n", data_len);
    if (p_buffer->total_buf_len <= p_buffer->total_data_len + data_len)
    {
        klb_buffer_expand(p_buffer, data_len);
    }

    char* p_src = (char*)p_data;
    int src_len = data_len;
    while (0 < src_len)
    {
        klb_buf_t* p_write = p_buffer->p_write;

        int w_len = MIN(src_len, p_write->buf_len - p_write->end);
        memcpy(p_write->p_buf + p_write->end, p_src, w_len);

        src_len -= w_len;
        p_src += w_len;
        p_write->end += w_len;

        if (p_write->buf_len <= p_write->end)
        {
            if (NULL == p_write->p_next)
            {
                klb_buffer_expand(p_buffer, 16 * 1024);
            }

            p_buffer->p_write = p_buffer->p_write->p_next;
            assert(NULL != p_buffer->p_write);
        }
    }

    p_buffer->total_data_len += data_len;

    return 0;
}

int64_t klb_buffer_data_len(klb_buffer_t* p_buffer)
{
    return p_buffer->total_data_len;
}

klb_buf_t* klb_buffer_join(klb_buffer_t* p_buffer)
{
    int64_t buf_len = KLB_ALIGNED_4(p_buffer->total_data_len);
    klb_buf_t* p_buf = klb_buf_malloc(buf_len, false);

    klb_buf_t* ptr = p_buffer->p_head;
    while (NULL != ptr)
    {
        int data_len = ptr->end - ptr->start;
        if (0 < data_len)
        {
            memcpy(p_buf->p_buf + p_buf->end, ptr->p_buf + ptr->start, data_len);
            p_buf->end += data_len;
        }

        ptr = ptr->p_next;
    }

    assert(p_buf->end <= p_buf->buf_len);
    assert(p_buf->end == p_buffer->total_data_len);
    return p_buf;
}
