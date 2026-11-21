// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_blkio_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_BLKIO_RAW_H_
#define _PFS_BLKIO_RAW_H_

#include "pfs_blkio.h"

#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// raw 后端私有态 (挂 pfs_blkio_t.priv)
typedef struct _pfs_blkio_raw_priv_t {
#ifdef _WIN32
    HANDLE  h_device;           ///< 未 open 时为 INVALID_HANDLE_VALUE
    int     sector_size;        ///< 缓冲 open 逻辑扇区; open_direct 为 0
    uint8_t *p_head_sector;
    uint8_t *p_tail_sector;
    uint8_t *p_buf;
    int     buf_len;
#else
    int     fd;                 ///< 未 open 时为 -1
#endif
} pfs_blkio_raw_priv_t;

/// 静态 ops 表 (facade open 默认绑此表)
extern const pfs_blkio_ops_t g_pfs_blkio_raw_ops;

/// ops 回调 (亦可直接调用; 须已由 facade 设 ops 或自行保证 priv 语义)
int pfs_blkio_raw_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);
int pfs_blkio_raw_open_direct(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int align_size);
int pfs_blkio_raw_close(pfs_blkio_t *p_blkio);
int pfs_blkio_raw_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);
int pfs_blkio_raw_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);
int pfs_blkio_raw_fsync(pfs_blkio_t *p_blkio);
int64_t pfs_blkio_raw_get_size(const pfs_blkio_t *p_blkio);
int pfs_blkio_raw_logical_sector_size(const pfs_blkio_t *p_blkio);

/// 路径探测 (不依赖已 open 句柄)
int pfs_blkio_raw_sector_size(const char *p_devpath);
int pfs_blkio_raw_sector_size_physical(const char *p_devpath);

/// 测试/卷控制: 取底层宿主句柄 (非 raw 或未 open 返回无效)
#ifdef _WIN32
HANDLE pfs_blkio_raw_handle(const pfs_blkio_t *p_blkio);
#else
int pfs_blkio_raw_fd(const pfs_blkio_t *p_blkio);
#endif

#ifdef __cplusplus
}
#endif

#endif // _PFS_BLKIO_RAW_H_

// end
