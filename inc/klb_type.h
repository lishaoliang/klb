///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/05/19
//
/// @file    klb_type.h
/// @brief   基础类型,常用宏定义,C99
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/19 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TYPE_H__
#define __KLB_TYPE_H__


#ifdef _WIN32
    //define something for Windows (32-bit and 64-bit, this part is common)
    #ifdef _WIN64
        //define something for Windows (64-bit only)
    #else
        //define something for Windows (32-bit only)
    #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
        // iOS Simulator
    #elif TARGET_OS_IPHONE
        // iOS device
    #elif TARGET_OS_MAC
        // Other kinds of Mac OS
    #else
        #error "Unknown Apple platform"
    #endif
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#elif defined(__STM32__)
    // STM32
#elif defined(__EMSCRIPTEN__)
    // emscripten
#else
    #error "Unknown compiler"
#endif


/// @def   int*_t, uint*_t
/// @brief 引入标准类型定义
/// @note 
///  \n int8_t  int16_t  int32_t  int64_t
///  \n uint8_t uint16_t uint32_t uint64_t
///  \n intptr_t uintptr_t size_t
///  \n bool false true
#include <stdint.h>     /// int*_t uint*_t
#include <stddef.h>     /// intptr_t uintptr_t size_t
#include <stdbool.h>    /// bool false true


/// @def   KLB_API
/// @brief 导出/导入函数
#if defined(_WIN32)
    #if defined(__KLB_BUILD_DLL__) || defined(__KLB_CORE_API__)
        #ifdef __cplusplus
            #define KLB_API extern "C" __declspec(dllexport)
        #else
            #define KLB_API __declspec(dllexport)
        #endif
    #elif defined(__KLB_USE_DLL__)
        #ifdef __cplusplus
            #define KLB_API extern "C" __declspec(dllimport)
        #else
            #define KLB_API __declspec(dllimport)
        #endif
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

/// @def    ABS_SUB
/// @brief 差值的绝对值: 适用于无符号类型
#ifndef ABS_SUB
    #define ABS_SUB(A_, B_)     (((A_) < (B_)) ? ((B_) - (A_)) : ((A_) - (B_)))
#endif


/// @def   KLB_PADDING
/// @brief 4字节对齐且保证末尾有多余至少[1,4]字节
#define KLB_PADDING_4(LEN_) ( (0 == ((LEN_) & 0x3)) ? ((LEN_) + 4) : (((LEN_) + 0x03) & (~0x03)) )


#define KLB_FOURCC(CH0_, CH1_, CH2_, CH3_) ( \
                    (uint32_t)(uint8_t)(CH0_) | \
                    ( (uint32_t)(uint8_t)(CH1_) << 8 )  | \
                    ( (uint32_t)(uint8_t)(CH2_) << 16 ) | \
                    ( (uint32_t)(uint8_t)(CH3_) << 24 )  )

#endif // __KLB_TYPE_H__
//end
