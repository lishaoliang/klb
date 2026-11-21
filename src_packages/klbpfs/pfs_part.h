// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_part.h
/// @brief   分区表对外接口 (MBR/GPT 探测与 mkpt)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    分册头, 亦经 pfs.h 聚合; 实现位于 src/partitions/
///  \n 应用优先 #include "pfs.h"; 亦可单独 #include "pfs_part.h"
///  \n 错误码与其它 src 根对外 API 一致, 返回 pfs_ret_e (PFS_OK / PFS_*)
///  \n 职责: 探测盘上分区布局, 解析分区起止供 mount/mkfs 使用
///  \n 布局/探测对齐 linux-7.1 block/partitions (efi 先于 msdos)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_PART_H_
#define _PFS_PART_H_

#include "pfs_ops.h"
#include "pfs_blkio.h"
#include <stdint.h>
#include <stdbool.h>    /// bool false true

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @def   PFS_PART_ENTRY_MAX
/// @brief 分区表最大项数
#define PFS_PART_ENTRY_MAX  128

/// @brief 分区表类型
typedef enum _pfs_part_scheme_e {
    PFS_PART_SCHEME_NONE = 0,   ///< 未识别 / 整盘无分区表
    PFS_PART_SCHEME_MBR  = 1,   ///< DOS/MBR
    PFS_PART_SCHEME_GPT  = 2,   ///< GPT (含 Protective MBR)
} pfs_part_scheme_e;

/// @brief probe / mkpt 输出: 盘上分区表结果
typedef struct _pfs_part_entry_t {
    uint64_t start_lba;     ///< [单位:扇区] 相对整盘 0
    uint64_t size_lba;      ///< [单位:扇区]
    uint8_t  mbr_sys_ind;   ///< MBR sys_ind; GPT 时为 0
    bool     bootable;      ///< true 表示可引导 (MBR boot_ind / GPT attr)
} pfs_part_entry_t;

/// @brief 分区表结果
typedef struct _pfs_part_table_t {
    pfs_part_scheme_e scheme;   ///< 分区表类型
    int               count;    ///< entries 有效项数
    pfs_part_entry_t  entries[PFS_PART_ENTRY_MAX]; ///< 分区表项
} pfs_part_table_t;

/// @brief mkpt 输入: 做分区表请求 (与输出 pfs_part_table_t 分离)
typedef struct _pfs_mkpt_entry_t {
    uint64_t        start_lba;      ///< 起始 LBA (0 可由实现自算)
    uint64_t        size_lba;       ///< 大小 (0 可由实现自算)
    uint8_t         mbr_sys_ind;    ///< MBR sys_ind; GPT 忽略
                                    // MBR: 非 0 用调用方值; 0 则按 fs_type 映射
    bool            bootable;       ///< true 表示可引导
    pfs_fs_type_e   fs_type;        ///< 该分区期望 FS (可各不同)
} pfs_mkpt_entry_t;

/// @brief mkpt 输入: 做分区表请求 (与输出 pfs_part_table_t 分离)
typedef struct _pfs_mkpt_table_t {
    int               count;        ///< 有效项数
    pfs_mkpt_entry_t  entries[PFS_PART_ENTRY_MAX];
} pfs_mkpt_table_t;

/// @brief 探测盘上分区表
/// @param [in]  *p_blkio    已 open 的块设备句柄
/// @param [out] *p_out      探测结果
/// @return int PFS_OK 成功; PFS_EINVAL 参数非法; PFS_EIO I/O 失败; PFS_EBADMSG 表损坏等
/// @note 探测序: GPT -> MBR; 皆非则 scheme=NONE 且返回 PFS_OK
///  \n GPT 读: 阶段 A+B (PMBR + 主 LBA1, 失败回退 alternate_lba/盘尾)
PFS_API int pfs_part_probe(pfs_blkio_t *p_blkio, pfs_part_table_t *p_out);

/// @brief 做 MBR 分区表 (count 1..4; 毁写 LBA0; 均分 1MiB 对齐数据区)
/// @param [in]  *p_in           count 1..4; entries[i].bootable/mbr_sys_ind/fs_type
/// @note mbr_sys_ind: 非 0 按指定; 0 则按 fs_type; count>4 时 PFS_EOPNOTSUPP
PFS_API int pfs_mbr_mkpt(pfs_blkio_t *p_blkio, uint64_t total_bytes, const pfs_mkpt_table_t *p_in, pfs_part_table_t *p_out);

/// @brief 做 GPT 分区表 (count>=1; 毁写 PMBR+主备 GPT; 均分可用区)
/// @param [in]  *p_in           同 pfs_mbr_mkpt; mbr_sys_ind 忽略
/// @note 分区名 pfs01..; 类型 GUID 现 Basic Data
PFS_API int pfs_gpt_mkpt(pfs_blkio_t *p_blkio, uint64_t total_bytes, const pfs_mkpt_table_t *p_in, pfs_part_table_t *p_out);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_PART_H_

// end
