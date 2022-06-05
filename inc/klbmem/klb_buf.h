///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buf.h
/// @brief   简易缓存
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUF_H__
#define __KLB_BUF_H__

#include "klb_type.h"
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(4)


/// @struct klb_buf_t
/// @brief  缓存类型
typedef enum klb_buf_type_e_
{
    KLB_BUF_NORMAL      = 0,        ///< 常规缓存; 直接使用 malloc/free 分配释放
    KLB_BUF_EXTRA       = 1,        ///< 常规缓存; klb_buf_t + [data]分配在一起

    KLB_BUF_ATOM        = 10,       ///< 带原子变量(引用计数)的缓存
    KLB_BUF_FIX_POOL    = 11,       ///< 固定大小内存池分配: klb_fpool_t
}klb_buf_type_e;


/// @struct klb_buf_t
/// @brief  简易缓存
typedef struct klb_buf_t_
{
    char*   p_buf;              ///< 缓存指针
    int     buf_len;            ///< 缓存大小

    int     start;              ///< 有效数据起始位置
    int     end;                ///< 有效数据末尾

    int     type;               ///< 缓存类型: klb_buf_type_e

    struct klb_buf_t_* p_next;  ///< 下一个节点

    char    extra[];            ///< 附加数据
}klb_buf_t;

#pragma pack()


/// @brief 缓存池获取缓存的 函数指针
typedef klb_buf_t*(*klb_buf_malloc_cb)(void* p_pool, size_t size);


/// @brief 初始化缓存: 类型KLB_BUF_EXTRA
/// @param [in] buf_len         目标缓存长度
/// @param [in] zero            是否初始化
/// @return klb_buf_t* p_buf
/// @note 直接通过 free 释放
KLB_API klb_buf_t* klb_buf_malloc(int buf_len, bool zero);


/// @brief 将有效数据拼接在一起
/// @param [in]  *p_head        缓存头指针
/// @return klb_buf_t* KLB_BUF_EXTRA类型缓存; [klb_buf_t][data...]
KLB_API klb_buf_t* klb_buf_join(klb_buf_t* p_head);


/// @brief 引用加一
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_ref(klb_buf_t* p_buf);


/// @brief 引用减一
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_unref(klb_buf_t* p_buf);


/// @brief 引用加一: 当前节点及后续节点
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_ref_next(klb_buf_t* p_buf);


/// @brief 引用减一: 当前节点及后续节点
/// @param [in]  *p_buf         目标缓存
/// @return int 
KLB_API int klb_buf_unref_next(klb_buf_t* p_buf);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUF_H__
//end
