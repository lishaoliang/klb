///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_type.h
/// @brief   基础类型,常用宏定义,C99
/// @version 0.1
/// @history 修改历史
///  \n 2019 0.1 创建文件
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
            #define KLB_EXTERN   
            #define KLB_API_CPP __declspec(dllexport)
        #else
            #define KLB_API __declspec(dllexport)
            #define KLB_EXTERN
            #define KLB_API_CPP __declspec(dllexport)
        #endif
    #elif defined(__KLB_USE_DLL__)
        #ifdef __cplusplus
            #define KLB_API extern "C" __declspec(dllimport)
            #define KLB_EXTERN
            #define KLB_API_CPP __declspec(dllimport)
        #else
            #define KLB_API __declspec(dllimport)
            #define KLB_EXTERN
            #define KLB_API_CPP __declspec(dllimport)
        #endif
    #else
        #define KLB_API
        #define KLB_EXTERN
        #define KLB_API_CPP 
    #endif
#else
    #ifdef __cplusplus
        #define KLB_API extern "C"
        #define KLB_EXTERN
        #define KLB_API_CPP 
    #else
        #define KLB_API extern
        #define KLB_EXTERN extern
        #define KLB_API_CPP 
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


/// @def   KLB_ALIGNED_4
/// @brief 4字节对齐
#define KLB_ALIGNED_4(LEN_) ( (0 == ((LEN_) & 0x3)) ? (LEN_) : (((LEN_) + 0x03) & (~0x03)) )


/// @def   KLB_FOURCC
/// @brief FOURCC
#define KLB_FOURCC(CH0_, CH1_, CH2_, CH3_) ( \
                    (uint32_t)(uint8_t)(CH0_) | \
                    ( (uint32_t)(uint8_t)(CH1_) << 8 )  | \
                    ( (uint32_t)(uint8_t)(CH2_) << 16 ) | \
                    ( (uint32_t)(uint8_t)(CH3_) << 24 )  )


/////////////////////////////////////////////////
// copy/modify from libavutil/intreadwrite.h

#ifndef KLB_RB16
#define KLB_RB16(x)                             \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])
#endif

#ifndef KLB_WB16
#define KLB_WB16(p, val) do {                   \
        uint16_t d = (val);                     \
        ((uint8_t*)(p))[1] = (d);               \
        ((uint8_t*)(p))[0] = (d)>>8;            \
    } while(0)
#endif

#ifndef KLB_RB32
#define KLB_RB32(x)                                  \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])
#endif

#ifndef KLB_WB32
#define KLB_WB32(p, val) do {                   \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#endif

#ifndef KLB_RB64
#define KLB_RB64(x)                                     \
    (((uint64_t)((const uint8_t*)(x))[0] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[1] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[6] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[7])
#endif

#ifndef KLB_WB64
#define KLB_WB64(p, val) do {                   \
        uint64_t d = (val);                     \
        ((uint8_t*)(p))[7] = (d);               \
        ((uint8_t*)(p))[6] = (d)>>8;            \
        ((uint8_t*)(p))[5] = (d)>>16;           \
        ((uint8_t*)(p))[4] = (d)>>24;           \
        ((uint8_t*)(p))[3] = (d)>>32;           \
        ((uint8_t*)(p))[2] = (d)>>40;           \
        ((uint8_t*)(p))[1] = (d)>>48;           \
        ((uint8_t*)(p))[0] = (d)>>56;           \
    } while(0)
#endif


#endif // __KLB_TYPE_H__
//end
