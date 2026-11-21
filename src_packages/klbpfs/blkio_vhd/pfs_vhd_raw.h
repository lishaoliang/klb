// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhd_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VHD_RAW_H_
#define _PFS_VHD_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Microsoft Virtual Hard Disk (VHD) on-disk 布局 (经典 .vhd, 非 VHDX)
// 多字节字段为 big-endian; 读写时用主机序转换

#define PFS_VHD_SECTOR_SIZE             512
#define PFS_VHD_FOOTER_SIZE             512
#define PFS_VHD_DYN_HEADER_SIZE         1024
#define PFS_VHD_BAT_ENTRY_UNUSED        0xFFFFFFFFu
#define PFS_VHD_COOKIE                  "conectix"
#define PFS_VHD_COOKIE_LEN              8
#define PFS_VHD_DYN_COOKIE              "cxsparse"
#define PFS_VHD_DYN_COOKIE_LEN          8

#define PFS_VHD_FEATURE_NONE            0x00000000u
#define PFS_VHD_FEATURE_TEMPORARY       0x00000001u
#define PFS_VHD_FEATURE_RESERVED        0x00000002u

#define PFS_VHD_VERSION_1_0             0x00010000u

// DiskType (footer)
#define PFS_VHD_DISK_TYPE_NONE          0u
#define PFS_VHD_DISK_TYPE_FIXED         2u
#define PFS_VHD_DISK_TYPE_DYNAMIC       3u
#define PFS_VHD_DISK_TYPE_DIFFERENCING  4u

#pragma pack(push, 1)

// 盘几何 (Cylinder/Heads/SectorsPerTrack); on-disk 为大端字段拼装
typedef struct _pfs_vhd_disk_geometry_t {
    uint16_t cylinder;          ///< Cylinder
    uint8_t  heads;             ///< Heads
    uint8_t  sectors_per_track; ///< SectorsPerTrack
} pfs_vhd_disk_geometry_t;

// 文件尾 footer (512 字节); Fixed 亦在文件末尾有一份
typedef struct _pfs_vhd_footer_t {
    char     cookie[8];             ///< "conectix"
    uint32_t features;              ///< Features
    uint32_t file_format_version;   ///< FileFormatVersion (通常 0x00010000)
    uint64_t data_offset;           ///< DataOffset; Fixed 为 0xFFFFFFFFFFFFFFFF
    uint32_t timestamp;             ///< TimeStamp (秒, 自 2000-01-01 UTC)
    char     creator_app[4];        ///< CreatorApplication
    uint32_t creator_version;       ///< CreatorVersion
    char     creator_os[4];         ///< CreatorHostOS ("Wi2k" / "Mac ")
    uint64_t original_size;         ///< OriginalSize [单位:字节]
    uint64_t current_size;          ///< CurrentSize [单位:字节]
    pfs_vhd_disk_geometry_t geometry;
    uint32_t disk_type;             ///< DiskType (FIXED/DYNAMIC/DIFFERENCING)
    uint32_t checksum;              ///< Checksum (ones-complement of struct w/ checksum=0)
    uint8_t  unique_id[16];         ///< UniqueId (UUID)
    uint8_t  saved_state;           ///< SavedState
    uint8_t  reserved[427];         ///< Reserved
} pfs_vhd_footer_t;

// Dynamic / Differencing 头 (1024 字节)
typedef struct _pfs_vhd_parent_locator_t {
    uint32_t platform_code;         ///< PlatformCode
    uint32_t platform_data_space;   ///< PlatformDataSpace [单位:扇区]
    uint32_t platform_data_length;  ///< PlatformDataLength [单位:字节]
    uint32_t reserved;
    uint64_t platform_data_offset;  ///< PlatformDataOffset [单位:字节]
} pfs_vhd_parent_locator_t;

typedef struct _pfs_vhd_dyn_header_t {
    char     cookie[8];             ///< "cxsparse"
    uint64_t data_offset;           ///< DataOffset (未用, 常为 0xFFFFFFFFFFFFFFFF)
    uint64_t table_offset;          ///< TableOffset BAT 字节偏移
    uint32_t header_version;        ///< HeaderVersion (通常 0x00010000)
    uint32_t max_table_entries;     ///< MaxTableEntries BAT 项数
    uint32_t block_size;            ///< BlockSize [单位:字节] (常见 2MiB)
    uint32_t checksum;              ///< Checksum
    uint8_t  parent_unique_id[16];  ///< ParentUniqueId
    uint32_t parent_timestamp;      ///< ParentTimeStamp
    uint32_t reserved;
    uint16_t parent_unicode_name[256]; ///< ParentUnicodeName (UTF-16BE)
    pfs_vhd_parent_locator_t parent_locator[8];
    uint8_t  reserved2[256];
} pfs_vhd_dyn_header_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHD_RAW_H_

// end
