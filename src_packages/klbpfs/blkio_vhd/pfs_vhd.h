// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhd.h
/// @brief   VHD blkio 后端
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VHD_H_
#define _PFS_VHD_H_

#include "pfs_blkio.h"
#include "blkio_vhd/pfs_vhd_raw.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// VHD 后端私有态 (挂 pfs_blkio_t.priv); 宿主文件经 raw 打开
typedef struct _pfs_vhd_priv_t {
    pfs_blkio_t host;           ///< 宿主 .vhd 文件 (g_pfs_blkio_raw_ops)
    uint32_t    disk_type;      ///< PFS_VHD_DISK_TYPE_*
    uint64_t    current_size;   ///< [单位:字节] 虚拟盘容量 (footer.CurrentSize)
    uint64_t    table_offset;   ///< [单位:字节] BAT 偏移; Fixed 为 0
    uint32_t    block_size;     ///< [单位:字节] Dynamic 块大小; Fixed 为 0
    uint32_t    max_table_entries;
    uint32_t    bitmap_sectors; ///< [单位:扇区] 每数据块前 sector bitmap 占用
    uint32_t   *p_bat;          ///< BAT 缓存 (扇区号大端已转主机序); Fixed 为 NULL
} pfs_vhd_priv_t;

/// 静态 ops 表 (facade 探测后可绑此表); open_direct 为 NULL
extern const pfs_blkio_ops_t g_pfs_vhd_ops;

int pfs_vhd_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);
int pfs_vhd_close(pfs_blkio_t *p_blkio);
int pfs_vhd_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);
int pfs_vhd_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);
int pfs_vhd_fsync(pfs_blkio_t *p_blkio);
int64_t pfs_vhd_get_size(const pfs_blkio_t *p_blkio);
int pfs_vhd_logical_sector_size(const pfs_blkio_t *p_blkio);

/// 创建空 VHD (覆盖已存在文件); size_bytes=虚拟盘容量且须 512 对齐; 逻辑扇区 512
/// Fixed: 宿主文件 size_bytes+512 (非 sparse); Dynamic: 瘦文件 (BAT 全未分配, 块大小 2MiB)
int pfs_vhd_create_fixed(const char *path, uint64_t size_bytes);
int pfs_vhd_create_dynamic(const char *path, uint64_t size_bytes);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHD_H_

// end
