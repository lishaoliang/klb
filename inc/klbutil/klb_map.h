///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_map.h
/// @brief   按(K:V)存储, key仅适用字符串
///     鉴于 klb_hlist_t 已经处理简单(K:V)键值对
///     klb_map_t 则按如下规则
///     1. 关键字范围缩小到只有 string
///     2. 即含有数组,也含有(K:V)键值对; 思路源于Lua对table处理
///     3. 按托管内存方式(即map管理内存释放)
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MAP_H__
#define __KLB_MAP_H__

#include "klb_type.h"
#include "klbutil/klb_adt.h"
#include "klbutil/klb_nvector.h"
#include "klbutil/klb_hlist.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @struct klb_map_t
/// @brief  map
typedef struct klb_map_t_
{
    klb_nvector_t*      p_nvector;  ///< array 数组
    klb_hlist_t*        p_hlist;    ///< key/value 键值对
}klb_map_t;


KLB_API void klb_map_init(klb_map_t* p_map);
KLB_API void klb_map_quit(klb_map_t* p_map);


/// @brief 创建map对象
/// @return klb_map_t*     map对象
KLB_API klb_map_t* klb_map_create();


/// @brief 销毁map对象
/// @param [in] *p_map     map对象
/// @return 无
/// @note 含有清理内存过程
KLB_API void klb_map_destroy(klb_map_t* p_map);


/// @brief object操作 设置
KLB_API void klb_map_set_null(klb_map_t* p_map, const char* p_key);
KLB_API void klb_map_set_bool(klb_map_t* p_map, const char* p_key, bool b);
KLB_API void klb_map_set_int64(klb_map_t* p_map, const char* p_key, int64_t i64);
KLB_API void klb_map_set_uint64(klb_map_t* p_map, const char* p_key, uint64_t u64);
KLB_API void klb_map_set_double(klb_map_t* p_map, const char* p_key, double d);
KLB_API void klb_map_set_string(klb_map_t* p_map, const char* p_key, const char* p_str);
KLB_API void klb_map_set_lstring(klb_map_t* p_map, const char* p_key, const char* p_str, int str_len);
KLB_API void klb_map_set_ptr(klb_map_t* p_map, const char* p_key, const void* ptr1, const void* ptr2);
KLB_API void klb_map_set_map(klb_map_t* p_map, const char* p_key, klb_map_t* ptr); // 托管指针
KLB_API void klb_map_set_map_clone(klb_map_t* p_map, const char* p_key, const klb_map_t* p_src); // 拷贝数据
KLB_API void klb_map_set_adt(klb_map_t* p_map, const char* p_key, klb_adt_t* p_adt); // 托管指针
KLB_API void klb_map_set_adt_clone(klb_map_t* p_map, const char* p_key, const klb_adt_t* p_adt); // 拷贝数据

/// @brief object 类型
KLB_API int klb_map_type(const klb_map_t* p_map, const char* p_key);
KLB_API int klb_map_key_value_size(const klb_map_t* p_map);

/// @brief object 强制取值
KLB_API bool klb_map_to_bool(const klb_map_t* p_map, const char* p_key);
KLB_API int64_t klb_map_to_int64(const klb_map_t* p_map, const char* p_key);
KLB_API uint64_t klb_map_to_uint64(const klb_map_t* p_map, const char* p_key);
KLB_API double klb_map_to_double(const klb_map_t* p_map, const char* p_key);
KLB_API const char* klb_map_to_string(const klb_map_t* p_map, const char* p_key);
KLB_API const char* klb_map_to_lstring(const klb_map_t* p_map, const char* p_key, int* p_out_len);
KLB_API const void* klb_map_to_ptr(const klb_map_t* p_map, const char* p_key, const void** p_out_ptr2);
KLB_API klb_map_t* klb_map_to_map(const klb_map_t* p_map, const char* p_key);
KLB_API const klb_adt_t* klb_map_to_adt(const klb_map_t* p_map, const char* p_key);

KLB_API bool klb_map_remove_by_key(klb_map_t* p_map, const char* p_key);

typedef klb_hlist_iter_t klb_map_iter_t;
KLB_API klb_adt_t* klb_map_data(klb_map_iter_t* p_iter);
KLB_API const char* klb_map_key(const klb_map_iter_t* p_iter);
KLB_API klb_map_iter_t* klb_map_begin(klb_map_t* p_map);
KLB_API klb_map_iter_t* klb_map_end(klb_map_t* p_map);
KLB_API klb_map_iter_t* klb_map_next(klb_map_iter_t* p_iter);
KLB_API klb_map_iter_t* klb_map_prev(klb_map_iter_t* p_iter);
KLB_API void klb_map_remove(klb_map_t* p_map, klb_map_iter_t* p_iter);

/// @brief 数组操作
KLB_API void klb_map_append_null(klb_map_t* p_map);
KLB_API void klb_map_append_bool(klb_map_t* p_map, bool b);
KLB_API void klb_map_append_int64(klb_map_t* p_map, int64_t i64);
KLB_API void klb_map_append_uint64(klb_map_t* p_map, uint64_t u64);
KLB_API void klb_map_append_double(klb_map_t* p_map, double d);
KLB_API void klb_map_append_string(klb_map_t* p_map, const char* p_str);
KLB_API void klb_map_append_lstring(klb_map_t* p_map, const char* p_str, int str_len);
KLB_API void klb_map_append_ptr(klb_map_t* p_map, const void* ptr1, const void* ptr2);
KLB_API void klb_map_append_map(klb_map_t* p_map, klb_map_t* ptr); // 托管指针
KLB_API void klb_map_append_map_clone(klb_map_t* p_map, const klb_map_t* p_src);
KLB_API void klb_map_append_adt(klb_map_t* p_map, klb_adt_t* p_adt); // 托管指针
KLB_API void klb_map_append_adt_clone(klb_map_t* p_map, const klb_adt_t* p_adt);

KLB_API void klb_map_set_idx_null(klb_map_t* p_map, int idx);
KLB_API void klb_map_set_idx_bool(klb_map_t* p_map, int idx, bool b);
KLB_API void klb_map_set_idx_int64(klb_map_t* p_map, int idx, int64_t i64);
KLB_API void klb_map_set_idx_uint64(klb_map_t* p_map, int idx, uint64_t u64);
KLB_API void klb_map_set_idx_double(klb_map_t* p_map, int idx, double d);
KLB_API void klb_map_set_idx_string(klb_map_t* p_map, int idx, const char* p_str);
KLB_API void klb_map_set_idx_lstring(klb_map_t* p_map, int idx, const char* p_str, int str_len);
KLB_API void klb_map_set_idx_ptr(klb_map_t* p_map, int idx, const void* ptr1, const void* ptr2);
KLB_API void klb_map_set_idx_map(klb_map_t* p_map, int idx, klb_map_t* ptr); // 托管指针
KLB_API void klb_map_set_idx_map_clone(klb_map_t* p_map, int idx, const klb_map_t* p_src);
KLB_API void klb_map_set_idx_adt(klb_map_t* p_map, int idx, klb_adt_t* p_adt); // 托管指针
KLB_API void klb_map_set_idx_adt_clone(klb_map_t* p_map, int idx, const klb_adt_t* p_adt);

KLB_API int klb_map_array_type(const klb_map_t* p_map, int idx);
KLB_API int klb_map_array_size(const klb_map_t* p_map);

KLB_API bool klb_map_idx_to_bool(const klb_map_t* p_map, int idx);
KLB_API int64_t klb_map_idx_to_int64(const klb_map_t* p_map, int idx);
KLB_API uint64_t klb_map_idx_to_uint64(const klb_map_t* p_map, int idx);
KLB_API double klb_map_idx_to_double(const klb_map_t* p_map, int idx);
KLB_API const char* klb_map_idx_to_string(const klb_map_t* p_map, int idx);
KLB_API const char* klb_map_idx_to_lstring(const klb_map_t* p_map, int idx, int* p_out_len);
KLB_API const void* klb_map_idx_to_ptr(const klb_map_t* p_map, int idx, const void** p_out_ptr2);
KLB_API klb_map_t* klb_map_idx_to_map(const klb_map_t* p_map, int idx);
KLB_API const klb_adt_t* klb_map_idx_to_adt(const klb_map_t* p_map, int idx);

KLB_API bool klb_map_idx_remove(klb_map_t* p_map, int idx);
KLB_API bool klb_map_idx_remove_tail(klb_map_t* p_map);


/////////////////////////////////////

KLB_API int klb_map_size(const klb_map_t* p_map);
KLB_API void klb_map_clear(klb_map_t* p_map);


// 按值拷贝整个 map
KLB_API void klb_map_copy(klb_map_t* p_dst, const klb_map_t* p_src);


////////////////////////////////////////////////////////////////////////


KLB_API int klb_map_test();


#ifdef __cplusplus
}
#endif

#endif // __KLB_MAP_H__
//end
