///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_buf_atom.h
/// @brief   带原子变量(引用计数)的缓存
///  直接使用malloc/free, 带引用计数;
///  适用于 单生产者, 多消费者场景
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BUF_ATOM_H__
#define __KLB_BUF_ATOM_H__

#include "klb_type.h"
#include "klbmem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif


KLB_API klb_buf_t* klb_buf_atom_malloc(void* p_pool, size_t size);


KLB_API int klb_buf_atom_ref(klb_buf_t* p_buf);
KLB_API int klb_buf_atom_unref(klb_buf_t* p_buf);


#ifdef __cplusplus
}
#endif

#endif // __KLB_BUF_ATOM_H__
//end
