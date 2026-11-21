// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_vhdx_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_VHDX_RAW_H_
#define _PFS_VHDX_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Microsoft VHDX (Virtual Hard Disk v2) on-disk 布局; 多字节字段为 little-endian
// 对照 [MS-VHDX]; 非经典 .vhd

#define PFS_VHDX_HEADER_BLOCK_SIZE      (64u * 1024u)
#define PFS_VHDX_HEADER_CRC_SIZE        (4u * 1024u)
#define PFS_VHDX_MIB                    (1024u * 1024u)
#define PFS_VHDX_MAX_SECTORS_PER_BLOCK  (1u << 23)

#define PFS_VHDX_FILE_ID_OFFSET         0
#define PFS_VHDX_HEADER1_OFFSET         ((uint64_t)PFS_VHDX_HEADER_BLOCK_SIZE)
#define PFS_VHDX_HEADER2_OFFSET         ((uint64_t)PFS_VHDX_HEADER_BLOCK_SIZE * 2ull)
#define PFS_VHDX_REGION1_OFFSET         ((uint64_t)PFS_VHDX_HEADER_BLOCK_SIZE * 3ull)
#define PFS_VHDX_REGION2_OFFSET         ((uint64_t)PFS_VHDX_HEADER_BLOCK_SIZE * 4ull)

#define PFS_VHDX_FILE_SIG               "vhdxfile"
#define PFS_VHDX_FILE_SIG_LEN           8
#define PFS_VHDX_HEAD_SIG               0x64616568u   ///< "head"
#define PFS_VHDX_REGI_SIG               0x69676572u   ///< "regi"
#define PFS_VHDX_METADATA_SIG           0x617461646174656Dull  ///< "metadata"

#define PFS_VHDX_VERSION_1              1u
#define PFS_VHDX_LOG_VERSION_0          0u

#define PFS_VHDX_LOG_SECTOR_SIZE        4096u
#define PFS_VHDX_LOGE_SIG               0x65676F6Cu   ///< "loge"
#define PFS_VHDX_DESC_SIG               0x63736564u   ///< "desc"
#define PFS_VHDX_ZERO_SIG               0x6F72657Au   ///< "zero"
#define PFS_VHDX_DATA_SIG               0x61746164u   ///< "data"

#define PFS_VHDX_REGION_REQUIRED        0x01u

#define PFS_VHDX_META_IS_USER           0x01u
#define PFS_VHDX_META_IS_VIRTUAL_DISK   0x02u
#define PFS_VHDX_META_IS_REQUIRED       0x04u

#define PFS_VHDX_PARAMS_LEAVE_ALLOCATED 0x01u
#define PFS_VHDX_PARAMS_HAS_PARENT      0x02u

#define PFS_VHDX_BLOCK_SIZE_MIN         PFS_VHDX_MIB
#define PFS_VHDX_BLOCK_SIZE_MAX         (256u * PFS_VHDX_MIB)

// Payload BAT states (低 3 bit)
#define PFS_VHDX_PAYLOAD_NOT_PRESENT    0u
#define PFS_VHDX_PAYLOAD_UNDEFINED      1u
#define PFS_VHDX_PAYLOAD_ZERO           2u
#define PFS_VHDX_PAYLOAD_UNMAPPED       3u
#define PFS_VHDX_PAYLOAD_FULLY_PRESENT  6u
#define PFS_VHDX_PAYLOAD_PARTIAL        7u

#define PFS_VHDX_BAT_STATE_MASK         0x07ull
#define PFS_VHDX_BAT_FILE_OFF_MASK      0xFFFFFFFFFFF00000ull

// 逻辑盘类型 (由 FileParameters 推导; 非 footer DiskType)
#define PFS_VHDX_DISK_FIXED             1u
#define PFS_VHDX_DISK_DYNAMIC           2u
#define PFS_VHDX_DISK_DIFF              3u

#pragma pack(push, 1)

typedef struct _pfs_vhdx_guid_t {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} pfs_vhdx_guid_t;

typedef struct _pfs_vhdx_header_t {
    uint32_t signature;
    uint32_t checksum;
    uint64_t sequence_number;
    pfs_vhdx_guid_t file_write_guid;
    pfs_vhdx_guid_t data_write_guid;
    pfs_vhdx_guid_t log_guid;
    uint16_t log_version;
    uint16_t version;
    uint32_t log_length;
    uint64_t log_offset;
} pfs_vhdx_header_t;

typedef struct _pfs_vhdx_region_hdr_t {
    uint32_t signature;
    uint32_t checksum;
    uint32_t entry_count;
    uint32_t reserved;
} pfs_vhdx_region_hdr_t;

typedef struct _pfs_vhdx_region_ent_t {
    pfs_vhdx_guid_t guid;
    uint64_t file_offset;
    uint32_t length;
    uint32_t data_bits;
} pfs_vhdx_region_ent_t;

typedef struct _pfs_vhdx_meta_hdr_t {
    uint64_t signature;
    uint16_t reserved;
    uint16_t entry_count;
    uint32_t reserved2[5];
} pfs_vhdx_meta_hdr_t;

typedef struct _pfs_vhdx_meta_ent_t {
    pfs_vhdx_guid_t item_id;
    uint32_t offset;
    uint32_t length;
    uint32_t data_bits;
    uint32_t reserved2;
} pfs_vhdx_meta_ent_t;

typedef struct _pfs_vhdx_file_params_t {
    uint32_t block_size;
    uint32_t data_bits;
} pfs_vhdx_file_params_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_VHDX_RAW_H_

// end
