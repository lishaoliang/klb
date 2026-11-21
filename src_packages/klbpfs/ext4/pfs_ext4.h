// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ext4.h
/// @brief   ext2/3/4 子模块对外接口
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EXT4_H_
#define _PFS_EXT4_H_

#include "pfs_blkio.h"
#include "ext4/pfs_ext4_raw.h"
#include "base/pfs_fs_ops.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _pfs_ext4_ret_e {
    PFS_EXT4_RET_OK            =  0,
    PFS_EXT4_RET_EPERM         = -1,
    PFS_EXT4_RET_ENOENT        = -2,
    PFS_EXT4_RET_EIO           = -5,
    PFS_EXT4_RET_ENOEXEC       = -8,
    PFS_EXT4_RET_ENOMEM        = -12,
    PFS_EXT4_RET_EBUSY         = -16,
    PFS_EXT4_RET_EEXIST        = -17,
    PFS_EXT4_RET_ENOTDIR       = -20,
    PFS_EXT4_RET_EISDIR        = -21,
    PFS_EXT4_RET_EINVAL        = -22,
    PFS_EXT4_RET_ENOSPC        = -28,
    PFS_EXT4_RET_EROFS         = -30,
    PFS_EXT4_RET_ERANGE        = -34,
    PFS_EXT4_RET_ENAMETOOLONG  = -36,
    PFS_EXT4_RET_ENOTEMPTY     = -39,
    PFS_EXT4_RET_EBADMSG       = -74,
    PFS_EXT4_RET_EILSEQ        = -84,
    PFS_EXT4_RET_EOPNOTSUPP    = -95,
} pfs_ext4_ret_e;

typedef enum _pfs_ext4_mount_state_e {
    PFS_EXT4_UNMOUNTED         = 0x00,
    PFS_EXT4_MOUNT_RDONLY      = 0x01,
    PFS_EXT4_MOUNT_RDWR        = 0x02,
} pfs_ext4_mount_state_e;

typedef enum _pfs_ext4_type_e {
    PFS_EXT4_TYPE_FILE         = 1,
    PFS_EXT4_TYPE_DIR          = 2,
    PFS_EXT4_TYPE_OTHER        = 3,
} pfs_ext4_type_e;

/// 运行时 sbi (ext2/3/4 共用; 靠 feature 区分)
typedef struct _pfs_ext4_sbi_t {
    pfs_ext4_mount_state_e mount_state;
    pfs_blkio_t           *p_blkio;
    int64_t                part_offset;

    uint32_t               block_size;
    uint32_t               block_bits;
    uint32_t               inode_size;
    uint32_t               desc_size;
    uint32_t               inodes_per_group;
    uint32_t               blocks_per_group;
    uint32_t               groups_count;
    uint32_t               first_data_block;
    uint32_t               first_ino;
    uint64_t               blocks_count;
    uint32_t               inodes_count;
    uint32_t               feature_compat;
    uint32_t               feature_incompat;
    uint32_t               feature_ro_compat;
    uint32_t               free_blocks_count;
    uint32_t               free_inodes_count;

    pfs_ext4_sb_raw_t      sb_raw;
    uint8_t               *p_gdt;               ///< groups_count * desc_size
} pfs_ext4_sbi_t;

typedef struct _pfs_ext4_file_meta_t {
    uint32_t               ino;
    pfs_ext4_type_e        type;
    uint64_t               size;
    uint32_t               mode;
    uint32_t               flags;
    uint32_t               links;
    uint32_t               i_block[PFS_EXT4_N_BLOCKS];
    uint32_t               blocks_512;          ///< i_blocks (512-byte units)
} pfs_ext4_file_meta_t;

typedef struct _pfs_ext4_dir_meta_t {
    uint32_t               ino;
    uint64_t               size;
    uint32_t               flags;
    uint32_t               i_block[PFS_EXT4_N_BLOCKS];
    uint64_t               pos;                 ///< 目录内字节偏移
} pfs_ext4_dir_meta_t;

extern const pfs_fs_ops_t g_pfs_ext2_fs_ops;
extern const pfs_fs_ops_t g_pfs_ext3_fs_ops;
extern const pfs_fs_ops_t g_pfs_ext4_fs_ops;

int pfs_ext4_init(pfs_ext4_sbi_t *p_sbi);
void pfs_ext4_deinit(pfs_ext4_sbi_t *p_sbi);
int pfs_ext4_probe(pfs_blkio_t *p_blkio, int64_t part_offset);
int pfs_ext4_mount(pfs_ext4_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_ext4_mount_state_e mode);
int pfs_ext4_remount(pfs_ext4_sbi_t *p_sbi, pfs_ext4_mount_state_e mode);
int pfs_ext4_umount(pfs_ext4_sbi_t *p_sbi);
int pfs_ext4_sbi_is_mounted(const pfs_ext4_sbi_t *p_sbi);
int pfs_ext4_sbi_can_write(const pfs_ext4_sbi_t *p_sbi);
int pfs_ext2_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);
int pfs_ext3_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);
int pfs_ext4_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

#ifdef __cplusplus
}
#endif

#endif // _PFS_EXT4_H_

// end
