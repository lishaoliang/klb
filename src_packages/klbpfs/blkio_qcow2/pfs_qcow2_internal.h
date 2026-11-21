// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_qcow2_internal.h
/// @brief   blkio_qcow2 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
// blkio_qcow2 内部 API; 仅供 pfs_qcow2_*.c
#ifndef _PFS_QCOW2_INTERNAL_H_
#define _PFS_QCOW2_INTERNAL_H_

#include "blkio_qcow2/pfs_qcow2.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- probe ---

// 读宿主偏移 0 头并校验 magic; 成功填 p_hdr (主机序)
// 非 qcow 魔数: PFS_BLKIO_RET_PROBE; 已识别但不受支持: STATE
int pfs_qcow2_probe_header(pfs_blkio_t *p_host, pfs_qcow2_header_t *p_hdr);

// --- L1 ---

int pfs_qcow2_l1_load(pfs_qcow2_priv_t *p_priv);
void pfs_qcow2_l1_free(pfs_qcow2_priv_t *p_priv);
// 更新内存 L1[idx] 并写回宿主 (entry 为主机序含标志)
int pfs_qcow2_l1_set(pfs_qcow2_priv_t *p_priv, uint32_t idx, uint64_t entry);

// --- L2 ---

void pfs_qcow2_l2_cache_invalidate(pfs_qcow2_priv_t *p_priv);
// 按 L2 宿主偏移加载到缓存; offset==0 返回 STATE
int pfs_qcow2_l2_cache_load(pfs_qcow2_priv_t *p_priv, uint64_t l2_offset);
// 写回当前 L2 缓存到宿主
int pfs_qcow2_l2_cache_store(pfs_qcow2_priv_t *p_priv);
// 读 guest 簇对应 L2 entry (主机序); 未分配 L2 时 *p_entry=0 且返回 OK
int pfs_qcow2_l2_get(pfs_qcow2_priv_t *p_priv, uint64_t guest_off, uint64_t *p_entry);
// 确保可写 L2 (分配或 COW), 再写 entry; entry 为主机序
int pfs_qcow2_l2_set(pfs_qcow2_priv_t *p_priv, uint64_t guest_off, uint64_t entry);

// --- refcount ---

int pfs_qcow2_reftable_load(pfs_qcow2_priv_t *p_priv);
void pfs_qcow2_refcount_free(pfs_qcow2_priv_t *p_priv);
int pfs_qcow2_refcount_get(pfs_qcow2_priv_t *p_priv, uint64_t host_off, uint64_t *p_count);
int pfs_qcow2_refcount_set(pfs_qcow2_priv_t *p_priv, uint64_t host_off, uint64_t count);
// 分配 1 个宿主 cluster (填 0, refcount=1); 成功写 *p_host_off
int pfs_qcow2_alloc_cluster(pfs_qcow2_priv_t *p_priv, uint64_t *p_host_off);
// 宿主偏移起写 size 字节的 0 (可扩文件)
int pfs_qcow2_host_write_zeros(pfs_qcow2_priv_t *p_priv, int64_t host_off, uint64_t size);

// --- cluster I/O ---

int pfs_qcow2_cluster_pread(pfs_qcow2_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_qcow2_cluster_pwrite(pfs_qcow2_priv_t *p_priv, const void *buf, int size, int64_t offset);

#ifdef __cplusplus
}
#endif

#endif // _PFS_QCOW2_INTERNAL_H_

// end
