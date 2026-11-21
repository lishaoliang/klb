// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_qcow2_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_QCOW2_RAW_H_
#define _PFS_QCOW2_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// QEMU qcow2 on-disk 布局子集; 多字节字段为大端
// 参考: QEMU docs/interop/qcow2.txt

#define PFS_QCOW2_MAGIC                 0x514649FBu   ///< "QFI\xfb"
#define PFS_QCOW2_VERSION_2             2u
#define PFS_QCOW2_VERSION_3             3u

#define PFS_QCOW2_SECTOR_SIZE           512
#define PFS_QCOW2_HEADER_SIZE_MIN       72
#define PFS_QCOW2_HEADER_SIZE_V3        104

// header.crypt_method
#define PFS_QCOW2_CRYPT_NONE            0u
#define PFS_QCOW2_CRYPT_AES             1u
#define PFS_QCOW2_CRYPT_LUKS            2u

// incompatible_features (v3+)
#define PFS_QCOW2_INCOMPAT_DIRTY        (1ull << 0)
#define PFS_QCOW2_INCOMPAT_CORRUPT      (1ull << 1)
#define PFS_QCOW2_INCOMPAT_EXTDATA      (1ull << 2)
#define PFS_QCOW2_INCOMPAT_COMPRESS     (1ull << 3)
#define PFS_QCOW2_INCOMPAT_EXTL2        (1ull << 4)

// L1/L2/refcount 偏移与标志 (主机序位定义)
#define PFS_QCOW2_OFLAG_COPIED          (1ull << 63)
#define PFS_QCOW2_OFLAG_COMPRESSED      (1ull << 62)
#define PFS_QCOW2_OFLAG_ZERO            (1ull << 0)
#define PFS_QCOW2_OFFSET_MASK           0x00fffffffffffe00ull

// cluster_bits 允许范围 (512B..2MiB)
#define PFS_QCOW2_CLUSTER_BITS_MIN      9u
#define PFS_QCOW2_CLUSTER_BITS_MAX      21u

// L1 表字节上限 (对齐 QEMU 实现限)
#define PFS_QCOW2_L1_SIZE_MAX_BYTES     (32u * 1024u * 1024u)
// refcount 表字节上限
#define PFS_QCOW2_REFTABLE_SIZE_MAX_BYTES (8u * 1024u * 1024u)

#pragma pack(push, 1)

// qcow2 文件头; v2 有效到 72 字节; v3 至少再含下列字段到 header_length
typedef struct _pfs_qcow2_header_t {
    uint32_t magic;                 ///< PFS_QCOW2_MAGIC
    uint32_t version;               ///< 2 或 3
    uint64_t backing_file_offset;   ///< [单位:字节]
    uint32_t backing_file_size;     ///< [单位:字节]
    uint32_t cluster_bits;
    uint64_t size;                  ///< 虚拟盘容量 [单位:字节]
    uint32_t crypt_method;
    uint32_t l1_size;               ///< L1 表项数
    uint64_t l1_table_offset;       ///< [单位:字节]
    uint64_t refcount_table_offset; ///< [单位:字节]
    uint32_t refcount_table_clusters;
    uint32_t nb_snapshots;
    uint64_t snapshots_offset;      ///< [单位:字节]
    // v3+ (version>=3 时由 probe 填充; v2 视为 0 / 默认)
    uint64_t incompatible_features;
    uint64_t compatible_features;
    uint64_t autoclear_features;
    uint32_t refcount_order;        ///< refcount_bits = 1 << order; v2 固定为 4
    uint32_t header_length;         ///< [单位:字节]; v2 为 72
} pfs_qcow2_header_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_QCOW2_RAW_H_

// end
