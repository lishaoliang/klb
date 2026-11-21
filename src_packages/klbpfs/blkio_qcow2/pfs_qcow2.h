// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_qcow2.h
/// @brief   qcow2 blkio 后端
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_QCOW2_H_
#define _PFS_QCOW2_H_

#include "pfs_blkio.h"
#include "blkio_qcow2/pfs_qcow2_raw.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// qcow2 后端私有态 (挂 pfs_blkio_t.priv); 宿主文件经 raw 打开
/// 子集: 单文件无 backing/压缩/加密/扩展 L2; 标准 cluster 读写 + 写时分配
typedef struct _pfs_qcow2_priv_t {
    pfs_blkio_t host;               ///< 宿主 .qcow2 文件 (g_pfs_blkio_raw_ops)
    uint32_t    version;            ///< 2 或 3
    uint64_t    virtual_size;       ///< [单位:字节] header.size
    uint32_t    cluster_bits;
    uint32_t    cluster_size;       ///< [单位:字节] 1 << cluster_bits
    uint32_t    l1_size;            ///< L1 表项数
    uint64_t    l1_table_offset;    ///< [单位:字节]
    uint64_t   *p_l1;               ///< L1 缓存 (主机序, 含 COPIED 等标志)
    uint64_t    refcount_table_offset; ///< [单位:字节]
    uint32_t    refcount_table_clusters;
    uint32_t    refcount_table_entries; ///< 表项数 (= clusters * cluster_size / 8)
    uint64_t   *p_reftable;         ///< 每项为 refcount block 起始偏移 (主机序, 已清标志)
    uint32_t    refcount_order;
    uint32_t    refcount_bits;      ///< 1 << refcount_order
    uint32_t    l2_entries;         ///< 每 L2 表项数 (= cluster_size / 8)
    uint32_t    refcount_block_entries;
    uint64_t    l2_cache_offset;    ///< [单位:字节] 当前缓存的 L2 宿主偏移; 0=无效
    uint64_t   *p_l2_cache;         ///< L2 缓存 (主机序)
    uint64_t    rb_cache_offset;    ///< [单位:字节] 当前缓存的 refcount block; 0=无效
    uint8_t    *p_rb_cache;         ///< refcount block 原始字节 (大端项)
    int64_t     host_file_size;     ///< [单位:字节] 宿主文件大小缓存
    int         allow_write;        ///< 非 0 允许 pwrite
} pfs_qcow2_priv_t;

/// 静态 ops 表 (facade 探测后可绑此表); open_direct 为 NULL
extern const pfs_blkio_ops_t g_pfs_qcow2_ops;

/// 打开 qcow2 镜像; 经 raw 宿主; 子集 v2/v3 无 backing/压缩/加密
int pfs_qcow2_open(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);

/// 关闭并释放 L1/L2/refcount 缓存与宿主
int pfs_qcow2_close(pfs_blkio_t *p_blkio);

/// 按虚拟偏移读; 未分配簇读 0
int pfs_qcow2_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);

/// 按虚拟偏移写; 写时分配; 只读打开返回 STATE
int pfs_qcow2_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);

/// 宿主文件 fsync
int pfs_qcow2_fsync(pfs_blkio_t *p_blkio);

/// 虚拟盘容量 [单位:字节]
int64_t pfs_qcow2_get_size(const pfs_blkio_t *p_blkio);

/// 逻辑扇区大小 (固定 512)
int pfs_qcow2_logical_sector_size(const pfs_blkio_t *p_blkio);

/// 创建空 qcow2 v3 (覆盖已存在文件); size_bytes=虚拟盘容量且须 512 对齐
/// cluster 64KiB; refcount_order=4; 无 backing/快照/加密
int pfs_qcow2_create(const char *path, uint64_t size_bytes);

#ifdef __cplusplus
}
#endif

#endif // _PFS_QCOW2_H_

// end
