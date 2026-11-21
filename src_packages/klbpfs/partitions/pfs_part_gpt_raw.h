// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_part_gpt_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
// GPT on-disk 布局; 对齐 linux-7.1 block/partitions/efi.h
#ifndef _PFS_PART_GPT_RAW_H_
#define _PFS_PART_GPT_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_PART_GPT_HEADER_SIGNATURE   0x5452415020494645ULL  ///< "EFI PART"
#define PFS_PART_GPT_HEADER_REVISION_V1 0x00010000u
#define PFS_PART_GPT_PRIMARY_LBA        1                      ///< [单位:扇区] 主 GPT header LBA

// UEFI Legacy BIOS bootable (GPT entry attributes bit 2)
#define PFS_PART_GPT_ATTR_LEGACY_BIOS_BOOTABLE  (1ULL << 2)

#pragma pack(push, 1)

typedef struct pfs_part_guid {
    uint8_t b[16];
} pfs_part_guid_t;

typedef struct pfs_part_gpt_header {
    uint64_t        signature;                  ///< le; PFS_PART_GPT_HEADER_SIGNATURE
    uint32_t        revision;                   ///< le
    uint32_t        header_size;                ///< [单位:字节] le
    uint32_t        header_crc32;               ///< le
    uint32_t        reserved1;
    uint64_t        my_lba;                     ///< [单位:扇区] le
    uint64_t        alternate_lba;              ///< [单位:扇区] le
    uint64_t        first_usable_lba;           ///< [单位:扇区] le
    uint64_t        last_usable_lba;            ///< [单位:扇区] le
    pfs_part_guid_t disk_guid;
    uint64_t        partition_entry_lba;        ///< [单位:扇区] le
    uint32_t        num_partition_entries;      ///< le
    uint32_t        sizeof_partition_entry;     ///< [单位:字节] le
    uint32_t        partition_entry_array_crc32; ///< le
} pfs_part_gpt_header_t;

typedef struct pfs_part_gpt_entry {
    pfs_part_guid_t type_guid;
    pfs_part_guid_t unique_guid;
    uint64_t        starting_lba;               ///< [单位:扇区] le
    uint64_t        ending_lba;                 ///< [单位:扇区] le (含)
    uint64_t        attributes;                 ///< le
    uint16_t        name_utf16[36];             ///< UTF-16LE
} pfs_part_gpt_entry_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_PART_GPT_RAW_H_

// end
