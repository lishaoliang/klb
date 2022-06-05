///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buffer.h
/// @brief   通用buffer处理: 提供分块写入, 通过 "join" 函数最终一次合并成一块内存
///          与 realloc 函数相比, 省去中间多次申请释放拷贝的开销
///          接口仿: https://studygolang.com/pkgdoc
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
    size_t      block_num;

    size_t      total_buf_len;
    size_t      total_data_len;
}klb_buffer_t;


/// @brief 创建缓存
/// @param [in]  suggest_len        初始缓存建议长度(最小4K)
/// @return klb_buffer_t* 缓存指针
/// @note 接口参考 https://studygolang.com/pkgdoc bytes/type Buffer
KLB_API klb_buffer_t* klb_buffer_create(int suggest_len);


/// @brief 销毁缓存
/// @param [in]  *p_buffer          缓存指针
/// @return 无
KLB_API void klb_buffer_destroy(klb_buffer_t* p_buffer);


/// @brief 重置缓存
/// @param [in]  *p_buffer          缓存指针
/// @return 无
KLB_API void klb_buffer_reset(klb_buffer_t* p_buffer);


/// @brief 向缓存写入数据
/// @param [in]  *p_buffer          缓存指针
/// @param [in]  *p_data            待写入数据
/// @param [in]  data_len           数据长度
/// @return bool 返回值
KLB_API int klb_buffer_write(klb_buffer_t* p_buffer, const char* p_data, int data_len);


/// @brief 当前有效数据长度
/// @param [in]  *p_buffer          缓存指针
/// @return int 有效数据长度
KLB_API int klb_buffer_data_len(klb_buffer_t* p_buffer);


/// @brief 将有效数据拼接在一起
/// @param [in]  *p_buffer          缓存指针
/// @return klb_buf_t* KLB_BUF_EXTRA类型缓存; [klb_buf_t][data...]
KLB_API klb_buf_t* klb_buffer_join(klb_buffer_t* p_buffer, klb_buf_malloc_cb cb_malloc, void* p_pool);


/// @brief 将有效数据拼接在一起
/// @param [in]  *p_buffer          缓存指针
/// @param [in]  *offset_x          起始偏移
/// @param [in]  *offset_y          末尾偏移
/// @return klb_buf_t* KLB_BUF_EXTRA类型缓存; [klb_buf_t][data...]
KLB_API klb_buf_t* klb_buffer_join_offset(klb_buffer_t* p_buffer, size_t offset_x, size_t offset_y, klb_buf_malloc_cb cb_malloc, void* p_pool);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUFFER_H__
//end
