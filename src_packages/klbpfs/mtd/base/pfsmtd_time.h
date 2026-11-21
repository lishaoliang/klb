// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_time.h
/// @brief   单调毫秒 tick
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对齐 Win GetTickCount 语义; 约 49.7 天回绕
///  \n 同步自 portfs base/pfs_time.*; Win: GetTickCount; Linux: CLOCK_MONOTONIC
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_TIME_H_
#define _PFSMTD_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 单调毫秒 tick
uint32_t pfsmtd_get_tickcount(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_TIME_H_

// end
