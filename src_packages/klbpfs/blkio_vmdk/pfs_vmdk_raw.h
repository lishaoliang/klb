// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vmdk_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VMDK_RAW_H_
#define _PFS_VMDK_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// VMware VMDK on-disk 布局子集 (Virtual Disk Format 1.1 / 5.x)
// SparseExtentHeader 与 GD/GT 为小端; 文本描述符为 ASCII

#define PFS_VMDK_SECTOR_SIZE            512
#define PFS_VMDK_PROBE_SIZE             512
#define PFS_VMDK_SPARSE_HEADER_SIZE     512
#define PFS_VMDK_PATH_MAX               512
#define PFS_VMDK_EXTENT_MAX             64
#define PFS_VMDK_DESC_MAX               (64 * 1024)

// Sparse extent header magic (little-endian on-disk)
#define PFS_VMDK_MAGIC_KDMV             0x564D444Bu   ///< "VMDK"
#define PFS_VMDK_MAGIC_COWD             0x44574F43u   ///< "COWD" (旧 ESX sparse, 本实现拒绝)

// 文本描述符常见首行标记
#define PFS_VMDK_DESC_MARK              "# Disk DescriptorFile"
#define PFS_VMDK_DESC_MARK_LEN          22

// SparseExtentHeader.flags (主机序)
#define PFS_VMDK_FLAG_NL_TEST           0x00000001u
#define PFS_VMDK_FLAG_RGD               0x00000002u
#define PFS_VMDK_FLAG_COMPRESSED        0x00010000u
#define PFS_VMDK_FLAG_MARKERS           0x00020000u
#define PFS_VMDK_FLAG_ZERO_GRAIN        0x00040000u

// Grain table entry 特殊值
#define PFS_VMDK_GTE_UNALLOC            0u
#define PFS_VMDK_GTE_ZEROED             1u

// 默认 numGTEsPerGT
#define PFS_VMDK_GTES_PER_GT_DEFAULT    512u

// 探测结果类别 (主机侧)
#define PFS_VMDK_KIND_NONE              0
#define PFS_VMDK_KIND_SPARSE            1   ///< 二进制 sparse extent (含 monolithicSparse)
#define PFS_VMDK_KIND_DESCRIPTOR        2   ///< 文本描述符

// 打开后工作模式
#define PFS_VMDK_MODE_NONE              0
#define PFS_VMDK_MODE_SPARSE            1   ///< 单文件 sparse (header+GD/GT+grains)
#define PFS_VMDK_MODE_FLAT              2   ///< 描述符 + FLAT/ZERO extent

// Extent 类型 (描述符)
#define PFS_VMDK_EXT_NONE               0
#define PFS_VMDK_EXT_FLAT               1
#define PFS_VMDK_EXT_SPARSE             2
#define PFS_VMDK_EXT_ZERO               3

#pragma pack(push, 1)

// SparseExtentHeader (512 字节); 多字节字段 on-disk 小端, 读入后转主机序使用本结构
typedef struct _pfs_vmdk_sparse_header_t {
    uint32_t magic;                 ///< PFS_VMDK_MAGIC_KDMV
    uint32_t version;               ///< 常见 1 / 2 / 3
    uint32_t flags;                 ///< PFS_VMDK_FLAG_*
    uint64_t capacity;              ///< [单位:扇区] 虚拟盘容量
    uint64_t grain_size;            ///< [单位:扇区] grain 大小 (常见 128)
    uint64_t descriptor_offset;     ///< [单位:扇区] 内嵌描述符; 0=无
    uint64_t descriptor_size;       ///< [单位:扇区]
    uint32_t num_gtes_per_gt;       ///< 每 GT 的 GTE 数 (常见 512)
    uint64_t rgd_offset;            ///< [单位:扇区] 冗余 GD; 0=无
    uint64_t gd_offset;             ///< [单位:扇区] Grain Directory
    uint64_t overhead;              ///< [单位:扇区] 元数据占用扇区数
    uint8_t  unclean_shutdown;
    uint8_t  single_end_line_char;
    uint8_t  non_end_line_char;
    uint8_t  double_end_line_char1;
    uint8_t  double_end_line_char2;
    uint16_t compress_algorithm;    ///< streamOptimized 用; 非压缩应为 0
    uint8_t  pad[433];
} pfs_vmdk_sparse_header_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_VMDK_RAW_H_

// end
