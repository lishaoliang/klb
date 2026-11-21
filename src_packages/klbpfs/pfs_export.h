// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_export.h
/// @brief   libpfs 符号导出/导入宏 (PFS_API)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅预处理宏, 无函数声明
///  \n Win 编 DLL: 定义 PFS_EXPORTS (MSVC 默认) 或 __PFS_BUILD_DLL__
///  \n Win 链 DLL: 定义 __PFS_USE_DLL__
///  \n Linux 编 .so: 定义 __PFS_SYMBOL_HIDING__ 且 -fvisibility=hidden
///  \n 编入 libklb 时: 对齐 __KLB_*_DLL__ / __KLB_SYMBOL_HIDING__ (与 KLB_API 同可见性)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXPORT_H_
#define _PFS_EXPORT_H_

/// @def   PFS_API
/// @brief 导出/导入 C 符号 (对外函数声明前缀)
#if defined(_WIN32)
    #if defined(__PFS_BUILD_DLL__) || defined(PFS_EXPORTS) || defined(__KLB_BUILD_DLL__) || defined(__KLB_CORE_API__)
        #ifdef __cplusplus
            #define PFS_API extern "C" __declspec(dllexport)
            #define PFS_EXTERN
            #define PFS_API_CPP __declspec(dllexport)
        #else
            #define PFS_API __declspec(dllexport)
            #define PFS_EXTERN
            #define PFS_API_CPP __declspec(dllexport)
        #endif
    #elif defined(__PFS_USE_DLL__) || defined(__KLB_USE_DLL__)
        #ifdef __cplusplus
            #define PFS_API extern "C" __declspec(dllimport)
            #define PFS_EXTERN
            #define PFS_API_CPP __declspec(dllimport)
        #else
            #define PFS_API __declspec(dllimport)
            #define PFS_EXTERN
            #define PFS_API_CPP __declspec(dllimport)
        #endif
    #else
        #define PFS_API
        #define PFS_EXTERN
        #define PFS_API_CPP
    #endif
#else
    #if defined(__PFS_SYMBOL_HIDING__) || defined(__KLB_SYMBOL_HIDING__)
        #ifdef __cplusplus
            #define PFS_API extern "C" __attribute__((visibility("default")))
            #define PFS_EXTERN
            #define PFS_API_CPP __attribute__((visibility("default")))
        #else
            #define PFS_API extern __attribute__((visibility("default")))
            #define PFS_EXTERN extern
            #define PFS_API_CPP __attribute__((visibility("default")))
        #endif
    #else
        #ifdef __cplusplus
            #define PFS_API extern "C"
            #define PFS_EXTERN
            #define PFS_API_CPP
        #else
            #define PFS_API extern
            #define PFS_EXTERN extern
            #define PFS_API_CPP
        #endif
    #endif
#endif

#endif // _PFS_EXPORT_H_

// end
