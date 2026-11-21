// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vdi.h
/// @brief   VDI blkio 后端
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VDI_H_
#define _PFS_VDI_H_

#include "pfs_blkio.h"
#include "blkio_vdi/pfs_vdi_raw.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// VDI 后端私有态 (挂 pfs_blkio_t.priv); 宿主文件经 raw 打开
/// 子集: DYNAMIC / FIXED 标准块读写; 拒 DIFF/UNDO / parent / 非 1.1 / 非零 block_extra
typedef struct _pfs_vdi_priv_t {
    pfs_blkio_t host;               ///< 宿主 .vdi 文件 (g_pfs_blkio_raw_ops)
    uint32_t    image_type;         ///< PFS_VDI_TYPE_DYNAMIC / FIXED
    uint64_t    disk_size;          ///< [单位:字节] 虚拟盘容量 (已对齐扇区)
    uint32_t    block_size;         ///< [单位:字节]
    uint32_t    offset_blocks;      ///< [单位:字节] 块表文件偏移
    uint32_t    offset_data;        ///< [单位:字节] 数据区文件偏移
    uint32_t    blocks_in_hdd;
    uint32_t    blocks_allocated;
    uint32_t   *p_map;              ///< 块表缓存 (主机序)
    int         want_write;         ///< 非 0: 允许写
} pfs_vdi_priv_t;

extern const pfs_blkio_ops_t g_pfs_vdi_ops;

int pfs_vdi_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);
int pfs_vdi_close(pfs_blkio_t *p_blkio);
int pfs_vdi_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);
int pfs_vdi_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);
int pfs_vdi_fsync(pfs_blkio_t *p_blkio);
int64_t pfs_vdi_get_size(const pfs_blkio_t *p_blkio);
int pfs_vdi_logical_sector_size(const pfs_blkio_t *p_blkio);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VDI_H_

// end
