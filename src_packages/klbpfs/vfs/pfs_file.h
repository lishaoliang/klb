// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_file.h
/// @brief   VFS 文件句柄完整定义 (模块内部)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    vfs 内部头; 对外仅见 src/pfs.h 不透明 pfs_file_t
///  \n FS 私有态在 p_fpriv (exfat/fat/ntfs *_ops 填表); 勿在此嵌 ntfs_meta 等
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_FILE_H_
#define _PFS_FILE_H_

#include "pfs.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief VFS 打开文件句柄 (完整定义)
struct _pfs_file_t {
    pfs_ctx_t               *p_ctx;                     ///< 借用挂载上下文
    int                     mode;                       ///< PFS_O_RD 或 PFS_O_RDWR, 可选 | CREAT
    int64_t                 pos;                        ///< [单位:字节] 当前偏移
    int64_t                 size;                       ///< [单位:字节] 逻辑文件大小缓存 (get_size)
    int                     err;                        ///< 非 0=出错 (PFS_* 负码)
    int                     eof;                        ///< 非 0=已到 EOF
    void                   *p_fpriv;                    ///< FS 私有文件态 (按 file.priv_size 分配)
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_FILE_H_

// end
