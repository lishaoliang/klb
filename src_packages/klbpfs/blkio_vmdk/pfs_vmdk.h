// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vmdk.h
/// @brief   VMDK blkio 后端
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VMDK_H_
#define _PFS_VMDK_H_

#include "pfs_blkio.h"
#include "blkio_vmdk/pfs_vmdk_raw.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// 描述符中的一个 extent (FLAT/ZERO; SPARSE extent 文件本阶段不经描述符打开)
typedef struct _pfs_vmdk_extent_t {
    int         type;                   ///< PFS_VMDK_EXT_*
    uint64_t    sectors;                ///< extent 扇区数
    uint64_t    flat_offset_sectors;    ///< FLAT: 文件内起始扇区
    char        path[PFS_VMDK_PATH_MAX];
    pfs_blkio_t host;                   ///< FLAT 数据文件; ZERO 不使用
    int         host_open;
} pfs_vmdk_extent_t;

/// VMDK 后端私有态 (挂 pfs_blkio_t.priv); 宿主文件经 raw 打开
typedef struct _pfs_vmdk_priv_t {
    int                   mode;             ///< PFS_VMDK_MODE_*
    pfs_blkio_oflags_e    open_flags;
    int                   sector_size;      ///< raw_open 扇区参数
    uint64_t              capacity_bytes;   ///< 虚拟盘容量 [字节]

    /// --- MODE_SPARSE ---
    pfs_blkio_t           host;             ///< sparse 宿主文件
    uint32_t              sparse_flags;
    uint64_t              grain_size_sectors;
    uint32_t              num_gtes_per_gt;
    uint64_t              gd_offset_sectors;
    uint64_t              rgd_offset_sectors;   ///< 0=无冗余 GD
    uint64_t              overhead_sectors;
    uint32_t             *p_gd;             ///< Grain Directory (主机序扇区号)
    uint32_t              gd_count;
    uint32_t            **pp_gt;            ///< 每项指向一张 GT (num_gtes_per_gt); NULL=未加载
    uint32_t              grain_count;

    /// --- MODE_FLAT ---
    char                  base_dir[PFS_VMDK_PATH_MAX];
    pfs_vmdk_extent_t    *p_extents;
    int                   extent_count;
} pfs_vmdk_priv_t;

extern const pfs_blkio_ops_t g_pfs_vmdk_ops;

int pfs_vmdk_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);
int pfs_vmdk_close(pfs_blkio_t *p_blkio);
int pfs_vmdk_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);
int pfs_vmdk_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);
int pfs_vmdk_fsync(pfs_blkio_t *p_blkio);
int64_t pfs_vmdk_get_size(const pfs_blkio_t *p_blkio);
int pfs_vmdk_logical_sector_size(const pfs_blkio_t *p_blkio);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VMDK_H_

// end
