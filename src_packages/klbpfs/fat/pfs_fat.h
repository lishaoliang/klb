// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_fat.h
/// @brief   FAT 子模块对外接口 (sbi / mount / mkfs / vfs 对接)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_FAT_H_
#define _PFS_FAT_H_

#include "pfs_blkio.h"
#include "pfs_fat_raw.h"
#include "base/pfs_cache_cluster.h"
#include "base/pfs_cache_sector.h"
#include "base/pfs_fs_ops.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief FAT API 返回值 (失败时 <0; 勿透传 PFS_BLKIO_RET_*)
/// @note 命名/数值对齐 linux-7.1 -errno (与 pfs_ret_e 同值); blkio 失败映射为 PFS_FAT_RET_EIO
typedef enum _pfs_fat_ret_e {
    PFS_FAT_RET_OK            =  0,                     ///< 成功
    PFS_FAT_RET_EPERM         = -1,                     ///< 只读卷/属性禁止写 (EPERM)
    PFS_FAT_RET_ENOENT        = -2,                     ///< 路径或目录项不存在 (ENOENT)
    PFS_FAT_RET_EIO           = -5,                     ///< I/O 失败 (EIO; blkio 层)
    PFS_FAT_RET_ENOEXEC       = -8,                     ///< boot/BPB 无效, 非 FAT (ENOEXEC)
    PFS_FAT_RET_ENOMEM        = -12,                    ///< 内存不足 (ENOMEM)
    PFS_FAT_RET_EBUSY         = -16,                    ///< mount 生命周期/状态不符 (EBUSY)
    PFS_FAT_RET_EEXIST        = -17,                    ///< 目标已存在 (EEXIST)
    PFS_FAT_RET_ENOTDIR       = -20,                    ///< 路径组件非目录 (ENOTDIR)
    PFS_FAT_RET_EISDIR        = -21,                    ///< 目标是目录 (EISDIR)
    PFS_FAT_RET_EINVAL        = -22,                    ///< 参数非法 (EINVAL)
    PFS_FAT_RET_ENOSPC        = -28,                    ///< 无空闲簇/目录项槽 (ENOSPC)
    PFS_FAT_RET_EROFS         = -30,                    ///< 只读文件系统 (EROFS)
    PFS_FAT_RET_ERANGE        = -34,                    ///< 簇号/FAT 链越界 (ERANGE)
    PFS_FAT_RET_ENAMETOOLONG  = -36,                    ///< 文件名超长 (ENAMETOOLONG)
    PFS_FAT_RET_ENOTEMPTY     = -39,                    ///< 目录非空 (ENOTEMPTY)
    PFS_FAT_RET_EBADMSG       = -74,                    ///< on-disk 元数据损坏 (EBADMSG)
    PFS_FAT_RET_EILSEQ        = -84,                    ///< 路径/字符集非法 (EILSEQ)
    PFS_FAT_RET_EOPNOTSUPP    = -95,                    ///< 未实现或不支持 (EOPNOTSUPP)
} pfs_fat_ret_e;

/// @brief mount 模式 (互斥; 仅 pfs_fat_sbi_t.mount_state; 非组合 bit)
typedef enum _pfs_fat_mount_state_e {
    PFS_FAT_UNMOUNTED         = 0x00,                   ///< 未挂载
    PFS_FAT_MOUNT_RDONLY      = 0x01,                   ///< 已挂载, 只读
    PFS_FAT_MOUNT_RDWR        = 0x02,                   ///< 已挂载, 读写
} pfs_fat_mount_state_e;

/// @brief BPB 解析 + 卷几何 (对齐 linux-7.1 fat_bios_param_block + msdos_sb_info 几何子集)
/// @note [单位:扇区]=卷内相对扇区; [单位:字节]; [单位:簇]=簇号或簇个数
typedef struct _pfs_fat_bs_t {
    fat_boot_sector_t   boot;                           ///< 引导扇区副本; 多字节字段已转主机序

    uint16_t            sector_size;                    ///< [单位:字节] 逻辑扇区大小
    uint8_t             sec_per_clus;                   ///< 每簇扇区数
    uint16_t            reserved;                       ///< [单位:扇区] 保留扇区 (= fat_start)
    uint16_t            sectors16;                      ///< [单位:扇区] BPB 16 位总扇区
    uint32_t            total_sect32;                   ///< [单位:扇区] BPB total_sect
    uint32_t            total_sectors;                  ///< [单位:扇区] 卷总扇区
    uint8_t             media;                          ///< 介质描述符

    uint32_t            cluster_size;                   ///< [单位:字节] 簇大小
    uint32_t            cluster_size_bits;              ///< log2(cluster_size)
    uint32_t            max_cluster;                    ///< [单位:簇] 最大簇号 (不含, 同 linux max_cluster)
    uint8_t             fat_bits;                       ///< 12 / 16 / 32 (linux: sbi->fat_bits)
    uint32_t            root_cluster;                   ///< [单位:簇] 根目录起始簇 (FAT32)

    uint8_t             fats;                           ///< FAT 表份数
    uint16_t            fat_length16;                   ///< [单位:扇区] BPB fat_length (FAT32 为 0)
    uint32_t            fat32_length;                   ///< [单位:扇区] FAT32 每 FAT 扇区数
    uint32_t            fat_start;                      ///< [单位:扇区] FAT1 首扇区 (= reserved)
    uint32_t            fat_length;                     ///< [单位:扇区] 实际每 FAT 扇区数
    uint16_t            fat32_flags;                    ///< FAT32 BPB flags

    uint16_t            dir_entries;                    ///< 根目录项数 (FAT32 为 0)
    uint32_t            dir_start;                      ///< [单位:扇区] 根目录区首扇区 (FAT12/16)
    uint32_t            rootdir_sectors;                ///< [单位:扇区] 固定根目录占用 (FAT32 为 0)
    uint32_t            data_start;                     ///< [单位:扇区] 数据区首扇区
    uint16_t            info_sector;                    ///< [单位:扇区] FSINFO 扇区号
    uint16_t            backup_boot;                    ///< [单位:扇区] 备份引导扇区
    uint32_t            dir_per_block;                  ///< 每逻辑扇区目录项数
    uint32_t            dir_per_block_bits;             ///< log2(dir_per_block)

    uint8_t             state;                          ///< BPB state (FAT_STATE_DIRTY)
    uint32_t            vol_id;                         ///< 卷序列号
    uint8_t             vol_label[FAT_MSDOS_NAME];      ///< 卷标
    uint8_t             fs_type[8];                     ///< 如 "FAT32   "
    uint8_t             dirty;                          ///< 1=挂载前卷脏
} pfs_fat_bs_t;

/// @brief FAT32 FSINFO 解析结果
typedef struct _pfs_fat_fsinfo_t {
    bool                valid;                          ///< true=签名合法且已读入
    uint32_t            free_clusters;                  ///< 空闲簇数, -1 表示未知
    uint32_t            next_cluster;                   ///< 最近分配簇提示
} pfs_fat_fsinfo_t;

/// @brief FAT1 表元数据 + 扇区缓存 (覆盖 FAT12/16/32)
/// @note fat_cache: mount 时仅分配槽; 读盘在 fat_cache_get (按需)
typedef struct _pfs_fat_fat_t {
    bool                valid;                          ///< true=已按 BPB 初始化
    uint64_t            start_sector;                   ///< [单位:扇区] FAT1 首扇区 LBA (相对卷首)
    uint32_t            num_sectors;                    ///< [单位:扇区] 单个 FAT 占用扇区数
    uint64_t            size;                           ///< [单位:字节] FAT 字节长度
    uint8_t             fat_bits;                       ///< 12 / 16 / 32 (与 bs.fat_bits 一致)

    pfs_cache_sector_t  fat_cache;                      ///< FAT 扇区高热缓存
} pfs_fat_fat_t;

/// @brief FAT superblock info (对齐 linux-7.1 msdos_sb_info 用户态子集)
typedef struct _pfs_fat_sbi_t {
    pfs_fat_mount_state_e mount_state;                  ///< UNMOUNTED / MOUNT_RDONLY / MOUNT_RDWR

    pfs_blkio_t         *p_blkio;                       ///< 借用块设备句柄
    int64_t             part_offset;                    ///< [单位:字节] 分区相对设备偏移; 裸卷/整盘为 0

    pfs_fat_bs_t        bs;                             ///< BPB / 几何
    pfs_fat_fsinfo_t    fsinfo;                         ///< FSINFO (仅 FAT32 有意义)
    pfs_fat_fat_t       fat;                            ///< FAT1 几何 + fat_cache

    uint32_t            free_clusters;                  ///< 空闲簇缓存; -1 未知
    uint32_t            prev_free;                      ///< 上次分配簇提示
    uint8_t             free_clus_valid;                ///< 1=free_clusters 有效

    // 目录缓存: init 只分配槽; get 才读盘 (对齐 exFAT dir_cache 思路)
    pfs_cache_cluster_t dir_cache;                      ///< FAT32 根 + 所有子目录 (按簇)
    pfs_cache_sector_t  root_sec_cache;                 ///< FAT12/16 固定根 (按扇区)
    uint8_t             root_sec_cache_on;              ///< 1=root_sec_cache 已 init
} pfs_fat_sbi_t;

/// @brief vfs 对接表 (pfs_fat_ops.c; 12/16/32 分表; 已接线)
extern const pfs_fs_ops_t g_pfs_fat12_fs_ops;
extern const pfs_fs_ops_t g_pfs_fat16_fs_ops;
extern const pfs_fs_ops_t g_pfs_fat32_fs_ops;

/// @brief 重置为 UNMOUNTED
/// @param [in, out] *p_sbi    FAT sbi
/// @return int PFS_FAT_RET_OK 成功; PFS_FAT_RET_EINVAL 失败
int pfs_fat_init(pfs_fat_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
/// @param [in, out] *p_sbi    FAT sbi
void pfs_fat_deinit(pfs_fat_sbi_t *p_sbi);

/// @brief 读取并解析 boot/BPB
/// @param [in, out] *p_sbi        FAT sbi
/// @param [in]      *p_blkio       须已 open
/// @param [in]      part_offset    分区字节偏移
/// @param [in]      mount_mode     PFS_FAT_MOUNT_RDONLY 或 PFS_FAT_MOUNT_RDWR
/// @return int PFS_FAT_RET_OK 成功; 失败保持 UNMOUNTED
/// @note PFS_FAT_RET_EINVAL / PFS_FAT_RET_EBUSY / PFS_FAT_RET_EIO / PFS_FAT_RET_ENOEXEC / PFS_FAT_RET_ENOMEM / PFS_FAT_RET_EBADMSG / PFS_FAT_RET_EOPNOTSUPP
int pfs_fat_mount(pfs_fat_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_fat_mount_state_e mount_mode);

/// @brief 已挂载时切换只读/读写; 不重新解析 BPB
/// @param [in, out] *p_sbi        FAT sbi
/// @param [in]      mount_mode     仅 RDONLY/RDWR
/// @return int PFS_FAT_RET_OK 成功; PFS_FAT_RET_EBUSY / PFS_FAT_RET_EINVAL
int pfs_fat_remount(pfs_fat_sbi_t *p_sbi, pfs_fat_mount_state_e mount_mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
/// @param [in, out] *p_sbi    FAT sbi
/// @return int PFS_FAT_RET_OK 成功; PFS_FAT_RET_EINVAL / PFS_FAT_RET_EBUSY
int pfs_fat_umount(pfs_fat_sbi_t *p_sbi);

/// @brief 仅探测是否为 FAT12/16/32; 不修改 p_sbi
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移
/// @return int PFS_FAT_RET_OK 成功; PFS_FAT_RET_EINVAL / PFS_FAT_RET_EIO / PFS_FAT_RET_ENOEXEC / PFS_FAT_RET_EOPNOTSUPP
int pfs_fat_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 在 p_blkio 指定分区写入 FAT12 布局
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移 (裸卷为 0)
/// @param [in] vol_bytes      卷容量 [单位:字节], 须为扇区整数倍
/// @return int PFS_FAT_RET_OK 成功; PFS_FAT_RET_EINVAL / PFS_FAT_RET_EIO / PFS_FAT_RET_ENOMEM / PFS_FAT_RET_EBADMSG / PFS_FAT_RET_EOPNOTSUPP
int pfs_fat_mkfs12(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 在 p_blkio 指定分区写入 FAT16 布局
int pfs_fat_mkfs16(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 在 p_blkio 指定分区写入 FAT32 布局
int pfs_fat_mkfs32(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_FAT_H_

// end
