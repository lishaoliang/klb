// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_buffer.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include <assert.h>


#define KLB_BUFFER_4K       4096
#define KLB_BUFFER_1M       1048576


static int auto_suggest_len_klb_buffer(int data_len)
{
    static int s_size[8] = {1024*4,   1024*8,   1024*16,  1024*32,  1024*64, 
                            1024*128, 1024*512, 1024*1024};

    for (int i = 0; i < sizeof(s_size) / sizeof(s_size[0]); i++)
    {
        if (data_len < s_size[i])
        {
            return s_size[i];
        }
    }
    
    return ((data_len + KLB_BUFFER_1M - 1) / KLB_BUFFER_1M) * KLB_BUFFER_1M;
}

klb_buffer_t* klb_buffer_create(int suggest_len)
{
    int buf_len = auto_suggest_len_klb_buffer(suggest_len);

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
    p_buffer->p_head->start = 0;
    p_buffer->p_head->end = 0;

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
    if (p_buffer->total_buf_len <= p_buffer->total_data_len + data_len)
    {
        klb_buffer_expand(p_buffer, auto_suggest_len_klb_buffer(data_len));
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
                klb_buffer_expand(p_buffer, auto_suggest_len_klb_buffer(src_len));
            }

            p_buffer->p_write = p_buffer->p_write->p_next;
            assert(NULL != p_buffer->p_write);
        }
    }

    p_buffer->total_data_len += data_len;

    return 0;
}

int klb_buffer_data_len(klb_buffer_t* p_buffer)
{
    return p_buffer->total_data_len;
}

static klb_buf_t* klb_buffer_malloc(void* p_pool, size_t size)
{
    return klb_buf_malloc(size, false);
}

klb_buf_t* klb_buffer_join(klb_buffer_t* p_buffer, klb_buf_malloc_cb cb_malloc, void* p_pool)
{
    klb_buf_malloc_cb real_malloc = (NULL != cb_malloc) ? cb_malloc : klb_buffer_malloc;

    int buf_len = KLB_ALIGNED_4(p_buffer->total_data_len);
    klb_buf_t* p_buf = real_malloc(p_pool, buf_len);
    assert(NULL != p_buf);

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

klb_buf_t* klb_buffer_join_offset(klb_buffer_t* p_buffer, size_t offset_x, size_t offset_y, klb_buf_malloc_cb cb_malloc, void* p_pool)
{
    klb_buf_malloc_cb real_malloc = (NULL != cb_malloc) ? cb_malloc : klb_buffer_malloc;

    size_t total_len = p_buffer->total_data_len + offset_x + offset_y;
    size_t buf_len = KLB_ALIGNED_4(total_len);
    klb_buf_t* p_buf = real_malloc(p_pool, buf_len);
    assert(NULL != p_buf);

    klb_buf_t* ptr = p_buffer->p_head;
    p_buf->start = offset_x;
    p_buf->end = offset_x;

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

    assert(p_buf->end + offset_y <= p_buf->buf_len);
    return p_buf;
}
