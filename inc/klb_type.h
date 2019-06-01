///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/19
//
/// @file    klb_type.h
/// @brief   基础类型,常用宏定义
/// @author  李绍良
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/19 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TYPE_H__
#define __KLB_TYPE_H__


/// @def   int*_t, uint*_t
/// @brief 引入与32,64位系统无关的类型定义
/// @note 
///  \n int8_t  int16_t  int32_t  int64_t
///  \n uint8_t uint16_t uint32_t uint64_t
#include <stdint.h>


/// @def   KLB_API
/// @brief 导出/导入函数
#if defined(_MSC_VER)
    #if defined(__KLB_BUILD_DLL__) || defined(__KLB_CORE_API__)
        #define KLB_API extern "C" __declspec(dllexport)
    #elif defined(__KLB_USE_DLL__)
        #define KLB_API extern "C" __declspec(dllimport)
    #else
        #define KLB_API
    #endif
#else
    #ifdef __cplusplus
        #define KLB_API extern "C"
    #else
        #define KLB_API extern
    #endif
#endif


/// @def   NULL
/// @brief NULL定义
#ifndef NULL
    #ifdef __cplusplus
        #define NULL 0
    #else
        #define NULL ((void *)0)
    #endif
#endif


/// @def   TRUE
/// @brief TRUE定义
#ifndef TRUE
    #define TRUE    1
#endif


/// @def   FALSE
/// @brief FALSE定义
#ifndef FALSE
    #define FALSE   0
#endif


/// @def   MAX
/// @brief MAX宏
#ifndef MAX
    #define MAX(A_,B_)    (((A_) > (B_)) ? (A_) : (B_))
#endif


/// @def   MIN
/// @brief MIN宏
#ifndef MIN
    #define MIN(A_,B_)    (((A_) < (B_)) ? (A_) : (B_))
#endif



#endif // __KLB_TYPE_H__
//end
