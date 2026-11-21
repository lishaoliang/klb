// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_log.h
/// @brief   库内日志
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_LOG_H_
#define _PFS_LOG_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 库内日志 (输出固定带 "[pfs]" 前缀; 默认写 stdout; 可设级别过滤)
/// 用法: PFS_LOG("mount ok");  PFS_LOGI("type=%d", type);  或 pfs_log(PFS_LOG_INFO, "...")
/// PFS_LOG 等同 PFS_LOGI (常规打印)

typedef enum _pfs_log_level_e {
    PFS_LOG_ERROR               = 0,                    ///< 错误
    PFS_LOG_WARN                = 1,                    ///< 警告
    PFS_LOG_INFO                = 2,                    ///< 信息 (默认阈值)
    PFS_LOG_DEBUG               = 3,                    ///< 调试
} pfs_log_level_e;

/// 设置/查询当前打印阈值; 高于阈值的级别被丢弃
void pfs_log_set_level(int level);
int pfs_log_get_level(void);

/// 打印一行: "[pfs][E|W|I|D] " + fmt... + '\n'; level 高于阈值则无输出
void pfs_log(int level, const char *fmt, ...);
void pfs_logv(int level, const char *fmt, va_list ap);

#ifndef PFS_LOG_DISABLE
#define PFS_LOGE(...)               pfs_log(PFS_LOG_ERROR, __VA_ARGS__)
#define PFS_LOGW(...)               pfs_log(PFS_LOG_WARN, __VA_ARGS__)
#define PFS_LOGI(...)               pfs_log(PFS_LOG_INFO, __VA_ARGS__)
#define PFS_LOGD(...)               pfs_log(PFS_LOG_DEBUG, __VA_ARGS__)
#define PFS_LOG(...)                PFS_LOGI(__VA_ARGS__)
#else
#define PFS_LOGE(...)               ((void)0)
#define PFS_LOGW(...)               ((void)0)
#define PFS_LOGI(...)               ((void)0)
#define PFS_LOGD(...)               ((void)0)
#define PFS_LOG(...)                ((void)0)
#endif


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_LOG_H_

// end
