// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vmdk_internal.h
/// @brief   blkio_vmdk 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
// blkio_vmdk 内部 API; 仅供 pfs_vmdk_*.c
#ifndef _PFS_VMDK_INTERNAL_H_
#define _PFS_VMDK_INTERNAL_H_

#include "blkio_vmdk/pfs_vmdk.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- probe ---

// 探测 sparse magic 或文本描述符标记; 成功填 *p_kind / 可选 sparse 头 (主机序)
// 非 VMDK: PFS_BLKIO_RET_PROBE
int pfs_vmdk_probe(pfs_blkio_t *p_host, int *p_kind, pfs_vmdk_sparse_header_t *p_hdr);

// 从已 open 宿主读完整 SparseExtentHeader 并校验 (仅 KDMV); 成功 OK
int pfs_vmdk_probe_sparse_header(pfs_blkio_t *p_host, pfs_vmdk_sparse_header_t *p_hdr);

// --- path ---

int pfs_vmdk_path_dirname(const char *path, char *out, size_t out_sz);
int pfs_vmdk_path_join(const char *dir, const char *name, char *out, size_t out_sz);
int pfs_vmdk_path_is_abs(const char *path);

// --- descriptor ---

typedef struct _pfs_vmdk_desc_extent_t {
    int      type;                  ///< PFS_VMDK_EXT_*
    uint64_t sectors;
    uint64_t flat_offset_sectors;
    char     path[PFS_VMDK_PATH_MAX];
} pfs_vmdk_desc_extent_t;

typedef struct _pfs_vmdk_desc_t {
    pfs_vmdk_desc_extent_t extents[PFS_VMDK_EXTENT_MAX];
    int                    extent_count;
} pfs_vmdk_desc_t;

// 解析文本描述符; 仅收集 FLAT/ZERO/SPARSE extent 行
int pfs_vmdk_desc_parse(const char *text, size_t len, pfs_vmdk_desc_t *p_out);

// 从宿主 offset 处读描述符文本并解析 (字节偏移)
int pfs_vmdk_desc_load(pfs_blkio_t *p_host, int64_t byte_off, uint64_t byte_len,
        pfs_vmdk_desc_t *p_out);

// 按描述符打开 FLAT/ZERO extents 填入 priv (MODE_FLAT); 成功 OK
int pfs_vmdk_flat_open_from_desc(pfs_vmdk_priv_t *p_priv, const pfs_vmdk_desc_t *p_desc,
        pfs_blkio_oflags_e flags, int sector_size);

// --- grain (GD / GT) ---

void pfs_vmdk_grain_free(pfs_vmdk_priv_t *p_priv);
int pfs_vmdk_grain_load_gd(pfs_vmdk_priv_t *p_priv);
// grain_idx -> 宿主扇区号; UNALLOC/ZEROED 时 *p_sector 为特殊值
int pfs_vmdk_grain_lookup(pfs_vmdk_priv_t *p_priv, uint32_t grain_idx, uint32_t *p_sector);
// 写路径: 确保 grain 已分配 (可新建 GT/grain); 返回宿主扇区号
int pfs_vmdk_grain_ensure(pfs_vmdk_priv_t *p_priv, uint32_t grain_idx, uint32_t *p_sector);

// --- sparse I/O ---

int pfs_vmdk_sparse_open(pfs_vmdk_priv_t *p_priv, const pfs_vmdk_sparse_header_t *p_hdr);
int pfs_vmdk_sparse_pread(pfs_vmdk_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vmdk_sparse_pwrite(pfs_vmdk_priv_t *p_priv, const void *buf, int size, int64_t offset);

// --- flat I/O ---

void pfs_vmdk_flat_close_extents(pfs_vmdk_priv_t *p_priv);
int pfs_vmdk_flat_pread(pfs_vmdk_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vmdk_flat_pwrite(pfs_vmdk_priv_t *p_priv, const void *buf, int size, int64_t offset);
int pfs_vmdk_flat_fsync(pfs_vmdk_priv_t *p_priv);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VMDK_INTERNAL_H_

// end
