// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ext4_raw.h
/// @brief   on-disk 常量与结构
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    保留下文 SPDX/内核版权声明; 布局字段注释保持不动
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXT4_RAW_H_
#define _PFS_EXT4_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ext2/3/4 on-disk 布局子集 (对照 linux-7.1 fs/ext4/ext4.h / ext4_extents.h)
// 多字节字段为小端

#define PFS_EXT4_SUPER_MAGIC            0xEF53u
#define PFS_EXT4_SUPER_OFFSET           1024u
#define PFS_EXT4_SUPER_SIZE             1024u
#define PFS_EXT4_MIN_BLOCK_SIZE         1024u
#define PFS_EXT4_MAX_BLOCK_SIZE         65536u

#define PFS_EXT4_GOOD_OLD_INODE_SIZE    128u
#define PFS_EXT4_GOOD_OLD_FIRST_INO     11u

#define PFS_EXT4_BAD_INO                1u
#define PFS_EXT4_ROOT_INO               2u
#define PFS_EXT4_JOURNAL_INO            8u

#define PFS_EXT4_NDIR_BLOCKS            12u
#define PFS_EXT4_IND_BLOCK              12u
#define PFS_EXT4_DIND_BLOCK             13u
#define PFS_EXT4_TIND_BLOCK             14u
#define PFS_EXT4_N_BLOCKS               15u

#define PFS_EXT4_NAME_LEN               255u
#define PFS_EXT4_LINK_MAX               65000u

#define PFS_EXT4_EXT_MAGIC              0xF30Au

// s_state
#define PFS_EXT4_VALID_FS               0x0001u
#define PFS_EXT4_ERROR_FS               0x0002u

// i_flags
#define PFS_EXT4_INDEX_FL               0x00001000u
#define PFS_EXT4_EXTENTS_FL             0x00080000u
#define PFS_EXT4_INLINE_DATA_FL         0x10000000u

// feature_compat
#define PFS_EXT4_FEATURE_COMPAT_HAS_JOURNAL     0x0004u
#define PFS_EXT4_FEATURE_COMPAT_EXT_ATTR        0x0008u
#define PFS_EXT4_FEATURE_COMPAT_DIR_INDEX       0x0020u

// feature_ro_compat
#define PFS_EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001u
#define PFS_EXT4_FEATURE_RO_COMPAT_LARGE_FILE   0x0002u
#define PFS_EXT4_FEATURE_RO_COMPAT_HUGE_FILE    0x0008u
#define PFS_EXT4_FEATURE_RO_COMPAT_GDT_CSUM     0x0010u
#define PFS_EXT4_FEATURE_RO_COMPAT_DIR_NLINK    0x0020u
#define PFS_EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE  0x0040u
#define PFS_EXT4_FEATURE_RO_COMPAT_METADATA_CSUM 0x0400u

// feature_incompat
#define PFS_EXT4_FEATURE_INCOMPAT_FILETYPE      0x0002u
#define PFS_EXT4_FEATURE_INCOMPAT_RECOVER       0x0004u
#define PFS_EXT4_FEATURE_INCOMPAT_JOURNAL_DEV   0x0008u
#define PFS_EXT4_FEATURE_INCOMPAT_META_BG       0x0010u
#define PFS_EXT4_FEATURE_INCOMPAT_EXTENTS       0x0040u
#define PFS_EXT4_FEATURE_INCOMPAT_64BIT         0x0080u
#define PFS_EXT4_FEATURE_INCOMPAT_MMP           0x0100u
#define PFS_EXT4_FEATURE_INCOMPAT_FLEX_BG       0x0200u
#define PFS_EXT4_FEATURE_INCOMPAT_EA_INODE      0x0400u
#define PFS_EXT4_FEATURE_INCOMPAT_DIRDATA       0x1000u
#define PFS_EXT4_FEATURE_INCOMPAT_CSUM_SEED     0x2000u
#define PFS_EXT4_FEATURE_INCOMPAT_LARGEDIR      0x4000u
#define PFS_EXT4_FEATURE_INCOMPAT_INLINE_DATA   0x8000u
#define PFS_EXT4_FEATURE_INCOMPAT_ENCRYPT       0x10000u

// 本实现支持的 incompat (其余拒绝挂载)
#define PFS_EXT4_INCOMPAT_SUPP  (PFS_EXT4_FEATURE_INCOMPAT_FILETYPE | \
                                 PFS_EXT4_FEATURE_INCOMPAT_RECOVER | \
                                 PFS_EXT4_FEATURE_INCOMPAT_EXTENTS | \
                                 PFS_EXT4_FEATURE_INCOMPAT_64BIT | \
                                 PFS_EXT4_FEATURE_INCOMPAT_FLEX_BG)

// bg_flags
#define PFS_EXT4_BG_INODE_UNINIT        0x0001u
#define PFS_EXT4_BG_BLOCK_UNINIT        0x0002u
#define PFS_EXT4_BG_INODE_ZEROED        0x0004u

// dir file_type
#define PFS_EXT4_FT_UNKNOWN             0u
#define PFS_EXT4_FT_REG_FILE            1u
#define PFS_EXT4_FT_DIR                 2u
#define PFS_EXT4_FT_CHRDEV              3u
#define PFS_EXT4_FT_BLKDEV              4u
#define PFS_EXT4_FT_FIFO                5u
#define PFS_EXT4_FT_SOCK                6u
#define PFS_EXT4_FT_SYMLINK             7u
#define PFS_EXT4_FT_DIR_CSUM            0xDEu

// mode (POSIX)
#define PFS_EXT4_S_IFMT                 0170000u
#define PFS_EXT4_S_IFREG                0100000u
#define PFS_EXT4_S_IFDIR                0040000u
#define PFS_EXT4_S_IFLNK                0120000u

#define PFS_EXT4_MIN_DESC_SIZE          32u
#define PFS_EXT4_MIN_DESC_SIZE_64BIT    64u

#pragma pack(push, 1)

// 超块按 1024 字节整块读入; 字段用 pfs_ext4_sb_* 助手从偏移解析
typedef uint8_t pfs_ext4_sb_raw_t[PFS_EXT4_SUPER_SIZE];

typedef struct _pfs_ext4_group_desc_t {
    uint32_t bg_block_bitmap_lo;
    uint32_t bg_inode_bitmap_lo;
    uint32_t bg_inode_table_lo;
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lo;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;
    uint32_t bg_block_bitmap_hi;
    uint32_t bg_inode_bitmap_hi;
    uint32_t bg_inode_table_hi;
    uint16_t bg_free_blocks_count_hi;
    uint16_t bg_free_inodes_count_hi;
    uint16_t bg_used_dirs_count_hi;
    uint16_t bg_itable_unused_hi;
    uint32_t bg_exclude_bitmap_hi;
    uint16_t bg_block_bitmap_csum_hi;
    uint16_t bg_inode_bitmap_csum_hi;
    uint32_t bg_reserved;
} pfs_ext4_group_desc_t;

typedef struct _pfs_ext4_inode_raw_t {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size_lo;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks_lo;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[PFS_EXT4_N_BLOCKS];
    uint32_t i_generation;
    uint32_t i_file_acl_lo;
    uint32_t i_size_high;
    uint32_t i_obso_faddr;
    uint16_t i_blocks_high;
    uint16_t i_file_acl_high;
    uint16_t i_uid_high;
    uint16_t i_gid_high;
    uint16_t i_checksum_lo;
    uint16_t i_reserved;
    // 动态 inode 可能更长; 读时按 s_inode_size
} pfs_ext4_inode_raw_t;

typedef struct _pfs_ext4_extent_header_t {
    uint16_t eh_magic;
    uint16_t eh_entries;
    uint16_t eh_max;
    uint16_t eh_depth;
    uint32_t eh_generation;
} pfs_ext4_extent_header_t;

typedef struct _pfs_ext4_extent_t {
    uint32_t ee_block;
    uint16_t ee_len;
    uint16_t ee_start_hi;
    uint32_t ee_start_lo;
} pfs_ext4_extent_t;

typedef struct _pfs_ext4_extent_idx_t {
    uint32_t ei_block;
    uint32_t ei_leaf_lo;
    uint16_t ei_leaf_hi;
    uint16_t ei_unused;
} pfs_ext4_extent_idx_t;

typedef struct _pfs_ext4_dir_entry_2_t {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[PFS_EXT4_NAME_LEN];
} pfs_ext4_dir_entry_2_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_EXT4_RAW_H_

// end
