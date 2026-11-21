// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_fat_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * The MS-DOS / FAT12/16/32 on-disk constants and structures.
 *
 * copy from linux-7.1.x kernel include/uapi/linux/msdos_fs.h
 * (FAT_VALID_MEDIA from include/linux/msdos_fs.h fat_valid_media).
 * Types: __u8/__le16/__le32 -> uint8_t/uint16_t/uint32_t; on-disk little endian.
 */

#ifndef _PFS_FAT_RAW_H_
#define _PFS_FAT_RAW_H_

#include <stdint.h>
#include "pfs_endian.h"

#ifndef FAT_SECTOR_SIZE
#define FAT_SECTOR_SIZE                 512             ///< [单位:字节] 逻辑扇区默认大小
#endif
#define FAT_SECTOR_BITS                 9               ///< log2(FAT_SECTOR_SIZE)
#define FAT_MSDOS_DIR_BITS              5               ///< log2(sizeof(fat_dir_entry_t))=5; linux: MSDOS_DIR_BITS
#define FAT_MSDOS_LONGNAME              256             ///< 长名缓冲上限 (含结尾); linux: MSDOS_LONGNAME

/* directory limit */
#define FAT_MAX_DIR_ENTRIES             (65536)         ///< 单目录最大目录项数
#define FAT_MAX_DIR_SIZE                (FAT_MAX_DIR_ENTRIES << FAT_MSDOS_DIR_BITS) ///< [单位:字节]

#define FAT_ATTR_NONE                   0               ///< 无属性位; linux: ATTR_NONE
#define FAT_ATTR_RO                     1               ///< 只读; linux: ATTR_RO
#define FAT_ATTR_HIDDEN                 2               ///< 隐藏; linux: ATTR_HIDDEN
#define FAT_ATTR_SYS                    4               ///< 系统; linux: ATTR_SYS
#define FAT_ATTR_VOLUME                 8               ///< 卷标; linux: ATTR_VOLUME
#define FAT_ATTR_DIR                    16              ///< 目录; linux: ATTR_DIR
#define FAT_ATTR_ARCH                   32              ///< 归档; linux: ATTR_ARCH

/* attribute bits that are copied "as is" */
#define FAT_ATTR_UNUSED                 (FAT_ATTR_VOLUME | FAT_ATTR_ARCH | FAT_ATTR_SYS | FAT_ATTR_HIDDEN) ///< linux: ATTR_UNUSED
/* bits that are used by the Windows 95/Windows NT extended FAT (LFN) */
#define FAT_ATTR_EXT                    (FAT_ATTR_RO | FAT_ATTR_HIDDEN | FAT_ATTR_SYS | FAT_ATTR_VOLUME) ///< linux: ATTR_EXT

#define FAT_CASE_LOWER_BASE             8               ///< 短名基名小写; linux: CASE_LOWER_BASE
#define FAT_CASE_LOWER_EXT              16              ///< 短名扩展名小写; linux: CASE_LOWER_EXT

#define FAT_DELETED_FLAG                0xe5            ///< name[0] 标记已删除; linux: DELETED_FLAG
#define FAT_IS_FREE(n)                  (!*(n) || *(n) == FAT_DELETED_FLAG) ///< linux: IS_FREE

#define FAT_LFN_LEN                     255             ///< 长文件名最大字符数
#define FAT_MSDOS_NAME                  11              ///< 8.3 短名长度; linux: MSDOS_NAME
#define FAT_MSDOS_SLOTS                 21              ///< 短名+长名最大 slot 数; linux: MSDOS_SLOTS
#define FAT_MAX_UNI_CHARS               ((FAT_MSDOS_SLOTS - 1) * 13 + 1) ///< LFN slot 组装 UTF-16 上限(含 NUL); linux: FAT_MAX_UNI_CHARS
#define FAT_MSDOS_DOT                   ".          "   ///< ".", 补齐到 FAT_MSDOS_NAME; linux: MSDOS_DOT
#define FAT_MSDOS_DOTDOT                "..         "   ///< "..", 补齐到 FAT_MSDOS_NAME; linux: MSDOS_DOTDOT

#define FAT_BOOT_SIGNATURE              0xAA55          ///< 引导扇区尾签名 (offset 510)
#define FAT_EXT_BOOT_SIG                0x29            ///< 扩展 BPB signature (FAT16/32)

/* start of data cluster's entry (number of reserved clusters) */
#define FAT_START_ENT                   2               ///< 首个数据簇号 (0/1 保留)

/* maximum number of clusters */
#define MAX_FAT12                       0xFF4
#define MAX_FAT16                       0xFFF4
#define MAX_FAT32                       0x0FFFFFF6

/* bad cluster mark */
#define BAD_FAT12                       0xFF7
#define BAD_FAT16                       0xFFF7
#define BAD_FAT32                       0x0FFFFFF7

/* standard EOF */
#define EOF_FAT12                       0xFFF
#define EOF_FAT16                       0xFFFF
#define EOF_FAT32                       0x0FFFFFFF

#define FAT_ENT_FREE                    (0)
#define FAT_ENT_BAD                     (BAD_FAT32)
#define FAT_ENT_EOF                     (EOF_FAT32)

#define FAT_FSINFO_SIG1                 0x41615252      ///< FSINFO signature1
#define FAT_FSINFO_SIG2                 0x61417272      ///< FSINFO signature2
/* linux: IS_FSINFO; signature 字段为 on-disk LE */
#define FAT_IS_FSINFO(x)                (pfs_le32_to_cpu((x)->signature1) == FAT_FSINFO_SIG1 \
                                         && pfs_le32_to_cpu((x)->signature2) == FAT_FSINFO_SIG2)

#define FAT_STATE_DIRTY                 0x01            ///< BPB state: 卷脏

/* FAT32 BPB flags (fat32.flags): bit7 mirror off; low 4 = active FAT */
#define FAT32_FAT_MIRROR                0x80            ///< 禁止 FAT 镜像, 仅用 active FAT
#define FAT32_ACTIVE_FAT_MASK           0x0F            ///< active FAT 索引掩码

/* media of boot sector (linux include/linux/msdos_fs.h fat_valid_media) */
#define FAT_VALID_MEDIA(media)          (((media) >= 0xf8) || ((media) == 0xf0))

#pragma pack(1)

/* FAT12/16/32 boot sector (BPB); on-disk little endian */
typedef struct _fat_boot_sector_t {
    uint8_t  ignored[3];                                ///< 引导跳转
    uint8_t  system_id[8];                              ///< OEM 名
    uint8_t  sector_size[2];                            ///< [单位:字节] 逻辑扇区大小 (LE, 未对齐)
    uint8_t  sec_per_clus;                              ///< 每簇扇区数
    uint16_t reserved;                                  ///< [单位:扇区] 保留扇区数
    uint8_t  fats;                                      ///< FAT 表份数
    uint8_t  dir_entries[2];                            ///< 根目录项数 (FAT32 为 0; LE 未对齐)
    uint8_t  sectors[2];                                ///< [单位:扇区] 总扇区 (小卷; LE 未对齐)
    uint8_t  media;                                     ///< 介质描述符
    uint16_t fat_length;                                ///< [单位:扇区] 每 FAT 扇区数 (FAT32 为 0)
    uint16_t secs_track;                                ///< 每磁道扇区数
    uint16_t heads;                                     ///< 磁头数
    uint32_t hidden;                                    ///< [单位:扇区] 隐藏扇区
    uint32_t total_sect;                                ///< [单位:扇区] 总扇区 (sectors==0 时用)

    union {
        struct {
            /* Extended BPB Fields for FAT12/FAT16 */
            uint8_t  drive_number;                      ///< 物理驱动器号
            uint8_t  state;                             ///< 挂载状态 (FAT_STATE_DIRTY)
            uint8_t  signature;                         ///< 扩展引导签名 (0x29)
            uint8_t  vol_id[4];                         ///< 卷序列号
            uint8_t  vol_label[FAT_MSDOS_NAME];         ///< 卷标
            uint8_t  fs_type[8];                        ///< 如 "FAT16   "
        } fat16;

        struct {
            /* only used by FAT32 */
            uint32_t length;                            ///< [单位:扇区] 每 FAT 扇区数
            uint16_t flags;                             ///< bit7 镜像; 低 4 位 active FAT
            uint8_t  version[2];                        ///< 文件系统版本 [主, 次]
            uint32_t root_cluster;                      ///< [单位:簇] 根目录起始簇
            uint16_t info_sector;                       ///< [单位:扇区] FSINFO 扇区号
            uint16_t backup_boot;                       ///< [单位:扇区] 备份引导扇区号
            uint16_t reserved2[6];                      ///< 保留
            /* Extended BPB Fields for FAT32 */
            uint8_t  drive_number;                      ///< 物理驱动器号
            uint8_t  state;                             ///< 挂载状态 (FAT_STATE_DIRTY)
            uint8_t  signature;                         ///< 扩展引导签名 (0x29)
            uint8_t  vol_id[4];                         ///< 卷序列号
            uint8_t  vol_label[FAT_MSDOS_NAME];         ///< 卷标
            uint8_t  fs_type[8];                        ///< 如 "FAT32   "
        } fat32;
    };
} fat_boot_sector_t;

/* FAT32 FSINFO sector; on-disk little endian */
typedef struct _fat_boot_fsinfo_t {
    uint32_t signature1;                                ///< 0x41615252
    uint32_t reserved1[120];                            ///< 保留
    uint32_t signature2;                                ///< 0x61417272
    uint32_t free_clusters;                             ///< 空闲簇数, -1 表示未知
    uint32_t next_cluster;                              ///< 最近分配簇提示
    uint32_t reserved2[4];                              ///< 保留
} fat_boot_fsinfo_t;

/* short directory entry; on-disk little endian; linux: struct msdos_dir_entry */
typedef struct _fat_dir_entry_t {
    uint8_t  name[FAT_MSDOS_NAME];                      ///< 8.3 短名
    uint8_t  attr;                                      ///< 属性位
    uint8_t  lcase;                                     ///< 短名大小写标志
    uint8_t  ctime_cs;                                  ///< 创建时间百分秒 (0-199)
    uint16_t ctime;                                     ///< 创建时间
    uint16_t cdate;                                     ///< 创建日期
    uint16_t adate;                                     ///< 访问日期
    uint16_t starthi;                                   ///< 首簇高 16 bit (FAT32)
    uint16_t time;                                      ///< 修改时间
    uint16_t date;                                      ///< 修改日期
    uint16_t start;                                     ///< 首簇低 16 bit
    uint32_t size;                                      ///< [单位:字节] 文件大小
} fat_dir_entry_t;

/* LFN slot; up to 13 UTF-16 chars; on-disk little endian; linux: struct msdos_dir_slot */
typedef struct _fat_dir_slot_t {
    uint8_t  id;                                        ///< 序号 (bit6=末项)
    uint8_t  name0_4[10];                               ///< 前 5 个 UTF-16 字符
    uint8_t  attr;                                      ///< 须为 FAT_ATTR_EXT
    uint8_t  reserved;                                  ///< 须为 0
    uint8_t  alias_checksum;                            ///< 8.3 别名校验和
    uint8_t  name5_10[12];                              ///< 随后 6 个 UTF-16 字符
    uint16_t start;                                     ///< 须为 0
    uint8_t  name11_12[4];                              ///< 末 2 个 UTF-16 字符
} fat_dir_slot_t;

#pragma pack()

#define FAT_MSDOS_DPS                   (FAT_SECTOR_SIZE / sizeof(fat_dir_entry_t)) ///< 每扇区目录项数; linux: MSDOS_DPS
#define FAT_MSDOS_DPS_BITS              4               ///< log2(FAT_MSDOS_DPS); linux: MSDOS_DPS_BITS
#define FAT_MSDOS_DPB                   (FAT_MSDOS_DPS) ///< 每块目录项数 (扇区=块); linux: MSDOS_DPB
#define FAT_MSDOS_DPB_BITS              4               ///< log2(FAT_MSDOS_DPB); linux: MSDOS_DPB_BITS

#endif /* !_PFS_FAT_RAW_H_ */

// end
