// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhd_internal.h
/// @brief   blkio_vhd 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
// blkio_vhd 内部 API; 仅供 pfs_vhd_*.c / pfs_vhd.c
#ifndef _PFS_VHD_INTERNAL_H_
#define _PFS_VHD_INTERNAL_H_

#include "blkio_vhd/pfs_vhd.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- probe: footer / cookie / dyn header ---

// 从已 open 的宿主 raw 读文件尾 footer; 成功 PFS_BLKIO_RET_OK
// 非 VHD / 校验失败: PFS_BLKIO_RET_PROBE; 参数非法: PARAM; I/O: IO
int pfs_vhd_probe_footer(pfs_blkio_t *p_host, pfs_vhd_footer_t *p_footer);

// 读 Dynamic/Differencing 头 (footer.data_offset); 成功 OK; 否则 PROBE/IO/PARAM
int pfs_vhd_probe_dyn_header(pfs_blkio_t *p_host, uint64_t data_offset, pfs_vhd_dyn_header_t *p_hdr);

// --- bat: Block Allocation Table (Dynamic / Differencing) ---

// 按 priv.table_offset / max_table_entries 加载 BAT 到 priv.p_bat; 成功 OK
int pfs_vhd_bat_load(pfs_vhd_priv_t *p_priv);

// 释放 priv.p_bat
void pfs_vhd_bat_free(pfs_vhd_priv_t *p_priv);

// block_idx -> 数据块起始扇区号 (相对宿主文件); UNUSED 时 *p_sector = PFS_VHD_BAT_ENTRY_UNUSED
// 成功 OK; 未 load / 越界: STATE / PARAM
int pfs_vhd_bat_lookup(const pfs_vhd_priv_t *p_priv, uint32_t block_idx, uint32_t *p_sector);

// 更新内存 BAT 并写回宿主 (sector 为主机序绝对扇区号)
int pfs_vhd_bat_set(pfs_vhd_priv_t *p_priv, uint32_t block_idx, uint32_t sector);

// Dynamic: 每块 sector bitmap 占用扇区数
uint32_t pfs_vhd_dyn_bitmap_sectors(uint32_t block_size);

// --- fixed: 线性映射 ---

int pfs_vhd_fixed_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhd_fixed_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset);

// --- dynamic: BAT + 块内 bitmap ---

int pfs_vhd_dynamic_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhd_dynamic_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset);

// --- differencing: 父子链 (二期) ---

int pfs_vhd_diff_pread(pfs_vhd_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhd_diff_pwrite(pfs_vhd_priv_t *p_priv, const void *buf, int size, int64_t offset);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHD_INTERNAL_H_

// end
