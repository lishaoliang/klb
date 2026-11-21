// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_util.h
/// @brief   库内杂项公共辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    本库模块共用; 非对外总入口
///  \n 堆内存请用 base/pfsmtd_mem.h (禁止 include portfs base/pfs_mem.h)
///  \n 仅可引用 portfs src 根对外头: pfs.h / pfs_blkio.h / pfs_ops.h / pfs_part.h
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_UTIL_H_
#define _PFSMTD_UTIL_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 库版本串 (静态只读; 格式 "major.minor.patch")
const char *pfsmtd_version(void);

/// @def   PFSMTD_MIN
/// @brief 安全取较小值 (求值两次; 勿传带副作用实参)
#define PFSMTD_MIN(a, b)            (((a) < (b)) ? (a) : (b))

/// @def   PFSMTD_MAX
/// @brief 安全取较大值 (求值两次; 勿传带副作用实参)
#define PFSMTD_MAX(a, b)            (((a) > (b)) ? (a) : (b))

/// @def   PFSMTD_CLAMP
/// @brief 将 v 限制在 [lo, hi] (假定 lo <= hi)
#define PFSMTD_CLAMP(v, lo, hi)     PFSMTD_MIN(PFSMTD_MAX((v), (lo)), (hi))

/// @brief 向上对齐到 align (align 须为 2 的幂且非 0)
static inline uint64_t pfsmtd_align_up_u64(uint64_t v, uint64_t align)
{
    return (v + (align - 1u)) & ~(align - 1u);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_UTIL_H_

// end
