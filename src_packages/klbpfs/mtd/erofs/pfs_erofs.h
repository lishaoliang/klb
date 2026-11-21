// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_erofs.h
/// @brief   erofs 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    erofs 子模块公开头 (pfsmtd; 未压缩 FLAT_PLAIN/INLINE 只读子集)
///  \n 目录: pfsmtd/src/erofs/; 对照 ../linux-7.1.2/fs/erofs/
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EROFS_H_
#define _PFS_EROFS_H_

#include "pfs.h"
#include "erofs/pfs_erofs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_erofs_sbi_t.mount_state)
typedef enum _pfs_erofs_mount_state_e {
    PFS_EROFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_EROFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_EROFS_MOUNT_RDWR     = 0x02,                   ///< 只读 FS; RDWR 拒绝
} pfs_erofs_mount_state_e;

/// @brief 解析后的 inode 视图
typedef struct _pfs_erofs_inode_t {
    uint64_t                nid;
    uint16_t                mode;
    uint16_t                datalayout;
    uint32_t                inode_isize;
    uint32_t                xattr_isize;
    uint64_t                i_size;
    uint64_t                startblk; ///< FLAT_*; NULL_ADDR=空洞
    uint64_t                iloc; ///< inode 绝对字节偏移 (相对 part)
    int                     dot_omitted;
} pfs_erofs_inode_t;

/// @brief erofs superblock info
typedef struct _pfs_erofs_sbi_t {
    pfs_erofs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    int                     blkszbits;
    uint32_t                blksz;
    uint32_t                meta_blkaddr;
    uint64_t                root_nid;
    uint32_t                feature_incompat;
} pfs_erofs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_erofs_file_meta_t {
    pfs_erofs_inode_t           vi;
} pfs_erofs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_erofs_dir_meta_t {
    pfs_erofs_inode_t           vi;
    int64_t                     pos; ///< 目录流逻辑偏移
    int                         invent_dot; ///< 合成 '.' 尚未发出
} pfs_erofs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_erofs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_erofs_init(pfs_erofs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_erofs_deinit(pfs_erofs_sbi_t *p_sbi);

/// @brief 仅探测是否为 erofs; 不修改 sbi
int pfs_erofs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 读取并解析 super; 挂载只读卷
int pfs_erofs_mount(pfs_erofs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写; RDWR 拒绝
int pfs_erofs_remount(pfs_erofs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_erofs_umount(pfs_erofs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY)
int pfs_erofs_sbi_is_mounted(const pfs_erofs_sbi_t *p_sbi);

/// @brief 是否允许写; erofs 恒为 0
int pfs_erofs_sbi_can_write(const pfs_erofs_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 erofs 布局
int pfs_erofs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 erofs 镜像
int pfs_erofs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_EROFS_H_

// end
