///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buffer.h
/// @brief   通用buffer处理
///          接口仿: https://studygolang.com/pkgdoc
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUFFER_H__
#define __KLB_BUFFER_H__

#include "klb_type.h"
#include "mem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_buffer_t_
{
    klb_buf_t*  p_head;
    klb_buf_t*  p_tail;
    klb_buf_t*  p_write;
    int         block_num;

    int64_t     total_buf_len;
    int64_t     total_data_len;
}klb_buffer_t;


klb_buffer_t* klb_buffer_create(int suggest_len);
void klb_buffer_destroy(klb_buffer_t* p_buffer);

void klb_buffer_reset(klb_buffer_t* p_buffer);

int klb_buffer_write(klb_buffer_t* p_buffer, const char* p_data, int data_len);

int64_t klb_buffer_data_len(klb_buffer_t* p_buffer);

klb_buf_t* klb_buffer_join(klb_buffer_t* p_buffer);

#ifdef __cplusplus
}
#endif

#endif // __KLB_BUFFER_H__
//end
