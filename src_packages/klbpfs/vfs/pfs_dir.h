// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_dir.h
/// @brief   VFS 目录句柄完整定义 (模块内部)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    vfs 内部头; 对外仅见 src/pfs.h 不透明 pfs_dir_t
///  \n FS 私有态在 p_dpriv; . / .. 由 vfs 合成; 勿在此嵌 ntfs_meta 等
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_DIR_H_
#define _PFS_DIR_H_

#include "pfs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief VFS 打开目录句柄 (完整定义)
struct _pfs_dir_t {
    pfs_ctx_t               *p_ctx;                     ///< 借用挂载上下文
    int                     dots_done;                  ///< 0=. 1=.. 2=磁盘项
    int                     eof;                        ///< 非 0=已无更多项
    int                     err;                        ///< 非 0=出错 (PFS_* 负码)
    pfs_dirent_t            entry;                      ///< 最近一次 readdir 结果缓冲
    void                   *p_dpriv;                    ///< FS 私有目录态 (按 dir.priv_size 分配)
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_DIR_H_

// end
