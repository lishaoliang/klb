// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_log.h
/// @brief   pfsmtd 库内日志
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    输出固定带 "[pfsmtd]" 前缀; 默认写 stdout; 可设级别过滤
///  \n 与 portfs pfs_log 独立; 勿混用宏前缀
///  \n 用法: PFSMTD_LOGI("open mtd ok"); 或 pfsmtd_log(PFSMTD_LOG_INFO, "...")
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_LOG_H_
#define _PFSMTD_LOG_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 日志级别
typedef enum _pfsmtd_log_level_e {
    PFSMTD_LOG_ERROR            = 0,                    ///< 错误
    PFSMTD_LOG_WARN             = 1,                    ///< 警告
    PFSMTD_LOG_INFO             = 2,                    ///< 信息 (默认阈值)
    PFSMTD_LOG_DEBUG            = 3,                    ///< 调试
} pfsmtd_log_level_e;

/// @brief 设置日志级别阈值
void pfsmtd_log_set_level(int level);

/// @brief 获取日志级别阈值
int pfsmtd_log_get_level(void);

/// @brief 写日志
void pfsmtd_log(int level, const char *fmt, ...);

/// @brief 写日志 (va_list)
void pfsmtd_logv(int level, const char *fmt, va_list ap);

#ifndef PFSMTD_LOG_DISABLE
#define PFSMTD_LOGE(...)            pfsmtd_log(PFSMTD_LOG_ERROR, __VA_ARGS__)
#define PFSMTD_LOGW(...)            pfsmtd_log(PFSMTD_LOG_WARN, __VA_ARGS__)
#define PFSMTD_LOGI(...)            pfsmtd_log(PFSMTD_LOG_INFO, __VA_ARGS__)
#define PFSMTD_LOGD(...)            pfsmtd_log(PFSMTD_LOG_DEBUG, __VA_ARGS__)
#define PFSMTD_LOG(...)             PFSMTD_LOGI(__VA_ARGS__)
#else
#define PFSMTD_LOGE(...)            ((void)0)
#define PFSMTD_LOGW(...)            ((void)0)
#define PFSMTD_LOGI(...)            ((void)0)
#define PFSMTD_LOGD(...)            ((void)0)
#define PFSMTD_LOG(...)             ((void)0)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_LOG_H_

// end
