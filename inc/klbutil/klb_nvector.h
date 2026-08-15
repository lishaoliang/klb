///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_nvector.h
/// @brief   仿std::vector
///  \n 放入的内存由调用者处理
/// @version 0.1
/// @history 修改历史
///  \n 2021 0.1 创建文件
///  \n 2022 0.2 从 klb_vector_t 改名为 klb_nvector_t
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NVECTOR_H__
#define __KLB_NVECTOR_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_nvector_t
/// @brief  vector对象
typedef struct klb_nvector_t_ klb_nvector_t;


/// @brief 创建vector对象
/// @return klb_nvector_t*       vector对象
KLB_API klb_nvector_t* klb_nvector_create();


/// @brief 销毁vector对象
/// @param [in] *p_vector       vector对象
/// @return 无
/// @note 销毁前必须自行清空, 否则内存泄露
KLB_API void klb_nvector_destroy(klb_nvector_t* p_vector);


/// @brief 清空列表回调函数
/// @param [in] *p_obj      传参对象
/// @param [in] *p_data     数据
/// @return int 0
typedef int(*klb_nvector_clear_cb)(void* p_obj, void* p_data);


/// @brief 清空列表
/// @param [in] *p_vector   vector对象
/// @param [in] cb_clear    清理回调函数
/// @param [in] *p_obj      传参对象
/// @return 无
/// @note 方向: 从头到尾
KLB_API void klb_nvector_clear(klb_nvector_t* p_vector, klb_nvector_clear_cb cb_clear, void* p_obj);


/// @brief 在最前面, 向vector压入数据
/// @param [in] *p_vector   vector对象
/// @param [in] *p_data     数据
/// @return 无
/// @note vector并不负责数据释放
//KLB_API void klb_nvector_push_head(klb_nvector_t* p_vector, void* p_data);


/// @brief 在最后面, 向vector压入数据
/// @param [in] *p_vector   vector对象
/// @param [in] *p_data     数据
/// @return int >=0.成功, 返回新元素下标; -1.失败(已达最大容量, 无法继续扩容)
/// @note vector并不负责数据释放
KLB_API int klb_nvector_push_tail(klb_nvector_t* p_vector, void* p_data);


/// @brief 在最前面, 弹出数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_pop_head(klb_nvector_t* p_vector);


/// @brief 在最后面, 弹出数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_pop_tail(klb_nvector_t* p_vector);


/// @brief 访问最前面的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_head(klb_nvector_t* p_vector);


/// @brief 访问最后面的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_tail(klb_nvector_t* p_vector);


/// @brief 访问第index的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_get(klb_nvector_t* p_vector, int index);


/// @brief 更换第index的数据: 必须已存在
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_update(klb_nvector_t* p_vector, int index, void* p_data);


/// @brief 移除第index的数据
/// @param [in] *p_vector   vector对象
/// @return void* 数据指针
KLB_API void* klb_nvector_remove(klb_nvector_t* p_vector, int index);


/// @brief 当前节点数目
/// @param [in] *p_vector   vector对象
/// @return int 返回当前节点数目
KLB_API int klb_nvector_size(klb_nvector_t* p_vector);


typedef int(*klb_nvector_sort_cb)(const void* p_data1, const void* p_data2, void* ptr1, void* ptr2);

/// @brief 排序
KLB_API int klb_nvector_sort(klb_nvector_t* p_vector, klb_nvector_sort_cb cb_sort, void* ptr1, void* ptr2);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NVECTOR_H__
//end
