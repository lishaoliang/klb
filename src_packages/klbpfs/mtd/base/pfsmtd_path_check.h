// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd_path_check.h
/// @brief   路径合法性检查 (pfsmtd 自备; 对齐 portfs base/pfs_path_check)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    供 src_mtd 各 FS ops.check_path; 禁止 include portfs base/pfs_path_check.h
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_PATH_CHECK_H_
#define _PFSMTD_PATH_CHECK_H_

#include "pfs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// 逐组件: 拒 Windows/FAT 非法字符 "*/:<>?\| 与 <0x20; 成功 PFS_OK
int pfsmtd_path_check_win_names(const char *path);

/// 逐组件: UTF-8 字节数 <= PFS_NAME_MAX; 成功 PFS_OK; 超长 PFS_ENAMETOOLONG
int pfsmtd_path_check_name_max(const char *path);

/// Win/FAT 风格 + 组件长度上限 (FAT/exFAT 等)
int pfsmtd_path_check_win_fat(const char *path);

/// Win/FAT 风格 + 长度 + 拒组件内 ':' (NTFS 路径 API 不含 ADS)
int pfsmtd_path_check_win_ntfs(const char *path);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_PATH_CHECK_H_

// end
