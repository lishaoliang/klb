///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/05/26
//
/// @file    klb_time.h
/// @brief   时间相关
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/05/26 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_TIME_H__
#define __KLB_TIME_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(__STM32__)


/// @brief 获取系统滴答数(毫秒)
/// @return uint32_t 系统滴答
KLB_API uint32_t klb_tick_count();


/// @brief 获取系统滴答数(毫秒)
/// @return uint64_t 系统滴答
KLB_API uint64_t klb_tick_count64();

/// @brief 获取系统滴答数(毫秒)
/// @return int64_t 系统滴答
KLB_API int64_t klb_tick_counti64();

#endif

/// @def   KLB_TIME_1S
/// @brief 1秒 = 1000毫秒
#define KLB_TIME_1S         1000


/// @def   KLB_TIME_1M
/// @brief 1分钟 = ‭60,000‬毫秒
#define KLB_TIME_1M         ‭60000‬


/// @def   KLB_TIME_1H
/// @brief 1小时 = ‭‭3,600,000‬毫秒
#define KLB_TIME_1H         ‭3600000‬


/// @def   ‭KLB_TIME_12H
/// @brief 12小时 = ‭‭43,200,000‬毫秒
#define KLB_TIME_12H        43200000‬


/// @def   ‭KLB_TIME_24H
/// @brief 24小时 = ‭‭‭86,400,000‬‬毫秒
#define KLB_TIME_1D         ‭86400000‬


#ifdef __cplusplus
}
#endif

#endif // __KLB_TIME_H__
//end
