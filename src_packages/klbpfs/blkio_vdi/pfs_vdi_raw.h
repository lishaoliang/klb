// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vdi_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VDI_RAW_H_
#define _PFS_VDI_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// VirtualBox VDI on-disk 布局子集
// 参考: VirtualBox VDI / QEMU block/vdi.c (pre-header 64B text + header)

#define PFS_VDI_SECTOR_SIZE             512
#define PFS_VDI_PRE_HEADER_SIZE         64
#define PFS_VDI_HEADER_SIZE_MIN         400
#define PFS_VDI_PROBE_SIZE              512
#define PFS_VDI_DEFAULT_BLOCK_SIZE      (1024u * 1024u)

// header.u32Signature (little-endian)
#define PFS_VDI_SIGNATURE               0xBEDA107Fu
#define PFS_VDI_VERSION_1_1             0x00010001u

// header.u32Type
#define PFS_VDI_TYPE_DYNAMIC            1u
#define PFS_VDI_TYPE_FIXED              2u
#define PFS_VDI_TYPE_UNDO               3u
#define PFS_VDI_TYPE_DIFF               4u

// block map 特殊值 (LE on-disk)
#define PFS_VDI_MAP_UNALLOCATED         0xFFFFFFFFu
#define PFS_VDI_MAP_ZERO                0xFFFFFFFEu
#define PFS_VDI_MAP_IS_ALLOCATED(x)     ((uint32_t)(x) < PFS_VDI_MAP_ZERO)

// 相对 header 起点 (文件偏移 = PRE_HEADER + 本值)
#define PFS_VDI_OFF_SIGNATURE           0u
#define PFS_VDI_OFF_VERSION             4u
#define PFS_VDI_OFF_HEADER_SIZE         8u
#define PFS_VDI_OFF_IMAGE_TYPE          12u
#define PFS_VDI_OFF_OFFSET_BLOCKS       276u
#define PFS_VDI_OFF_OFFSET_DATA         280u
#define PFS_VDI_OFF_SECTOR_SIZE         296u
#define PFS_VDI_OFF_DISK_SIZE           304u
#define PFS_VDI_OFF_BLOCK_SIZE          312u
#define PFS_VDI_OFF_BLOCK_EXTRA         316u
#define PFS_VDI_OFF_BLOCKS_IN_HDD       320u
#define PFS_VDI_OFF_BLOCKS_ALLOCATED    324u
#define PFS_VDI_OFF_UUID_LINK           360u
#define PFS_VDI_OFF_UUID_PARENT         376u

// map 项上限 (约 4GiB map 内; 1MiB 块对应约 1PiB 虚盘上限远大于本栈测盘)
#define PFS_VDI_BLOCKS_IN_IMAGE_MAX     (256u * 1024u)

#pragma pack(push, 1)

// VDI 头前缀 (跳过 64 字节文本 pre-header 后)
typedef struct _pfs_vdi_header_t {
    uint32_t signature;             ///< PFS_VDI_SIGNATURE
    uint32_t version;               ///< 主次版本打包
    uint32_t header_size;
    uint32_t image_type;            ///< PFS_VDI_TYPE_*
    uint32_t image_flags;
    uint8_t  description[256];
    uint32_t offset_blocks;         ///< [单位:字节] 块表偏移
    uint32_t offset_data;           ///< [单位:字节] 数据偏移
    uint32_t cylinders;             ///< 几何 (可忽略)
    uint32_t heads;
    uint32_t sectors;
    uint32_t sector_size;
    uint32_t unused0;
    uint64_t disk_size;             ///< 虚拟盘容量 [单位:字节]
    uint32_t block_size;
    uint32_t block_extra;
    uint32_t blocks_in_hdd;
    uint32_t blocks_allocated;
} pfs_vdi_header_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_VDI_RAW_H_

// end
