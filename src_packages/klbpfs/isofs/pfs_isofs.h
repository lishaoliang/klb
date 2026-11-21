// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_isofs.h
/// @brief   ISOFS 子模块对外接口
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ISOFS_H_
#define _PFS_ISOFS_H_

#include "pfs_blkio.h"
#include "isofs/pfs_isofs_raw.h"
#include "base/pfs_fs_ops.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _pfs_isofs_ret_e {
    PFS_ISOFS_RET_OK            =  0,
    PFS_ISOFS_RET_EPERM         = -1,
    PFS_ISOFS_RET_ENOENT        = -2,
    PFS_ISOFS_RET_EIO           = -5,
    PFS_ISOFS_RET_ENOEXEC       = -8,
    PFS_ISOFS_RET_ENOMEM        = -12,
    PFS_ISOFS_RET_EBUSY         = -16,
    PFS_ISOFS_RET_EEXIST        = -17,
    PFS_ISOFS_RET_ENOTDIR       = -20,
    PFS_ISOFS_RET_EISDIR        = -21,
    PFS_ISOFS_RET_EINVAL        = -22,
    PFS_ISOFS_RET_ENOSPC        = -28,
    PFS_ISOFS_RET_EROFS         = -30,
    PFS_ISOFS_RET_ERANGE        = -34,
    PFS_ISOFS_RET_ENAMETOOLONG  = -36,
    PFS_ISOFS_RET_ENOTEMPTY     = -39,
    PFS_ISOFS_RET_EBADMSG       = -74,
    PFS_ISOFS_RET_EILSEQ        = -84,
    PFS_ISOFS_RET_EOPNOTSUPP    = -95,
} pfs_isofs_ret_e;

typedef enum _pfs_isofs_mount_state_e {
    PFS_ISOFS_UNMOUNTED         = 0x00,
    PFS_ISOFS_MOUNT_RDONLY      = 0x01,
    PFS_ISOFS_MOUNT_RDWR        = 0x02,                 ///< 首期拒绝; 保留枚举对齐其它栈
} pfs_isofs_mount_state_e;

typedef enum _pfs_isofs_type_e {
    PFS_ISOFS_TYPE_FILE         = 1,
    PFS_ISOFS_TYPE_DIR          = 2,
} pfs_isofs_type_e;

#define PFS_ISOFS_MAX_SECTIONS          32

/// 单节数据区 (extent 已含 +ext_attr_length)
typedef struct _pfs_isofs_section_t {
    uint32_t                extent;                     ///< [单位:逻辑块] 数据起点
    uint32_t                size;                       ///< [单位:字节] 本节长度
} pfs_isofs_section_t;

/// 目录/文件定位
typedef struct _pfs_isofs_inode_t {
    pfs_isofs_type_e        type;
    uint8_t                 flags;                      ///< directory record flags (首节)
    uint8_t                 n_sections;                 ///< >=1
    uint32_t                size;                       ///< [单位:字节] 各节之和
    uint32_t                extent;                     ///< 首节数据 LBA (便于目录 walk)
    pfs_isofs_section_t     sec[PFS_ISOFS_MAX_SECTIONS];
} pfs_isofs_inode_t;

typedef struct _pfs_isofs_sbi_t {
    pfs_isofs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;                ///< [单位:字节]

    uint32_t                logical_block_size;         ///< [单位:字节] 通常 2048
    uint32_t                volume_space_size;          ///< [单位:逻辑块]
    pfs_isofs_inode_t       root;                       ///< 根目录

    int                     joliet_level;               ///< 0=无; 1..3
    int                     rock;                       ///< 1=Rock Ridge 生效 (优先于 Joliet)
    int                     rock_offset;                ///< SUSP skip (来自 SP); -1 未设
} pfs_isofs_sbi_t;

/// ops 私有: 打开文件
typedef struct _pfs_isofs_file_meta_t {
    pfs_isofs_inode_t       inode;
} pfs_isofs_file_meta_t;

/// ops 私有: 打开目录流
typedef struct _pfs_isofs_dir_meta_t {
    pfs_isofs_inode_t       dir;
    uint32_t                pos;                        ///< [单位:字节] 目录内偏移
} pfs_isofs_dir_meta_t;

extern const pfs_fs_ops_t g_pfs_isofs_fs_ops;

int pfs_isofs_init(pfs_isofs_sbi_t *p_sbi);
void pfs_isofs_deinit(pfs_isofs_sbi_t *p_sbi);
int pfs_isofs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);
int pfs_isofs_mount(pfs_isofs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset,
        pfs_isofs_mount_state_e mode);
int pfs_isofs_remount(pfs_isofs_sbi_t *p_sbi, pfs_isofs_mount_state_e mode);
int pfs_isofs_umount(pfs_isofs_sbi_t *p_sbi);
int pfs_isofs_sbi_is_mounted(const pfs_isofs_sbi_t *p_sbi);
int pfs_isofs_sbi_can_write(const pfs_isofs_sbi_t *p_sbi);
int pfs_isofs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

#define PFS_ISOFS_MKVOL_JOLIET_DEFAULT  3
#define PFS_ISOFS_MKVOL_ROCK_DEFAULT    1
/// 从宿主目录打包 ISO9660 Level 2 映像 (供 pfs_mkvol); volume_id 可 NULL
/// joliet_level: 0=关, 1..3; rock_ridge: 0/1
int pfs_isofs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, const char *src_dir,
        const char *volume_id, int joliet_level, int rock_ridge);

#ifdef __cplusplus
}
#endif

#endif // _PFS_ISOFS_H_

// end
