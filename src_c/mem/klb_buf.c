///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buf.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   简易缓存
///////////////////////////////////////////////////////////////////////////
#include "mem/klb_buf.h"
#include "mem/klb_mem.h"
#include <assert.h>


klb_buf_t* klb_buf_malloc(int buf_len, bool zero)
{
    assert(0 <= buf_len);

    klb_buf_t* p_buf = (klb_buf_t*)KLB_MALLOC(char, sizeof(klb_buf_t) + buf_len, 0);
    KLB_MEMSET(p_buf, 0, sizeof(klb_buf_t));

    p_buf->p_buf = p_buf->extra;
    p_buf->buf_len = buf_len;
    p_buf->type = KLB_BUF_EXTRA;

    if (zero && 0 < p_buf->buf_len)
    {
        KLB_MEMSET(p_buf->p_buf, 0, p_buf->buf_len);
    }

    return p_buf;
}

klb_buf_t* klb_buf_init(klb_buf_t* p_buf, int buf_len, const char* p_data, int data_len)
{
    assert(NULL != p_buf);
    assert(data_len <= buf_len);

    p_buf->p_buf = KLB_MALLOC(char, buf_len, 0);
    p_buf->buf_len = buf_len;
    p_buf->start = 0;
    p_buf->end = 0;
    p_buf->type = KLB_BUF_NORMAL;
    p_buf->p_next = NULL;

    if (NULL != p_data && 0 < data_len)
    {
        memcpy(p_buf->p_buf, p_data, data_len);
        p_buf->end = data_len;
    }

    return p_buf;
}

void klb_buf_quit(klb_buf_t* p_buf)
{
    assert(NULL != p_buf);

    KLB_FREE(p_buf->p_buf);
    p_buf->buf_len = 0;
    p_buf->start = 0;
    p_buf->end = 0;
}
