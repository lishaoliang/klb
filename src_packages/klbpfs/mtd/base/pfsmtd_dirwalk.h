// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_dirwalk.h
/// @brief   目录遍历 (mkvol 灌树)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_DIRWALK_H_
#define _PFSMTD_DIRWALK_H_

#include "pfs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @def   PFSMTD_DIRWALK_PATH_MAX
/// @brief vfs 相对路径缓冲上限
#define PFSMTD_DIRWALK_PATH_MAX         1024

/// @def   PFSMTD_DIRWALK_NAME_MAX
/// @brief 单文件名上限
#define PFSMTD_DIRWALK_NAME_MAX         255

/// @brief 目录遍历回调
/// @param [in] *ctx          用户上下文
/// @param [in] vfs_rel       以 / 开头相对 src 根
/// @param [in] host_path     宿主绝对/相对路径
/// @param [in] is_dir        true=目录; false=普通文件
/// @return int PFS_OK; 非 OK 则中止遍历并回传
typedef int (*pfsmtd_dirwalk_fn)(void *ctx, const char *vfs_rel, const char *host_path, bool is_dir);

/// @brief 深度优先遍历宿主目录树
/// @param [in] src_dir       源根目录
/// @param [in] fn            回调
/// @param [in] *ctx          用户上下文
/// @return int PFS_OK; 跳过 . / .. / 符号链接(Win reparse); 回调非 OK 则中止
int pfsmtd_dirwalk(const char *src_dir, pfsmtd_dirwalk_fn fn, void *ctx);

/// @brief 读宿主文件全部内容
/// @param [in] host_path     宿主路径
/// @param [out] *p_buf       输出缓冲 (pfsmtd_malloc); 调用方 pfsmtd_free
/// @param [out] *p_size      字节数
/// @return int PFS_OK / 负错误码
int pfsmtd_host_read_file(const char *host_path, uint8_t **p_buf, uint32_t *p_size);

#ifdef __cplusplus
}
#endif

#endif // _PFSMTD_DIRWALK_H_

// end
