///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buf.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   简易缓存
///////////////////////////////////////////////////////////////////////////
#include "klbmem/klb_buf.h"
#include "klbmem/klb_mem.h"
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
