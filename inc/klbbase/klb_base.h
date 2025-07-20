///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_base.h
/// @brief   klb需要初始化的部分
/// @version 0.3
/// @history 修改历史
///   \n 2021 0.1 创建文件
///   \n 2025 0.2 添加版本字符串: KLB_VERSION
///   \n [2025-7] 添加版本日期: KLB_VERSION_DATE
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BASE_H__
#define __KLB_BASE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief KLB库版本字符串
#define KLB_VERSION             "1.0.3.20250720"
#define KLB_VERSION_DATE        20250720


/// @brief KLB库版本 数值
///    主版本(3位): 001
///    子版本(3位): 000
///    三版本(3位): 003
#define KLB_VERSION_NUMBER      1000003


/// @brief KLB库初始化
/// @param [in]  *p_json_cfg            json配置
/// @return int 0.成功; 非0.失败
KLB_API int klb_base_init(const char* p_json_cfg);


/// @brief KLB库退出
/// @return 无
KLB_API void klb_base_quit();


/// @brief KLB库版本 字符串
KLB_API const char* klb_version();


/// @brief KLB库版本 数值 / 日期
KLB_API int klb_version_number();
KLB_API int klb_version_date();


#ifdef __cplusplus
}
#endif

#endif // __KLB_BASE_H__
//end
