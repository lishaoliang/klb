// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhdx_internal.h
/// @brief   blkio_vhdx 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
// blkio_vhdx 内部 API; 仅供 pfs_vhdx_*.c
#ifndef _PFS_VHDX_INTERNAL_H_
#define _PFS_VHDX_INTERNAL_H_

#include "blkio_vhdx/pfs_vhdx.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// CRC-32C (Castagnoli); crc_offset>=0 时该 4 字节按 0 参与计算
uint32_t pfs_vhdx_crc32c(uint32_t crc, const uint8_t *buf, size_t size);
int pfs_vhdx_crc32c_valid(uint8_t *buf, size_t size, int crc_offset);

// GUID 比较 (on-disk LE 布局已转主机序字段)
int pfs_vhdx_guid_eq(const pfs_vhdx_guid_t *a, const pfs_vhdx_guid_t *b);
void pfs_vhdx_guid_from_raw(const uint8_t *p, pfs_vhdx_guid_t *out);
void pfs_vhdx_guid_to_raw(uint8_t *p, const pfs_vhdx_guid_t *g);
void pfs_vhdx_crc_store(uint8_t *buf, size_t size, int crc_off);

// 已知 region / metadata GUID (主机序)
const pfs_vhdx_guid_t *pfs_vhdx_guid_bat(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_metadata(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_file_params(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_virtual_size(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_logical_sector(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_physical_sector(void);
const pfs_vhdx_guid_t *pfs_vhdx_guid_parent_locator(void);

int pfs_vhdx_guid_is_zero(const pfs_vhdx_guid_t *g);
void pfs_vhdx_guid_make(pfs_vhdx_guid_t *g);

// 探测并填充 priv 几何 + BAT (host 须已 open; open_flags 已填); 成功 OK
// 非 VHDX: PROBE; Differencing / 无法回放的脏 log: STATE
int pfs_vhdx_probe_open(pfs_vhdx_priv_t *p_priv);

void pfs_vhdx_bat_free(pfs_vhdx_priv_t *p_priv);
int pfs_vhdx_bat_load(pfs_vhdx_priv_t *p_priv);
int pfs_vhdx_bat_set_entry(pfs_vhdx_priv_t *p_priv, uint32_t bat_idx, uint64_t entry);

// 写两份 header (递增 sequence); 使用 priv 内 guid/log 字段
int pfs_vhdx_headers_write(pfs_vhdx_priv_t *p_priv);

// 最小 log: 一次提交对 file_offset 处 4KiB 的更新 (先记 log 再落盘, 成功后清 log_guid)
int pfs_vhdx_log_commit_4k(pfs_vhdx_priv_t *p_priv, uint64_t file_offset, const uint8_t *data_4k);
// 打开时: log_guid 非 0 则尝试回放 log 起始处的一条 entry 并清 guid
int pfs_vhdx_log_replay_open(pfs_vhdx_priv_t *p_priv);

// Fixed/Dynamic 共用 payload 读/写
int pfs_vhdx_payload_pread(pfs_vhdx_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhdx_payload_pwrite(pfs_vhdx_priv_t *p_priv, const void *buf, int size, int64_t offset);

int pfs_vhdx_fixed_pread(pfs_vhdx_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhdx_fixed_pwrite(pfs_vhdx_priv_t *p_priv, const void *buf, int size, int64_t offset);

int pfs_vhdx_dynamic_pread(pfs_vhdx_priv_t *p_priv, void *buf, int size, int64_t offset);
int pfs_vhdx_dynamic_pwrite(pfs_vhdx_priv_t *p_priv, const void *buf, int size, int64_t offset);

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHDX_INTERNAL_H_

// end
