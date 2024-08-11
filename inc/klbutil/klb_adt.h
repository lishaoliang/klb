///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_adt.h
/// @brief   抽象数据类型: abstruct data type
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_ADT_H__
#define __KLB_ADT_H__

#include "klb_type.h"
#include "klbthird/sds.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_map_t_ klb_map_t;


typedef enum klb_adt_type_e_
{
    KLB_ADT_null        = 0,    ///< null
    KLB_ADT_bool        = 1,    ///< bool
    KLB_ADT_string      = 2,    ///< string 或 buffer; 注意buffer按string类型存储
    KLB_ADT_double      = 3,    ///< double
    KLB_ADT_uint64      = 4,    ///< uint64
    KLB_ADT_int64       = 5,    ///< int64
    KLB_ADT_map         = 7,    ///< map(klb_map_t)

    KLB_ADT_ptr         = 10,   ///< ptr1, ptr2; 函数地址或附加参数等
}klb_adt_type_e;


typedef struct klb_adt_t_
{
    int             type;       ///< klb_adt_type_e

    union
    {
        bool        b;          ///< KLB_ADT_bool
        sds         str;        ///< KLB_ADT_string 
        
        struct
        {
            void*   ptr1;       ///< KLB_ADT_ptr
            void*   ptr2;       ///< KLB_ADT_ptr 附加指针2
        };

        double      d;          ///< KLB_ADT_double
        uint64_t    u64;        ///< KLB_ADT_uint64
        int64_t     i64;        ///< KLB_ADT_int64

        klb_map_t*  p_map;      ///< KLB_ADT_map
    };
}klb_adt_t;


KLB_API void klb_adt_init(klb_adt_t* p_adt);
KLB_API void klb_adt_quit(klb_adt_t* p_adt);

KLB_API klb_adt_t* klb_adt_create();
KLB_API void klb_adt_destroy(klb_adt_t* p_adt);

KLB_API void klb_adt_set_null(klb_adt_t* p_adt);
KLB_API void klb_adt_set_bool(klb_adt_t* p_adt, bool b);
KLB_API void klb_adt_set_string(klb_adt_t* p_adt, const char* p_str);
KLB_API void klb_adt_set_lstring(klb_adt_t* p_adt, const char* p_str, int str_len);
KLB_API void klb_adt_set_ptr(klb_adt_t* p_adt, const void* ptr1, const void* ptr2);
KLB_API void klb_adt_set_double(klb_adt_t* p_adt, double d);
KLB_API void klb_adt_set_uint64(klb_adt_t* p_adt, uint64_t u64);
KLB_API void klb_adt_set_int64(klb_adt_t* p_adt, int64_t i64);
KLB_API void klb_adt_set_map(klb_adt_t* p_adt, klb_map_t* ptr); // 托管指针
KLB_API void klb_adt_set_map_clone(klb_adt_t* p_adt, const klb_map_t* p_src);

KLB_API klb_adt_type_e klb_adt_type(klb_adt_t* p_adt);


// 强制转换, 数据类型不对应时, 按内部定义默认值处理
KLB_API bool klb_adt_to_bool(klb_adt_t* p_adt);
KLB_API const char* klb_adt_to_string(klb_adt_t* p_adt);
KLB_API const char* klb_adt_to_lstring(klb_adt_t* p_adt, int* p_out_len);
KLB_API const void* klb_adt_to_ptr(klb_adt_t* p_adt, const void** p_out_ptr2);
KLB_API double klb_adt_to_double(klb_adt_t* p_adt);
KLB_API uint64_t klb_adt_to_uint64(klb_adt_t* p_adt);
KLB_API int64_t klb_adt_to_int64(klb_adt_t* p_adt);
KLB_API klb_map_t* klb_adt_to_map(klb_adt_t* p_adt); // adt 原始数据指针

// 非强制转换, 数据类型不对应时, 按参数给定的默认值处理
KLB_API bool klb_adt_get_bool(klb_adt_t* p_adt, bool default_b);
KLB_API const char* klb_adt_get_string(klb_adt_t* p_adt, const char* p_default_str);
KLB_API const char* klb_adt_get_lstring(klb_adt_t* p_adt, const char* p_default_str, int* p_in_out_len);
KLB_API double klb_adt_get_double(klb_adt_t* p_adt, double default_d);
KLB_API uint64_t klb_adt_get_uint64(klb_adt_t* p_adt, uint64_t default_u64);
KLB_API int64_t klb_adt_get_int64(klb_adt_t* p_adt, int64_t default_i64);


KLB_API void klb_adt_copy(klb_adt_t* p_dst, const klb_adt_t* p_src);




#ifdef __cplusplus
}
#endif

#endif // __KLB_ADT_H__
//end
