///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_vector.h
/// @brief   vector
///  \n 参考: std::vector
///  \n 插入的数据托管内存
/// @version 0.1
/// @history 修改历史
///  \n 2022 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_VECTOR_H__
#define __KLB_VECTOR_H__

#include "klb_type.h"
#include "klbutil/klb_adt.h"
#include "klbutil/klb_nvector.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_vector_t
/// @brief  vector
typedef struct klb_vector_t_
{
    klb_nvector_t*  p_nvector;
}klb_vector_t;


KLB_API void klb_vector_init(klb_vector_t* p_vector);
KLB_API void klb_vector_quit(klb_vector_t* p_vector);

KLB_API klb_vector_t* klb_vector_create();
KLB_API void klb_vector_destroy(klb_vector_t* p_vector);

KLB_API void klb_vector_append_null(klb_vector_t* p_vector);
KLB_API void klb_vector_append_int64(klb_vector_t* p_vector, int64_t i64);

KLB_API void klb_vector_set_int64(klb_vector_t* p_vector, int idx, int64_t i64);

KLB_API int klb_vector_type(klb_vector_t* p_vector, int idx);

KLB_API int64_t klb_vector_to_int64(klb_vector_t* p_vector, int idx);
KLB_API const klb_adt_t* klb_vector_to_adt(klb_vector_t* p_vector, int idx);

KLB_API bool klb_vector_remove(klb_vector_t* p_vector, int idx);
KLB_API bool klb_vector_remove_tail(klb_vector_t* p_vector);

KLB_API int klb_vector_size(klb_vector_t* p_vector);
KLB_API void klb_vector_clean(klb_vector_t* p_vector);


#ifdef __cplusplus
}
#endif

#endif // __KLB_VECTOR_H__
//end
