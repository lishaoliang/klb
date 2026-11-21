// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_time.h
/// @brief   时间辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_TIME_H_
#define _PFS_TIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 单调毫秒 tick (对齐 Win GetTickCount 语义; 约 49.7 天回绕)
/// Win: GetTickCount; Linux: CLOCK_MONOTONIC
uint32_t pfs_get_tickcount(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_TIME_H_

// end
