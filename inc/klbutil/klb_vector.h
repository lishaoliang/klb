///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_vector.h
/// @brief   仿std::vector
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_VECTOR_H__
#define __KLB_VECTOR_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_vector_t_
/// @brief  vector对象
typedef struct klb_vector_t_ klb_vector_t;


/// @brief 创建vector对象
/// @return klb_vector_t*       vector对象
KLB_API klb_vector_t* klb_vector_create();


/// @brief 销毁vector对象
/// @param [in] *p_vector       vector对象
/// @return 无
/// @note 销毁前必须自行清空, 否则内存泄露
KLB_API void klb_vector_destroy(klb_vector_t* p_vector);


/// @brief 清空列表回调函数
/// @param [in] *p_obj      传参对象
/// @param [in] *p_data     数据
/// @return int 0
typedef int(*klb_vector_clean_cb)(void* p_obj, void* p_data);


/// @brief 清空列表
/// @param [in] *p_vector   vector对象
/// @param [in] cb_clean    清理回调函数
/// @param [in] *p_obj      传参对象
/// @return 无
/// @note 方向: 从头到尾
KLB_API void klb_vector_clean(klb_vector_t* p_vector, klb_vector_clean_cb cb_clean, void* p_obj);


/// @brief 在最前面, 向vector压入数据
/// @param [in] *p_vector   vector对象
/// @param [in] *p_data     数据
/// @return 无
/// @note vector并不负责数据释放
KLB_API void klb_vector_push_head(klb_vector_t* p_vector, void* p_data);


/// @brief 在最后面, 向vector压入数据
/// @param [in] *p_vector   vector对象
/// @param [in] *p_data     数据
/// @return 无
KLB_API void klb_vector_push_tail(klb_vector_t* p_vector, void* p_data);


/// @brief 在最前面, 弹出数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_pop_head(klb_vector_t* p_vector);


/// @brief 在最后面, 弹出数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_pop_tail(klb_vector_t* p_vector);


/// @brief 访问最前面的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_head(klb_vector_t* p_vector);


/// @brief 访问最后面的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_tail(klb_vector_t* p_vector);


/// @brief 访问第index的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_get(klb_vector_t* p_vector, int index);


/// @brief 移除第index的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_vector_remove(klb_vector_t* p_vector, int index);


/// @brief 当前节点数目
/// @param [in] *p_vector   vector对象
/// @return uint32_t 返回当前节点数目
KLB_API uint32_t klb_vector_size(klb_vector_t* p_vector);


#ifdef __cplusplus
}
#endif

#endif // __KLB_VECTOR_H__
//end
