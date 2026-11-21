// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_compiler.h
/// @brief   编译器提示与结构体探测宏
///          对齐 linux compiler.h / stddef.h / container_of.h / array_size.h / build_bug.h 用法
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    GCC/Clang 使用 __builtin_expect; MSVC 等无该内建时 likely/unlikely 退化为普通布尔
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_COMPILER_H_
#define _PFS_COMPILER_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(__GNUC__) || defined(__clang__)
#define pfs_likely(x)       (__builtin_expect(!!(x), 1))
#define pfs_unlikely(x)     (__builtin_expect(!!(x), 0))
#else
#define pfs_likely(x)       (!!(x))
#define pfs_unlikely(x)     (!!(x))
#endif

/// @def   pfs_offsetof
/// @brief 成员偏移 (stddef offsetof)
#define pfs_offsetof(type, member) \
        offsetof(type, member)

/// @def   pfs_offsetofend
/// @brief 成员结束偏移 (= offsetof + sizeof 该成员)
#define pfs_offsetofend(type, member) \
        (offsetof(type, member) + sizeof(((type *)0)->member))

/// @def   pfs_sizeof_field
/// @brief 成员大小
#define pfs_sizeof_field(type, member) \
        sizeof(((type *)0)->member)

/// @def   pfs_container_of
/// @brief 由成员指针反推外层结构体指针 (linux container_of; 无 GNU 语句表达式, 便于 MSVC)
#define pfs_container_of(ptr, type, member) \
        ((type *)((char *)(ptr) - offsetof(type, member)))

/// @def   pfs_array_size
/// @brief 数组元素个数 (勿用于衰减后的指针)
#define pfs_array_size(arr) \
        (sizeof(arr) / sizeof((arr)[0]))

/// @def   pfs_build_bug_on
/// @brief 编译期断言: condition 为真则编译失败 (linux BUILD_BUG_ON 精简可移植版)
#define pfs_build_bug_on(condition) \
        ((void)sizeof(char[1 - 2 * !!(condition)]))

/// @def   pfs_build_bug_on_zero
/// @brief 编译期断言: 表达式 (常用于宏展开中占位, 值为 0)
#define pfs_build_bug_on_zero(e) \
        (sizeof(char[1 - 2 * !!(e)]) - 1)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_COMPILER_H_

// end
