// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_isofs_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
// ISO 9660 on-disk (对照 linux uapi/linux/iso_fs.h; 只读栈)
#ifndef _PFS_ISOFS_RAW_H_
#define _PFS_ISOFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_ISODCL(from, to)            ((to) - (from) + 1)

#define PFS_ISO_VD_PRIMARY              1
#define PFS_ISO_VD_SUPPLEMENTARY        2
#define PFS_ISO_VD_END                  255

#define PFS_ISO_STANDARD_ID             "CD001"
#define PFS_ISO_STANDARD_ID_LEN         5

#define PFS_ISOFS_BLOCK_BITS            11
#define PFS_ISOFS_BLOCK_SIZE            2048

// directory record flags
#define PFS_ISO_FILE_HIDDEN             0x01
#define PFS_ISO_FILE_DIRECTORY          0x02
#define PFS_ISO_FILE_ASSOCIATED         0x04
#define PFS_ISO_FILE_RECORD             0x08
#define PFS_ISO_FILE_PROTECTION         0x10
#define PFS_ISO_FILE_MULTI_EXTENT       0x80

#pragma pack(push, 1)

typedef struct _pfs_iso_volume_descriptor_t {
    uint8_t type[PFS_ISODCL(1, 1)];
    char    id[PFS_ISODCL(2, 6)];
    uint8_t version[PFS_ISODCL(7, 7)];
    uint8_t data[PFS_ISODCL(8, 2048)];
} pfs_iso_volume_descriptor_t;

typedef struct _pfs_iso_primary_descriptor_t {
    uint8_t type[PFS_ISODCL(1, 1)];
    char    id[PFS_ISODCL(2, 6)];
    uint8_t version[PFS_ISODCL(7, 7)];
    uint8_t unused1[PFS_ISODCL(8, 8)];
    char    system_id[PFS_ISODCL(9, 40)];
    char    volume_id[PFS_ISODCL(41, 72)];
    uint8_t unused2[PFS_ISODCL(73, 80)];
    uint8_t volume_space_size[PFS_ISODCL(81, 88)];
    uint8_t unused3[PFS_ISODCL(89, 120)];
    uint8_t volume_set_size[PFS_ISODCL(121, 124)];
    uint8_t volume_sequence_number[PFS_ISODCL(125, 128)];
    uint8_t logical_block_size[PFS_ISODCL(129, 132)];
    uint8_t path_table_size[PFS_ISODCL(133, 140)];
    uint8_t type_l_path_table[PFS_ISODCL(141, 144)];
    uint8_t opt_type_l_path_table[PFS_ISODCL(145, 148)];
    uint8_t type_m_path_table[PFS_ISODCL(149, 152)];
    uint8_t opt_type_m_path_table[PFS_ISODCL(153, 156)];
    uint8_t root_directory_record[PFS_ISODCL(157, 190)];
    char    volume_set_id[PFS_ISODCL(191, 318)];
    char    publisher_id[PFS_ISODCL(319, 446)];
    char    preparer_id[PFS_ISODCL(447, 574)];
    char    application_id[PFS_ISODCL(575, 702)];
    char    copyright_file_id[PFS_ISODCL(703, 739)];
    char    abstract_file_id[PFS_ISODCL(740, 776)];
    char    bibliographic_file_id[PFS_ISODCL(777, 813)];
    uint8_t creation_date[PFS_ISODCL(814, 830)];
    uint8_t modification_date[PFS_ISODCL(831, 847)];
    uint8_t expiration_date[PFS_ISODCL(848, 864)];
    uint8_t effective_date[PFS_ISODCL(865, 881)];
    uint8_t file_structure_version[PFS_ISODCL(882, 882)];
    uint8_t unused4[PFS_ISODCL(883, 883)];
    uint8_t application_data[PFS_ISODCL(884, 1395)];
    uint8_t unused5[PFS_ISODCL(1396, 2048)];
} pfs_iso_primary_descriptor_t;

// Joliet SVD: unused3 位置为 escape (对照 iso_supplementary_descriptor)
typedef struct _pfs_iso_supplementary_descriptor_t {
    uint8_t type[PFS_ISODCL(1, 1)];
    char    id[PFS_ISODCL(2, 6)];
    uint8_t version[PFS_ISODCL(7, 7)];
    uint8_t flags[PFS_ISODCL(8, 8)];
    char    system_id[PFS_ISODCL(9, 40)];
    char    volume_id[PFS_ISODCL(41, 72)];
    uint8_t unused2[PFS_ISODCL(73, 80)];
    uint8_t volume_space_size[PFS_ISODCL(81, 88)];
    uint8_t escape[PFS_ISODCL(89, 120)];
    uint8_t volume_set_size[PFS_ISODCL(121, 124)];
    uint8_t volume_sequence_number[PFS_ISODCL(125, 128)];
    uint8_t logical_block_size[PFS_ISODCL(129, 132)];
    uint8_t path_table_size[PFS_ISODCL(133, 140)];
    uint8_t type_l_path_table[PFS_ISODCL(141, 144)];
    uint8_t opt_type_l_path_table[PFS_ISODCL(145, 148)];
    uint8_t type_m_path_table[PFS_ISODCL(149, 152)];
    uint8_t opt_type_m_path_table[PFS_ISODCL(153, 156)];
    uint8_t root_directory_record[PFS_ISODCL(157, 190)];
    char    volume_set_id[PFS_ISODCL(191, 318)];
    char    publisher_id[PFS_ISODCL(319, 446)];
    char    preparer_id[PFS_ISODCL(447, 574)];
    char    application_id[PFS_ISODCL(575, 702)];
    char    copyright_file_id[PFS_ISODCL(703, 739)];
    char    abstract_file_id[PFS_ISODCL(740, 776)];
    char    bibliographic_file_id[PFS_ISODCL(777, 813)];
    uint8_t creation_date[PFS_ISODCL(814, 830)];
    uint8_t modification_date[PFS_ISODCL(831, 847)];
    uint8_t expiration_date[PFS_ISODCL(848, 864)];
    uint8_t effective_date[PFS_ISODCL(865, 881)];
    uint8_t file_structure_version[PFS_ISODCL(882, 882)];
    uint8_t unused4[PFS_ISODCL(883, 883)];
    uint8_t application_data[PFS_ISODCL(884, 1395)];
    uint8_t unused5[PFS_ISODCL(1396, 2048)];
} pfs_iso_supplementary_descriptor_t;

typedef struct _pfs_iso_directory_record_t {
    uint8_t length[PFS_ISODCL(1, 1)];
    uint8_t ext_attr_length[PFS_ISODCL(2, 2)];
    uint8_t extent[PFS_ISODCL(3, 10)];
    uint8_t size[PFS_ISODCL(11, 18)];
    uint8_t date[PFS_ISODCL(19, 25)];
    uint8_t flags[PFS_ISODCL(26, 26)];
    uint8_t file_unit_size[PFS_ISODCL(27, 27)];
    uint8_t interleave[PFS_ISODCL(28, 28)];
    uint8_t volume_sequence_number[PFS_ISODCL(29, 32)];
    uint8_t name_len[PFS_ISODCL(33, 33)];
    char    name[1];
} pfs_iso_directory_record_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_ISOFS_RAW_H_

// end
