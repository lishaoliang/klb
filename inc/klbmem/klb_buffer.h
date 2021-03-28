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
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_buffer_t_
{
    klb_buf_t*  p_head;
    klb_buf_t*  p_tail;
    klb_buf_t*  p_write;
    int         block_num;

    int         total_buf_len;
    int         total_data_len;
}klb_buffer_t;


/// @brief 创建缓存
/// @param [in]  suggest_len        初始缓存建议长度(最小4K)
/// @return klb_buffer_t* 缓存指针
/// @note 接口参考 https://studygolang.com/pkgdoc bytes/type Buffer
klb_buffer_t* klb_buffer_create(int suggest_len);


/// @brief 销毁缓存
/// @param [in]  *p_buffer          缓存指针
/// @return 无
void klb_buffer_destroy(klb_buffer_t* p_buffer);


/// @brief 重置缓存
/// @param [in]  *p_buffer          缓存指针
/// @return 无
void klb_buffer_reset(klb_buffer_t* p_buffer);


/// @brief 向缓存写入数据
/// @param [in]  *p_buffer          缓存指针
/// @param [in]  *p_data            待写入数据
/// @param [in]  data_len           数据长度
/// @return bool 返回值
int klb_buffer_write(klb_buffer_t* p_buffer, const char* p_data, int data_len);


/// @brief 当前有效数据长度
/// @param [in]  *p_buffer          缓存指针
/// @return int 有效数据长度
int klb_buffer_data_len(klb_buffer_t* p_buffer);


/// @brief 将有效数据拼接在一起
/// @param [in]  *p_buffer          缓存指针
/// @return klb_buf_t* KLB_BUF_EXTRA类型缓存; [klb_buf_t][data...]
klb_buf_t* klb_buffer_join(klb_buffer_t* p_buffer);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUFFER_H__
//end
