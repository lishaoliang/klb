// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vdi_internal.h
/// @brief   blkio_vdi 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VDI_INTERNAL_H_
#define _PFS_VDI_INTERNAL_H_

#include "blkio_vdi/pfs_vdi.h"

#ifdef __cplusplus
extern "C" {
#endif

// 读 pre-header+头, 校验 signature/版本/类型; 成功填 p_hdr (主机序)
// 非 VDI: PFS_BLKIO_RET_PROBE; 已识别但不受支持: STATE
int pfs_vdi_probe_header(pfs_blkio_t *p_host, pfs_vdi_header_t *p_hdr);

void pfs_vdi_map_free(pfs_vdi_priv_t *p_priv);
int pfs_vdi_map_load(pfs_vdi_priv_t *p_priv);
int pfs_vdi_map_set(pfs_vdi_priv_t *p_priv, uint32_t block_idx, uint32_t entry);
int pfs_vdi_header_set_blocks_allocated(pfs_vdi_priv_t *p_priv);

// 逻辑字节偏移 -> 宿主; 未分配读填 0; 写时分配 (DYNAMIC)
int pfs_vdi_io_pread(pfs_vdi_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vdi_io_pwrite(pfs_vdi_priv_t *p_priv, const void *buf, int size, int64_t offset);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VDI_INTERNAL_H_

// end
