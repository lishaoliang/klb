// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_exfat_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * copy from linux-7.1.x kernel fs/exfat/exfat_raw.h.
 */

#ifndef _EXFAT_RAW_H
#define _EXFAT_RAW_H

#include <stdint.h>

#define EXFAT_BOOT_SIGNATURE            0xAA55          ///< 引导扇区尾签名
#define EXFAT_EXBOOT_SIGNATURE          0xAA550000      ///< 扩展引导区尾签名
#define STR_EXFAT                       "EXFAT   "      ///< 文件系统标识, 须 8 字节

#define EXFAT_MAX_FILE_LEN              255             ///< 文件名最大字符数 (UTF-16)

#define EXFAT_VOLUME_DIRTY              0x0002          ///< vol_flags: 卷脏
#define EXFAT_MEDIA_FAILURE             0x0004          ///< vol_flags: 介质故障

#define EXFAT_EOF_CLUSTER               0xFFFFFFFFu     ///< FAT 链结束标记
#define EXFAT_BAD_CLUSTER               0xFFFFFFF7u     ///< 坏簇标记
#define EXFAT_FREE_CLUSTER              0               ///< 空闲簇
/* Cluster 0, 1 are reserved, the first cluster is 2 in the cluster heap. */
#define EXFAT_RESERVED_CLUSTERS         2               ///< 保留簇数 (簇 0/1)
#define EXFAT_FIRST_CLUSTER             2               ///< 首个数据簇号
#define EXFAT_DATA_CLUSTER_COUNT(sbi)   \
        ((sbi)->num_clusters - EXFAT_RESERVED_CLUSTERS) ///< 数据区可用簇数
#define EXFAT_MAX_NUM_CLUSTER           (0xFFFFFFF5)    ///< 最大簇号上界

/* AllocationPossible and NoFatChain field in GeneralSecondaryFlags Field */
#define EXFAT_ALLOC_POSSIBLE            0x01            ///< stream.flags: 可分配
#define EXFAT_ALLOC_FAT_CHAIN           0x01            ///< 经 FAT 链分配
#define EXFAT_ALLOC_NO_FAT_CHAIN        0x03            ///< 连续簇, 不建 FAT 链

#define EXFAT_DENTRY_SIZE               32              ///< 目录项大小 (字节)
#define EXFAT_DENTRY_SIZE_BITS          5               ///< 目录项大小 2^5=32
/* exFAT allows 8388608(256MB) directory entries */
#define EXFAT_MAX_DENTRIES              8388608         ///< 单目录最大目录项数

/* dentry types */
#define EXFAT_UNUSED                    0x00            ///< 目录结束
#define EXFAT_DELETE                    (~0x80)         ///< 已删除项掩码
#define IS_EXFAT_DELETED(x)             ((x) < 0x80)    ///< 已删除 (0x01~0x7F)
#define EXFAT_INVAL                     0x80            ///< 无效/空闲目录项
#define EXFAT_BITMAP                    0x81            ///< 分配位图
#define EXFAT_UPCASE                    0x82            ///< 大写转换表
#define EXFAT_VOLUME                    0x83            ///< 卷标
#define EXFAT_FILE                      0x85            ///< 文件或目录 (主项)
#define EXFAT_GUID                      0xA0            ///< GUID 目录项
#define EXFAT_PADDING                   0xA1            ///< 填充目录项
#define EXFAT_ACLTAB                    0xA2            ///< ACL 表目录项
#define EXFAT_STREAM                    0xC0            ///< 流扩展目录项
#define EXFAT_NAME                      0xC1            ///< 文件名目录项
#define EXFAT_ACL                       0xC2            ///< ACL 目录项
#define EXFAT_VENDOR_EXT                0xE0            ///< 厂商扩展目录项
#define EXFAT_VENDOR_ALLOC              0xE1            ///< 厂商分配目录项

#define IS_EXFAT_CRITICAL_PRI(x)        (x < 0xA0)      ///< 主目录项 (critical primary)
#define IS_EXFAT_BENIGN_PRI(x)          (x < 0xC0)      ///< 可忽略主项 (benign primary)
#define IS_EXFAT_CRITICAL_SEC(x)        (x < 0xE0)      ///< 次级目录项 (critical secondary)

/* checksum types */
#define EXFAT_CS_DIR_ENTRY              0               ///< 目录项组校验
#define EXFAT_CS_BOOT_SECTOR            1               ///< 引导扇区校验
#define EXFAT_CS_DEFAULT                2               ///< 默认校验算法

/* file attributes */
#define EXFAT_ATTR_READONLY             0x0001          ///< 只读
#define EXFAT_ATTR_HIDDEN               0x0002          ///< 隐藏
#define EXFAT_ATTR_SYSTEM               0x0004          ///< 系统
#define EXFAT_ATTR_VOLUME               0x0008          ///< 卷标
#define EXFAT_ATTR_SUBDIR               0x0010          ///< 目录
#define EXFAT_ATTR_ARCHIVE              0x0020          ///< 归档

#define EXFAT_ATTR_RWMASK               (EXFAT_ATTR_HIDDEN | EXFAT_ATTR_SYSTEM | \
                                         EXFAT_ATTR_VOLUME | EXFAT_ATTR_SUBDIR | \
                                         EXFAT_ATTR_ARCHIVE) ///< 可写属性掩码

#define EXFAT_BOOTSEC_JUMP_BOOT_LEN     3               ///< jmp_boot 长度
#define EXFAT_BOOTSEC_FS_NAME_LEN       8               ///< fs_name 长度
#define EXFAT_BOOTSEC_OLDBPB_LEN        53              ///< must_be_zero 长度

#define EXFAT_FILE_NAME_LEN             15              ///< 文件名目录项 UTF-16 字符数
#define EXFAT_VOLUME_LABEL_LEN          11              ///< 卷标 UTF-16 字符数

#define EXFAT_MIN_SECT_SIZE_BITS        9               ///< 最小扇区 512B (2^9)
#define EXFAT_MAX_SECT_SIZE_BITS        12              ///< 最大扇区 4096B (2^12)
#define EXFAT_MAX_SECT_PER_CLUS_BITS(x) (25 - (x)->sect_size_bits) ///< 每簇扇区数指数上界

#pragma pack(1)

/* EXFAT: Main and Backup Boot Sector (512 bytes) little endian */
typedef struct _exfat_boot_sector_t {
    uint8_t  jmp_boot[EXFAT_BOOTSEC_JUMP_BOOT_LEN];     ///< 引导跳转指令
    uint8_t  fs_name[EXFAT_BOOTSEC_FS_NAME_LEN];        ///< 文件系统标识 "EXFAT   "
    uint8_t  must_be_zero[EXFAT_BOOTSEC_OLDBPB_LEN];    ///< 保留, 须全 0
    uint64_t partition_offset;                          ///< 分区起始扇区偏移
    uint64_t vol_length;                                ///< 卷总扇区数
    uint32_t fat_offset;                                ///< FAT 表首扇区偏移
    uint32_t fat_length;                                ///< 单个 FAT 表扇区数
    uint32_t clu_offset;                                ///< 簇堆首扇区偏移
    uint32_t clu_count;                                 ///< 簇堆可用簇数
    uint32_t root_cluster;                              ///< 根目录起始簇号
    uint32_t vol_serial;                                ///< 卷序列号
    uint8_t  fs_revision[2];                            ///< 文件系统版本 [次, 主]
    uint16_t vol_flags;                                 ///< 卷标志 (VOLUME_DIRTY 等)
    uint8_t  sect_size_bits;                            ///< 扇区大小指数 (9~12)
    uint8_t  sect_per_clus_bits;                        ///< 每簇扇区数指数
    uint8_t  num_fats;                                  ///< FAT 表份数 (1 或 2)
    uint8_t  drv_sel;                                   ///< INT 13h 驱动器号
    uint8_t  percent_in_use;                            ///< 已分配簇占比 (0~100, 0xFF=未知)
    uint8_t  reserved[7];                               ///< 保留
    uint8_t  boot_code[390];                            ///< 引导代码
    uint16_t signature;                                 ///< 尾签名 0xAA55
} exfat_boot_sector_t;

/* little endian */
typedef struct _exfat_dentry_t {
    uint8_t type;                                       ///< 目录项类型 (EXFAT_FILE 等)
    union {
        struct {                                        ///< 主文件目录项 (type=EXFAT_FILE)
            uint8_t  num_ext;                           ///< 后续次级项数, 总项数=num_ext+1
            uint16_t checksum;                          ///< 本组全部目录项校验和
            uint16_t attr;                              ///< 文件属性
            uint16_t reserved1;                         ///< 保留
            uint16_t create_time;                       ///< 创建时间 (FAT 格式)
            uint16_t create_date;                       ///< 创建日期
            uint16_t modify_time;                       ///< 修改时间
            uint16_t modify_date;                       ///< 修改日期
            uint16_t access_time;                       ///< 访问时间
            uint16_t access_date;                       ///< 访问日期
            uint8_t  create_time_cs;                    ///< 创建时间百分秒 (10ms)
            uint8_t  modify_time_cs;                    ///< 修改时间百分秒
            uint8_t  create_tz;                         ///< 创建时区 (bit7=EXFAT_TZ_VALID)
            uint8_t  modify_tz;                         ///< 修改时区
            uint8_t  access_tz;                         ///< 访问时区
            uint8_t  reserved2[7];                      ///< 保留
        } file; /* file directory entry */
        struct {                                        ///< 流扩展目录项 (type=EXFAT_STREAM)
            uint8_t  flags;                             ///< 分配标志 (ALLOC_*)
            uint8_t  reserved1;                         ///< 保留
            uint8_t  name_len;                          ///< 文件名 UTF-16 总长度
            uint16_t name_hash;                         ///< 文件名哈希
            uint16_t reserved2;                         ///< 保留
            uint64_t valid_size;                        ///< 有效数据长度 (字节)
            uint32_t reserved3;                         ///< 保留
            uint32_t start_clu;                         ///< 数据起始簇号
            uint64_t size;                              ///< 流总长度 (字节)
        } stream; /* stream extension directory entry */
        struct {                                        ///< 文件名目录项 (type=EXFAT_NAME)
            uint8_t  flags;                             ///< 文件名片段序号 (低 5 bit)
            uint16_t unicode_0_14[EXFAT_FILE_NAME_LEN]; ///< 本段 UTF-16LE 文件名
        } name; /* file name directory entry */
        struct {                                        ///< 分配位图目录项 (type=EXFAT_BITMAP)
            uint8_t  flags;                             ///< 标志
            uint8_t  reserved[18];                      ///< 保留
            uint32_t start_clu;                         ///< 位图起始簇号
            uint64_t size;                              ///< 位图长度 (字节)
        } bitmap; /* allocation bitmap directory entry */
        struct {                                        ///< 大写表目录项 (type=EXFAT_UPCASE)
            uint8_t  reserved1[3];                      ///< 保留
            uint32_t checksum;                          ///< 大写表校验和
            uint8_t  reserved2[12];                     ///< 保留
            uint32_t start_clu;                         ///< 大写表起始簇号
            uint64_t size;                              ///< 大写表长度 (字节)
        } upcase; /* up-case table directory entry */
        struct {                                        ///< 卷标目录项 (type=EXFAT_VOLUME)
            uint8_t  char_count;                        ///< 卷标字符数
            uint16_t volume_label[EXFAT_VOLUME_LABEL_LEN]; ///< 卷标 UTF-16LE
            uint8_t  reserved[8];                       ///< 保留
        } volume_label; /* volume label directory entry */
        struct {                                        ///< 厂商扩展目录项 (type=EXFAT_VENDOR_EXT)
            uint8_t  flags;                             ///< 标志
            uint8_t  vendor_guid[16];                   ///< 厂商 GUID
            uint8_t  vendor_defined[14];                ///< 厂商自定义
        } vendor_ext; /* vendor extension directory entry */
        struct {                                        ///< 厂商分配目录项 (type=EXFAT_VENDOR_ALLOC)
            uint8_t  flags;                             ///< 标志
            uint8_t  vendor_guid[16];                   ///< 厂商 GUID
            uint8_t  vendor_defined[2];                 ///< 厂商自定义
            uint32_t start_clu;                         ///< 数据起始簇号
            uint64_t size;                              ///< 数据长度 (字节)
        } vendor_alloc; /* vendor allocation directory entry */
        struct {                                        ///< 通用次级目录项
            uint8_t  flags;                             ///< 标志
            uint8_t  custom_defined[18];                ///< 自定义字段
            uint32_t start_clu;                         ///< 数据起始簇号
            uint64_t size;                              ///< 数据长度 (字节)
        } generic_secondary; /* generic secondary directory entry */
    } dentry;
} exfat_dentry_t;

#pragma pack()

#define EXFAT_TZ_VALID                  (1 << 7)        ///< 时区字段有效位

/* Jan 1 GMT 00:00:00 1980 */
#define EXFAT_MIN_TIMESTAMP_SECS        315532800LL     ///< 最小 Unix 时间戳
/* Dec 31 GMT 23:59:59 2107 */
#define EXFAT_MAX_TIMESTAMP_SECS        4354819199LL    ///< 最大 Unix 时间戳

#endif /* !_EXFAT_RAW_H */

// end
