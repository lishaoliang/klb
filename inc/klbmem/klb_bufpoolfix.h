///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_bufpoolfix.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb buffer pool fix 固定大小缓存池
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUFPOOLFIX_H__
#define __KLB_BUFPOOLFIX_H__


#include "klb_type.h"
#include "klbmem/klb_buf.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_bufpoolfix_t
/// @brief  固定大小缓存池
typedef struct klb_bufpoolfix_t_ klb_bufpoolfix_t;


/// @brief 创建 缓存池
/// @return klb_bufpoolfix_t* 缓存池
KLB_API klb_bufpoolfix_t* klb_bufpoolfix_create();


/// @brief 销毁 缓存池
/// @param [in]  *p_bufpool          缓存池 指针
/// @return 无
KLB_API void klb_bufpoolfix_destroy(klb_bufpoolfix_t* p_bufpool);


/// @brief 申请内存
KLB_API klb_buf_t* klb_bufpoolfix_malloc(void* p_bufpool, size_t size);



#ifdef __cplusplus
}
#endif

#endif // __KLB_BUFPOOLFIX_H__
//end
