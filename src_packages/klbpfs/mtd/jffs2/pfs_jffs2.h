// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_jffs2.h
/// @brief   jffs2 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    jffs2 子模块公开头 (pfsmtd 目标 FS; 扫描挂载 + 读; RDWR 写子集)
///  \n 目录: pfsmtd/src/jffs2/; 文件前缀 pfs_jffs2_*
///  \n 介质: raw 镜像/MTD 线性区 (非 UBI); 仅可引用 portfs src 根对外头
///  \n 可写: mkfs 根 inode; mkdir/creat/EOF append/unlink/rmdir (COMPR_NONE); 无 GC/擦除块
///  \n 内核对照: ../linux-7.1.2/fs/jffs2/; uapi/linux/jffs2.h
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_JFFS2_H_
#define _PFS_JFFS2_H_

#include "pfs.h"
#include "jffs2/pfs_jffs2_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_jffs2_sbi_t.mount_state)
typedef enum _pfs_jffs2_mount_state_e {
    PFS_JFFS2_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_JFFS2_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_JFFS2_MOUNT_RDWR     = 0x02,                   ///< 已挂载, 读写
} pfs_jffs2_mount_state_e;

/// @brief 扫描缓存: 数据节点 (对照 raw_inode 落点)
typedef struct _pfs_jffs2_dnode_t {
    struct _pfs_jffs2_dnode_t *next;
    uint32_t                flash_ofs; ///< 相对 part_offset
    uint32_t                version;
    uint32_t                offset;
    uint32_t                csize;
    uint32_t                dsize;
    uint8_t                 compr;
} pfs_jffs2_dnode_t;

/// @brief 扫描缓存: 目录项 (同名保留高 version)
typedef struct _pfs_jffs2_fd_t {
    struct _pfs_jffs2_fd_t *next;
    uint32_t                version;
    uint32_t                ino; ///< 0 = 已删除
    uint8_t                 type; ///< PFS_JFFS2_DT_*
    uint8_t                 nsize;
    char                    name[PFS_JFFS2_MAX_NAME_LEN + 1];
} pfs_jffs2_fd_t;

/// @brief 扫描缓存: inode
typedef struct _pfs_jffs2_ic_t {
    struct _pfs_jffs2_ic_t *next;
    uint32_t                ino;
    uint32_t                mode;
    uint32_t                isize;
    uint32_t                highest_version;
    pfs_jffs2_dnode_t      *nodes;
    pfs_jffs2_fd_t         *dents;
} pfs_jffs2_ic_t;

/// @brief jffs2 superblock info
typedef struct _pfs_jffs2_sbi_t {
    pfs_jffs2_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset; ///< [单位:字节]
    int64_t                 flash_size; ///< 分区/镜像可见长度
    int64_t                 free_ofs; ///< 追加写落点 (相对 part)
    uint32_t                erase_size; ///< 擦除块字节数; 0=线性追加 (仅 pfs 自测); >0 须与 MTD 一致
    uint32_t                next_ino; ///< 下一分配 ino (>=2)
    pfs_jffs2_ic_t         *ic_list;
} pfs_jffs2_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_jffs2_file_meta_t {
    uint32_t                    ino;
    uint32_t                    isize;
    int                         writable;
} pfs_jffs2_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_jffs2_dir_meta_t {
    uint32_t                    ino;
    int                         invent; ///< 0=.; 1=..; >=2 扫 dents
    pfs_jffs2_fd_t             *p_next;
} pfs_jffs2_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_jffs2_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_jffs2_init(pfs_jffs2_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_jffs2_deinit(pfs_jffs2_sbi_t *p_sbi);

/// @brief 仅探测是否为 jffs2; 不修改 sbi
int pfs_jffs2_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 扫描挂载; RDONLY 或 RDWR
int pfs_jffs2_mount(pfs_jffs2_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写
int pfs_jffs2_remount(pfs_jffs2_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_jffs2_umount(pfs_jffs2_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY 或 RDWR)
int pfs_jffs2_sbi_is_mounted(const pfs_jffs2_sbi_t *p_sbi);

/// @brief 是否允许写 (RDWR 且已挂载)
int pfs_jffs2_sbi_can_write(const pfs_jffs2_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 jffs2 布局 (根 inode)
int pfs_jffs2_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 jffs2 镜像
int pfs_jffs2_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

/// @brief 从 src_dir 预制 jffs2 镜像 (可指定擦除块, 供 Linux 内核挂载)
int pfs_jffs2_mkvol_ex(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id, uint32_t erase_size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_JFFS2_H_

// end
