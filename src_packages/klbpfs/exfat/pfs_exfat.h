// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_exfat.h
/// @brief   exFAT 子模块对外接口 (sbi / mount / mkfs / vfs 对接)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXFAT_H_
#define _PFS_EXFAT_H_

#include "base/pfs_buf.h"
#include "base/pfs_cache_cluster.h"
#include "base/pfs_cache_sector.h"
#include "exfat/pfs_exfat_raw.h"
#include "base/pfs_fs_ops.h"
#include "pfs_blkio.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @def   PFS_EXFAT_UPCASE_BUF_SIZE
/// @brief 展开后 Up-case 表固定缓冲 (0x10000 项 * 2 字节 = 128KiB; 对齐 linux UTBL_COUNT)
#define PFS_EXFAT_UPCASE_BUF_SIZE       (128 * 1024)

/// @def   PFS_EXFAT_UPCASE_ENTRIES
/// @brief Up-case 表项数
#define PFS_EXFAT_UPCASE_ENTRIES        (PFS_EXFAT_UPCASE_BUF_SIZE / (int)sizeof(uint16_t))

/// @brief exFAT API 返回值 (失败时 <0; 勿透传 PFS_BLKIO_RET_*)
/// @note 命名/数值对齐 linux-7.1 -errno (与 pfs_ret_e 同值, vfs 可透传); blkio 失败映射为 PFS_EXFAT_RET_EIO
typedef enum _pfs_exfat_ret_e {
    PFS_EXFAT_RET_OK            =  0,                   ///< 成功
    PFS_EXFAT_RET_EPERM         = -1,                   ///< 只读卷/属性禁止写 (EPERM)
    PFS_EXFAT_RET_ENOENT        = -2,                   ///< 路径或目录项不存在 (ENOENT)
    PFS_EXFAT_RET_EIO           = -5,                   ///< I/O 失败 (EIO; blkio 层)
    PFS_EXFAT_RET_ENOEXEC       = -8,                   ///< boot/super 无效, 非 exFAT (ENOEXEC)
    PFS_EXFAT_RET_ENOMEM        = -12,                  ///< 内存不足 (ENOMEM)
    PFS_EXFAT_RET_EBUSY         = -16,                  ///< mount 生命周期/状态不符 (EBUSY)
    PFS_EXFAT_RET_EEXIST        = -17,                  ///< 目标已存在 (EEXIST)
    PFS_EXFAT_RET_ENOTDIR       = -20,                  ///< 路径组件非目录 (ENOTDIR)
    PFS_EXFAT_RET_EISDIR        = -21,                  ///< 目标是目录 (EISDIR)
    PFS_EXFAT_RET_EINVAL        = -22,                  ///< 参数非法 (EINVAL)
    PFS_EXFAT_RET_ENOSPC        = -28,                  ///< 无空闲簇/目录项槽 (ENOSPC)
    PFS_EXFAT_RET_EROFS         = -30,                  ///< 只读文件系统 (EROFS)
    PFS_EXFAT_RET_ERANGE        = -34,                  ///< 簇号/FAT 链越界 (ERANGE)
    PFS_EXFAT_RET_ENAMETOOLONG  = -36,                  ///< 文件名超长 (ENAMETOOLONG)
    PFS_EXFAT_RET_ENOTEMPTY     = -39,                  ///< 目录非空 (ENOTEMPTY)
    PFS_EXFAT_RET_EBADMSG       = -74,                  ///< on-disk 元数据损坏 (EBADMSG)
    PFS_EXFAT_RET_EILSEQ        = -84,                  ///< 路径/字符集非法 (EILSEQ)
    PFS_EXFAT_RET_EOPNOTSUPP    = -95,                  ///< 未实现或不支持 (EOPNOTSUPP)
} pfs_exfat_ret_e;

/// @brief mount 模式 (互斥; 仅 pfs_exfat_sbi_t.mount_state; 非组合 bit)
typedef enum _pfs_exfat_mount_state_e {
    PFS_EXFAT_UNMOUNTED         = 0x00,                 ///< 未挂载
    PFS_EXFAT_MOUNT_RDONLY      = 0x01,                 ///< 已挂载, 只读
    PFS_EXFAT_MOUNT_RDWR        = 0x02,                 ///< 已挂载, 读写
} pfs_exfat_mount_state_e;

/// @brief exFAT boot sector 解析结果 (几何对齐 linux-7.1 exfat_sb_info)
/// @note [单位:扇区]=卷内相对扇区; [单位:字节]; [单位:簇]=簇号或簇个数
typedef struct _pfs_exfat_bs_t {
    exfat_boot_sector_t boot;                             ///< 引导扇区副本; 多字节字段已转主机序

    uint64_t            num_sectors;                      ///< [单位:扇区]卷扇区总数 (boot.vol_length)
    uint32_t            sectors_per_clu;                  ///< 每簇扇区数 (2^sect_per_clus_bits)
    uint32_t            sectors_per_clu_bits;             ///< 每簇扇区数指数

    uint32_t            cluster_size;                     ///< 簇大小 (2^cluster_size_bits)
    uint32_t            cluster_size_bits;                ///< 簇大小指数 (sect_size_bits + sect_per_clus_bits)
    uint32_t            num_clusters;                     ///< [单位:簇]簇总数, 含保留簇 0/1

    uint64_t            FAT1_start_sector;                ///< FAT1 首扇区 LBA (相对卷首)
    uint64_t            FAT2_start_sector;                ///< FAT2 首扇区 LBA; 单 FAT 时同 FAT1
    uint32_t            num_FAT_sectors;                  ///< [单位:扇区]单个 FAT 表占用扇区数

    uint64_t            data_start_sector;                ///< [单位:扇区]簇堆(数据区)首扇区 LBA
    uint32_t            root_dir_clu;                     ///< [单位:簇]根目录起始簇号
    uint32_t            dentries_per_clu;                 ///< 每簇目录项数 (项; 每项 32 字节)

    uint32_t            vol_flags;                        ///< 卷标志 (EXFAT_VOLUME_DIRTY 等)
    uint32_t            vol_flags_persistent;             ///< 须持久化的 vol_flags 子集
} pfs_exfat_bs_t;

/// @brief 根目录分配位图元数据 + 独立高热簇缓存
/// @note bmp_cache: mount 时仅分配槽内存; 读盘在 bmp_cache_get (按需)
typedef struct _pfs_exfat_bitmap_t {
    bool                valid;                            ///< true=位图已找到
    uint32_t            start_clu;                        ///< [单位:簇] 位图起始簇号
    uint64_t            size;                             ///< [单位:字节] 位图长度

    pfs_cache_cluster_t bmp_cache;                        ///< 位图高热簇缓存 (独立于 dir_cache)
} pfs_exfat_bitmap_t;

/// @brief FAT 表元数据 + 独立扇区缓存
/// @note fat_cache: mount 时仅分配槽并填几何; 读盘在 fat_cache_get (按需)
typedef struct _pfs_exfat_fat_t {
    bool                valid;                            ///< true=已按 boot 初始化
    uint64_t            start_sector;                     ///< [单位:扇区] FAT1 首扇区 LBA (相对卷首)
    uint32_t            num_sectors;                      ///< [单位:扇区] 单个 FAT 占用扇区数
    uint64_t            size;                             ///< [单位:字节] FAT 字节长度

    pfs_cache_sector_t  fat_cache;                        ///< FAT 扇区高热缓存 (独立于 dir_cache/bitmap)
} pfs_exfat_fat_t;

/// @brief 根目录 upcase 目录项元数据 + 展开表缓冲
/// @note mount 时 create_upcase_table 分配 PFS_EXFAT_UPCASE_BUF_SIZE 并展开加载; 此后只读至 umount
typedef struct _pfs_exfat_upcase_t {
    bool                valid;                            ///< true=根目录已找到 UPCASE 项
    uint32_t            start_clu;                        ///< [单位:簇] upcase 起始簇号
    uint64_t            size;                             ///< [单位:字节] upcase on-disk 压缩长度
    uint32_t            checksum;                         ///< upcase 目录项校验和

    uint16_t            *p_entries;                       ///< 指向 buf 内已展开表 (主机序, 0x10000 项; mount 后只读)
    int                 entries_count;                    ///< p_entries 元素个数

    pfs_buf_t           buf;                              ///< 固定 PFS_EXFAT_UPCASE_BUF_SIZE
} pfs_exfat_upcase_t;

/// @brief 根目录卷标目录项元数据
typedef struct _pfs_exfat_vol_label_t {
    bool                valid;                            ///< true=根目录已找到 VOLUME 项
    int                 len;                              ///< 卷标字符数 (UTF-16 码元个数)
    uint16_t            name[EXFAT_VOLUME_LABEL_LEN];     ///< 卷标 (主机序 UTF-16)
} pfs_exfat_vol_label_t;

/// @brief exFAT superblock info (自 boot 解析)
typedef struct _pfs_exfat_sbi_t {
    pfs_exfat_mount_state_e mount_state;                      ///< UNMOUNTED / MOUNT_RDONLY / MOUNT_RDWR

    pfs_blkio_t             *p_blkio;                         ///< 借用块设备句柄
    int64_t                 part_offset;                      ///< [单位:字节] 分区相对设备偏移; 裸卷/整盘为 0

    pfs_exfat_bs_t          bs;                               ///< boot 解析结果
    pfs_exfat_bitmap_t      bitmap;                           ///< 分配位图元数据 + bmp_cache (按需读)
    pfs_exfat_fat_t         fat;                              ///< FAT 元数据 + fat_cache (按需读)
    pfs_exfat_upcase_t      upcase;                           ///< mount 全量加载后只读
    pfs_exfat_vol_label_t   vol_label;                        ///< 卷标元数据

    pfs_cache_cluster_t     dir_cache;                        ///< 目录元数据簇缓存; init 只分配, get 才读盘
} pfs_exfat_sbi_t;

/// @brief vfs 对接表 (pfs_exfat_ops.c; 已接线)
extern const pfs_fs_ops_t g_pfs_exfat_fs_ops;

/// @brief 重置为 UNMOUNTED
/// @param [in, out] *p_sbi    exFAT sbi
/// @return int PFS_EXFAT_RET_OK 成功; PFS_EXFAT_RET_EINVAL 失败
int pfs_exfat_init(pfs_exfat_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
/// @param [in, out] *p_sbi    exFAT sbi
void pfs_exfat_deinit(pfs_exfat_sbi_t *p_sbi);

/// @brief 读取并解析 boot sector, 并扫描根目录系统项
/// @param [in, out] *p_sbi        exFAT sbi
/// @param [in]      *p_blkio       须已 open
/// @param [in]      part_offset    分区字节偏移
/// @param [in]      mount_mode     PFS_EXFAT_MOUNT_RDONLY 或 PFS_EXFAT_MOUNT_RDWR
/// @return int PFS_EXFAT_RET_OK 成功; 失败保持 UNMOUNTED
/// @note PFS_EXFAT_RET_EINVAL / PFS_EXFAT_RET_EBUSY / PFS_EXFAT_RET_EIO / PFS_EXFAT_RET_ENOEXEC / PFS_EXFAT_RET_ENOMEM / PFS_EXFAT_RET_EBADMSG
int pfs_exfat_mount(pfs_exfat_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, pfs_exfat_mount_state_e mount_mode);

/// @brief 已挂载时切换只读/读写; 不重新解析 boot
/// @param [in, out] *p_sbi        exFAT sbi
/// @param [in]      mount_mode     仅 RDONLY/RDWR
/// @return int PFS_EXFAT_RET_OK 成功; PFS_EXFAT_RET_EBUSY / PFS_EXFAT_RET_EINVAL
int pfs_exfat_remount(pfs_exfat_sbi_t *p_sbi, pfs_exfat_mount_state_e mount_mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
/// @param [in, out] *p_sbi    exFAT sbi
/// @return int PFS_EXFAT_RET_OK 成功; PFS_EXFAT_RET_EINVAL / PFS_EXFAT_RET_EBUSY
int pfs_exfat_umount(pfs_exfat_sbi_t *p_sbi);

/// @brief 仅探测是否为 exFAT; 不修改 p_sbi
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移
/// @return int PFS_EXFAT_RET_OK 成功; PFS_EXFAT_RET_EINVAL / PFS_EXFAT_RET_EIO / PFS_EXFAT_RET_ENOEXEC
int pfs_exfat_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 在 p_blkio 指定分区写入 exFAT 布局
/// @param [in] *p_blkio       须已 open
/// @param [in] part_offset    分区字节偏移 (裸卷为 0)
/// @param [in] vol_bytes      卷容量 [单位:字节], 须为扇区整数倍
/// @return int PFS_EXFAT_RET_OK 成功; PFS_EXFAT_RET_EINVAL / PFS_EXFAT_RET_EIO / PFS_EXFAT_RET_ENOMEM / PFS_EXFAT_RET_EBADMSG
int pfs_exfat_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_EXFAT_H_

// end
