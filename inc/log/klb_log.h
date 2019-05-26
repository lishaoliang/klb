///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, MIT License
//  Created: 2019/05/25
//
/// @file    klb_log.h
/// @brief   日志打印
/// @author  李绍良
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/25 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_LOG_H__
#define __KLB_LOG_H__

#include "klb_type.h"
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_LOG
/// @brief 常规打印
#define KLB_LOG(FMT_, ...)      printf(FMT_, ## __VA_ARGS__)


/// @def   KLB_LOG_D
/// @brief 调试打印
#define KLB_LOG_D(FMT_, ...)    printf(FMT_, ## __VA_ARGS__)


/// @def   KLB_LOG_E
/// @brief 错误打印
#define KLB_LOG_E(FMT_, ...)    printf(FMT_, ## __VA_ARGS__)


/// @def   KLB_LOG_S
/// @brief 系统错误打印
#define KLB_LOG_S(FMT_, ...)    printf(FMT_, ## __VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif // __KLB_LOG_H__
//end
