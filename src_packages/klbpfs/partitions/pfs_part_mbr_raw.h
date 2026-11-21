// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_part_mbr_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
// MBR on-disk 布局; 对齐 linux-7.1 include/linux/msdos_partition.h 与 block/partitions/efi.h legacy_mbr
#ifndef _PFS_PART_MBR_RAW_H_
#define _PFS_PART_MBR_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_PART_MBR_SECTOR_SIZE        512
#define PFS_PART_MBR_SIGNATURE          0xAA55
#define PFS_PART_MBR_PRIMARY_COUNT      4

// Protective / EFI MBR os_type (linux EFI_PMBR_OSTYPE_*)
#define PFS_PART_MBR_OSTYPE_EFI         0xEF
#define PFS_PART_MBR_OSTYPE_EFI_GPT     0xEE

// 常见 sys_ind (linux msdos_sys_ind 子集)
#define PFS_PART_MBR_SYS_EMPTY          0x00
#define PFS_PART_MBR_SYS_FAT12          0x01
#define PFS_PART_MBR_SYS_EXTENDED       0x05
#define PFS_PART_MBR_SYS_FAT16          0x06
#define PFS_PART_MBR_SYS_NTFS_EXFAT     0x07
#define PFS_PART_MBR_SYS_FAT32          0x0B
#define PFS_PART_MBR_SYS_FAT32_LBA      0x0C
#define PFS_PART_MBR_SYS_FAT16_LBA      0x0E
#define PFS_PART_MBR_SYS_WIN98_EXT      0x0F
#define PFS_PART_MBR_SYS_LINUX_DATA     0x83
#define PFS_PART_MBR_SYS_LINUX_EXT      0x85

#pragma pack(push, 1)

// 主分区表项 (16 字节); on-disk 小端扇区字段
typedef struct pfs_part_mbr_ent {
    uint8_t  boot_ind;          ///< 0x80=active
    uint8_t  start_head;
    uint8_t  start_sector;      ///< 含 cylinder 高 2 bit
    uint8_t  start_cyl;
    uint8_t  sys_ind;           ///< 分区类型
    uint8_t  end_head;
    uint8_t  end_sector;
    uint8_t  end_cyl;
    uint32_t start_lba;         ///< [单位:扇区] 起始 LBA (le)
    uint32_t size_lba;          ///< [单位:扇区] 长度 (le)
} pfs_part_mbr_ent_t;

// 整扇 MBR (512 字节)
typedef struct pfs_part_mbr {
    uint8_t            boot_code[440];
    uint32_t           disk_signature;  ///< le
    uint16_t           unknown;
    pfs_part_mbr_ent_t partition[PFS_PART_MBR_PRIMARY_COUNT];
    uint16_t           signature;       ///< le; 期望 0xAA55
} pfs_part_mbr_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_PART_MBR_RAW_H_

// end
