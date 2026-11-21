// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_squashfs_raw.h
/// @brief   squashfs on-disk 布局
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    对照 linux-7.1.2/fs/squashfs/squashfs_fs.h; 磁盘序 LE
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_SQUASHFS_RAW_H_
#define _PFS_SQUASHFS_RAW_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PFS_SQUASHFS_MAGIC              0x73717368u     ///< "hsqs" LE
#define PFS_SQUASHFS_MAJOR              4
#define PFS_SQUASHFS_MINOR              0

#define PFS_SQUASHFS_COMP_ZLIB          1
#define PFS_SQUASHFS_COMP_LZMA          2
#define PFS_SQUASHFS_COMP_LZO           3
#define PFS_SQUASHFS_COMP_XZ            4
#define PFS_SQUASHFS_COMP_LZ4           5
#define PFS_SQUASHFS_COMP_ZSTD          6

#define PFS_SQUASHFS_METADATA_SIZE      8192
#define PFS_SQUASHFS_NAME_LEN           256
#define PFS_SQUASHFS_DIR_COUNT          256
#define PFS_SQUASHFS_INVALID_FRAG       0xffffffffu
#define PFS_SQUASHFS_INVALID_BLK        (-1LL)

/// @brief Volume flags bits (对照 squashfs_fs.h)
#define PFS_SQUASHFS_NOI                0
#define PFS_SQUASHFS_NOD                1
#define PFS_SQUASHFS_NOF                3
#define PFS_SQUASHFS_NO_FRAG            4

#define PFS_SQUASHFS_DIR_TYPE           1
#define PFS_SQUASHFS_REG_TYPE           2
#define PFS_SQUASHFS_SYMLINK_TYPE       3
#define PFS_SQUASHFS_BLKDEV_TYPE        4
#define PFS_SQUASHFS_CHRDEV_TYPE        5
#define PFS_SQUASHFS_FIFO_TYPE          6
#define PFS_SQUASHFS_SOCKET_TYPE        7
#define PFS_SQUASHFS_LDIR_TYPE          8
#define PFS_SQUASHFS_LREG_TYPE          9
#define PFS_SQUASHFS_LSYMLINK_TYPE      10

#define PFS_SQUASHFS_COMPRESSED_BIT     (1u << 15)
#define PFS_SQUASHFS_COMPRESSED_BIT_BLOCK (1u << 24)

#define PFS_SQUASHFS_COMPRESSED_SIZE(B) \
    (((B) & ~PFS_SQUASHFS_COMPRESSED_BIT) ? ((B) & ~PFS_SQUASHFS_COMPRESSED_BIT) \
                                          : PFS_SQUASHFS_COMPRESSED_BIT)
#define PFS_SQUASHFS_COMPRESSED(B)      (!((B) & PFS_SQUASHFS_COMPRESSED_BIT))

#define PFS_SQUASHFS_COMPRESSED_SIZE_BLOCK(B) ((B) & ~PFS_SQUASHFS_COMPRESSED_BIT_BLOCK)
#define PFS_SQUASHFS_COMPRESSED_BLOCK(B) (!((B) & PFS_SQUASHFS_COMPRESSED_BIT_BLOCK))

#define PFS_SQUASHFS_INODE_BLK(A)       ((unsigned int)((uint64_t)(A) >> 16))
#define PFS_SQUASHFS_INODE_OFFSET(A)    ((unsigned int)((uint64_t)(A) & 0xffffu))
#define PFS_SQUASHFS_MKINODE(A, B) \
    ((int64_t)(((uint64_t)(A) << 16) | (uint64_t)(unsigned int)(B)))

#define PFS_SQUASHFS_FRAGMENT_BYTES(A) \
    ((uint64_t)(A) * sizeof(pfs_squashfs_fragment_entry_t))
#define PFS_SQUASHFS_FRAGMENT_INDEX(A) \
    ((unsigned int)(PFS_SQUASHFS_FRAGMENT_BYTES(A) / PFS_SQUASHFS_METADATA_SIZE))
#define PFS_SQUASHFS_FRAGMENT_INDEX_OFFSET(A) \
    ((unsigned int)(PFS_SQUASHFS_FRAGMENT_BYTES(A) % PFS_SQUASHFS_METADATA_SIZE))
#define PFS_SQUASHFS_FRAGMENT_INDEXES(A) \
    ((unsigned int)((PFS_SQUASHFS_FRAGMENT_BYTES(A) + PFS_SQUASHFS_METADATA_SIZE - 1) \
                    / PFS_SQUASHFS_METADATA_SIZE))
#define PFS_SQUASHFS_FRAGMENT_INDEX_BYTES(A) \
    ((unsigned int)(PFS_SQUASHFS_FRAGMENT_INDEXES(A) * sizeof(uint64_t)))

#pragma pack(push, 1)

typedef struct _pfs_squashfs_super_block_t {
    uint32_t                s_magic;
    uint32_t                inodes;
    uint32_t                mkfs_time;
    uint32_t                block_size;
    uint32_t                fragments;
    uint16_t                compression;
    uint16_t                block_log;
    uint16_t                flags;
    uint16_t                no_ids;
    uint16_t                s_major;
    uint16_t                s_minor;
    uint64_t                root_inode;
    uint64_t                bytes_used;
    uint64_t                id_table_start;
    uint64_t                xattr_id_table_start;
    uint64_t                inode_table_start;
    uint64_t                directory_table_start;
    uint64_t                fragment_table_start;
    uint64_t                lookup_table_start;
} pfs_squashfs_super_block_t;

typedef struct _pfs_squashfs_base_inode_t {
    uint16_t                inode_type;
    uint16_t                mode;
    uint16_t                uid;
    uint16_t                guid;
    uint32_t                mtime;
    uint32_t                inode_number;
} pfs_squashfs_base_inode_t;

typedef struct _pfs_squashfs_reg_inode_t {
    uint16_t                inode_type;
    uint16_t                mode;
    uint16_t                uid;
    uint16_t                guid;
    uint32_t                mtime;
    uint32_t                inode_number;
    uint32_t                start_block;
    uint32_t                fragment;
    uint32_t                offset;
    uint32_t                file_size;
    ///< block_list[] (可变长, 不在此结构体内)
} pfs_squashfs_reg_inode_t;

typedef struct _pfs_squashfs_lreg_inode_t {
    uint16_t                inode_type;
    uint16_t                mode;
    uint16_t                uid;
    uint16_t                guid;
    uint32_t                mtime;
    uint32_t                inode_number;
    uint64_t                start_block;
    uint64_t                file_size;
    uint64_t                sparse;
    uint32_t                nlink;
    uint32_t                fragment;
    uint32_t                offset;
    uint32_t                xattr;
    ///< block_list[] (可变长, 不在此结构体内)
} pfs_squashfs_lreg_inode_t;

typedef struct _pfs_squashfs_dir_inode_t {
    uint16_t                inode_type;
    uint16_t                mode;
    uint16_t                uid;
    uint16_t                guid;
    uint32_t                mtime;
    uint32_t                inode_number;
    uint32_t                start_block;
    uint32_t                nlink;
    uint16_t                file_size;
    uint16_t                offset;
    uint32_t                parent_inode;
} pfs_squashfs_dir_inode_t;

typedef struct _pfs_squashfs_ldir_inode_t {
    uint16_t                inode_type;
    uint16_t                mode;
    uint16_t                uid;
    uint16_t                guid;
    uint32_t                mtime;
    uint32_t                inode_number;
    uint32_t                nlink;
    uint32_t                file_size;
    uint32_t                start_block;
    uint32_t                parent_inode;
    uint16_t                i_count;
    uint16_t                offset;
    uint32_t                xattr;
    ///< dir_index[] (可变长, 不在此结构体内)
} pfs_squashfs_ldir_inode_t;

typedef struct _pfs_squashfs_dir_index_t {
    uint32_t                index;
    uint32_t                start_block;
    uint32_t                size;
    ///< name[size+1] (可变长, 不在此结构体内)
} pfs_squashfs_dir_index_t;

typedef struct _pfs_squashfs_dir_header_t {
    uint32_t                count;
    uint32_t                start_block;
    uint32_t                inode_number;
} pfs_squashfs_dir_header_t;

typedef struct _pfs_squashfs_dir_entry_t {
    uint16_t                offset;
    uint16_t                inode_number;
    uint16_t                type;
    uint16_t                size;
    ///< name[size+1] (可变长, 不在此结构体内)
} pfs_squashfs_dir_entry_t;

typedef struct _pfs_squashfs_fragment_entry_t {
    uint64_t                start_block;
    uint32_t                size;
    uint32_t                unused;
} pfs_squashfs_fragment_entry_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // _PFS_SQUASHFS_RAW_H_

// end
