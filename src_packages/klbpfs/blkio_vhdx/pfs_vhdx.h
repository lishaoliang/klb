// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhdx.h
/// @brief   VHDX blkio 后端
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VHDX_H_
#define _PFS_VHDX_H_

#include "pfs_blkio.h"
#include "blkio_vhdx/pfs_vhdx_raw.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// VHDX 后端私有态 (挂 pfs_blkio_t.priv); 宿主文件经 raw 打开
/// Fixed/Dynamic 可读可写; Differencing 拒绝; 脏 log 可写打开时最小回放
typedef struct _pfs_vhdx_priv_t {
    pfs_blkio_t host;               ///< 宿主 .vhdx 文件 (g_pfs_blkio_raw_ops)
    pfs_blkio_oflags_e open_flags;
    uint32_t    disk_type;          ///< PFS_VHDX_DISK_*
    uint64_t    virtual_disk_size;  ///< [单位:字节] VirtualDiskSize
    uint32_t    block_size;         ///< [单位:字节] FileParameters.BlockSize
    uint32_t    logical_sector_size; ///< [单位:字节] 512 或 4096
    uint32_t    physical_sector_size;
    uint32_t    sectors_per_block;
    uint64_t    chunk_ratio;
    uint32_t    bat_entries;
    uint64_t    bat_offset;         ///< [单位:字节] BAT 区宿主偏移
    uint64_t    bat_length;         ///< [单位:字节] BAT region 长度
    uint64_t   *p_bat;              ///< BAT 缓存 (主机序 uint64)

    /// 活动 header 缓存 (写元数据 / log 用)
    uint64_t    hdr_sequence;
    pfs_vhdx_guid_t file_write_guid;
    pfs_vhdx_guid_t data_write_guid;
    pfs_vhdx_guid_t log_guid;
    uint64_t    log_offset;         ///< [单位:字节]
    uint32_t    log_length;         ///< [单位:字节]
    uint64_t    log_sequence;       ///< 下一 log entry 序号 (从 1 起)
    int         first_visible_write; ///< 1=尚未写过客户可见数据
} pfs_vhdx_priv_t;

extern const pfs_blkio_ops_t g_pfs_vhdx_ops;

int pfs_vhdx_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);
int pfs_vhdx_close(pfs_blkio_t *p_blkio);
int pfs_vhdx_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);
int pfs_vhdx_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);
int pfs_vhdx_fsync(pfs_blkio_t *p_blkio);
int64_t pfs_vhdx_get_size(const pfs_blkio_t *p_blkio);
int pfs_vhdx_logical_sector_size(const pfs_blkio_t *p_blkio);

/// 创建空 VHDX (覆盖已存在文件); size_bytes 须为逻辑扇区(512)对齐
/// Fixed: 宿主预分配 virtual size + 元数据; Dynamic: 瘦文件 (BAT 全 NOT_PRESENT)
/// 逻辑扇区 512; 块大小 2MiB; 预留 1MiB log 区 (初始 log_guid=0)
int pfs_vhdx_create_fixed(const char *path, uint64_t size_bytes);
int pfs_vhdx_create_dynamic(const char *path, uint64_t size_bytes);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHDX_H_

// end
