// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_cramfs.h
/// @brief   cramfs 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    cramfs 子模块公开头 (pfsmtd; zlib 块只读子集)
///  \n 目录: pfsmtd/src/cramfs/; 对照 ../linux-7.1.2/fs/cramfs/
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CRAMFS_H_
#define _PFS_CRAMFS_H_

#include "pfs.h"
#include "cramfs/pfs_cramfs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_cramfs_sbi_t.mount_state)
typedef enum _pfs_cramfs_mount_state_e {
    PFS_CRAMFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_CRAMFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_CRAMFS_MOUNT_RDWR     = 0x02,                   ///< 只读 FS; RDWR 拒绝
} pfs_cramfs_mount_state_e;

/// @brief cramfs superblock info
typedef struct _pfs_cramfs_sbi_t {
    pfs_cramfs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    int64_t                 sb_rel; ///< super 相对 part (0 或 512)
    uint32_t                size; ///< 镜像有效长度
    uint32_t                flags;
    pfs_cramfs_inode_t      root;
} pfs_cramfs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_cramfs_file_meta_t {
    pfs_cramfs_inode_t          ino;
    uint32_t                    data_off; ///< 块指针表起始绝对相对 part
} pfs_cramfs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_cramfs_dir_meta_t {
    pfs_cramfs_inode_t          ino;
    uint32_t                    dir_off;
    uint32_t                    dir_end;
    uint32_t                    cur; ///< 当前条目相对 part 的偏移
} pfs_cramfs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_cramfs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_cramfs_init(pfs_cramfs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_cramfs_deinit(pfs_cramfs_sbi_t *p_sbi);

/// @brief 仅探测是否为 cramfs; 不修改 sbi
int pfs_cramfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 读取并解析 super; 挂载只读卷
int pfs_cramfs_mount(pfs_cramfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写; RDWR 拒绝
int pfs_cramfs_remount(pfs_cramfs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_cramfs_umount(pfs_cramfs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY)
int pfs_cramfs_sbi_is_mounted(const pfs_cramfs_sbi_t *p_sbi);

/// @brief 是否允许写; cramfs 恒为 0
int pfs_cramfs_sbi_can_write(const pfs_cramfs_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 cramfs 布局
int pfs_cramfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 cramfs 镜像
int pfs_cramfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_CRAMFS_H_

// end
