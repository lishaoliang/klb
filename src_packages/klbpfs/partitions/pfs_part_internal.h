// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_part_internal.h
/// @brief   partitions 模块内部辅助
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    模块内部头, 非稳定公开 API
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_PART_INTERNAL_H_
#define _PFS_PART_INTERNAL_H_

#include "pfs_part.h"

#ifdef __cplusplus
extern "C" {
#endif

// 模块内返回值 (数值对齐 pfs_ret_e; 仅 partitions/ 与探测路径使用)
// 对外 pfs_part_probe 须映到 / 直接返回 PFS_* (pfs_ret_e)
typedef enum pfs_part_ret_e {
    PFS_PART_RET_OK         =  0,   ///< == PFS_OK
    PFS_PART_RET_EIO        = -5,   ///< == PFS_EIO
    PFS_PART_RET_ENOEXEC    = -8,   ///< == PFS_ENOEXEC
    PFS_PART_RET_ENOMEM     = -12,  ///< == PFS_ENOMEM
    PFS_PART_RET_EINVAL     = -22,  ///< == PFS_EINVAL
    PFS_PART_RET_EBADMSG    = -74,  ///< == PFS_EBADMSG
    PFS_PART_RET_EOPNOTSUPP = -95,  ///< == PFS_EOPNOTSUPP
} pfs_part_ret_e;

////////////////////////////////////////////////////////////
// 内部探测 (仅 partitions/ 内互调); 对齐 linux-7.1 efi_partition / msdos_partition
// 成功 PFS_PART_RET_OK 且已填 *p_out; 非本 scheme 为 PFS_PART_RET_ENOEXEC
// GPT 损坏 (如 CRC) 可为 EBADMSG; I/O 为 EIO
int pfs_part_gpt_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out);
int pfs_part_mbr_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out);

/// @brief 将 [first_usable, last_usable] 均分为 count 段 (按 align_lba 对齐 chunk)
int pfs_part_mkpt_split_even(uint64_t first_usable, uint64_t last_usable, uint64_t align_lba,
        int count, uint64_t *p_starts, uint64_t *p_sizes);

#ifdef __cplusplus
}
#endif

#endif // _PFS_PART_INTERNAL_H_

// end
