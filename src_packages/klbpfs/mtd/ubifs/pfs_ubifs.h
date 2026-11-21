// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ubifs.h
/// @brief   ubifs 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    ubifs 子模块公开头 (pfsmtd; 空盘 mkfs + 扫描缓存写子集)
///  \n 可写: RDWR mount 下 mkdir / creat / append; 无 TNC/GC/压缩/unlink
///  \n 目录: pfsmtd/src/ubifs/; 介质建议 PFUB
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UBIFS_H_
#define _PFS_UBIFS_H_

#include "pfs.h"
#include "ubifs/pfs_ubifs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define PFS_UBIFS_CACHE_INO_MAX         128
#define PFS_UBIFS_CACHE_DENT_MAX        256
#define PFS_UBIFS_CACHE_DATA_MAX        512

/// @brief mount 模式 (互斥; 仅 pfs_ubifs_sbi_t.mount_state)
typedef enum _pfs_ubifs_mount_state_e {
    PFS_UBIFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_UBIFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_UBIFS_MOUNT_RDWR     = 0x02,                   ///< 已挂载, 读写
} pfs_ubifs_mount_state_e;

/// @brief 扫描缓存: inode
typedef struct _pfs_ubifs_ic_t {
    uint64_t                inum;
    uint32_t                mode;
    uint64_t                size;
    uint32_t                nlink;
    uint64_t                sqnum;
    int                     used;
} pfs_ubifs_ic_t;

/// @brief 扫描缓存: 目录项
typedef struct _pfs_ubifs_dc_t {
    uint64_t                parent;
    uint64_t                inum;
    uint8_t                 type;
    uint16_t                nlen;
    uint64_t                sqnum;
    char                    name[PFS_UBIFS_MAX_NLEN + 1];
    int                     used;
} pfs_ubifs_dc_t;

/// @brief 扫描缓存: 数据节点引用
typedef struct _pfs_ubifs_dref_t {
    uint64_t                inum;
    uint32_t                block;
    uint32_t                size;
    int                     lnum;
    int                     offs; ///< 节点起始
    int                     data_offs; ///< 载荷相对节点
    uint64_t                sqnum;
    int                     used;
} pfs_ubifs_dref_t;

/// @brief ubifs superblock info
typedef struct _pfs_ubifs_sbi_t {
    pfs_ubifs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    int                     leb_size;
    int                     leb_cnt;
    int                     main_lebs;
    int                     main_first;
    int                     log_lebs;
    int                     lpt_lebs;
    int                     orph_lebs;
    uint64_t                max_sqnum;
    uint64_t                highest_inum;
    int                     write_lnum;
    int                     write_offs;
    pfs_ubifs_ic_t         *p_ics;
    pfs_ubifs_dc_t         *p_dcs;
    pfs_ubifs_dref_t       *p_drefs;
    int                     ic_cnt;
    int                     dc_cnt;
    int                     dref_cnt;
} pfs_ubifs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_ubifs_file_meta_t {
    uint64_t                    inum;
    uint64_t                    size;
    int                         writable;
} pfs_ubifs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_ubifs_dir_meta_t {
    uint64_t                    inum;
    int                         invent; ///< 0=.; 1=..; 2+=dent 下标游标
    int                         dent_idx;
} pfs_ubifs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_ubifs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_ubifs_init(pfs_ubifs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_ubifs_deinit(pfs_ubifs_sbi_t *p_sbi);

/// @brief 仅探测是否为 ubifs; 不修改 sbi
int pfs_ubifs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 扫描挂载; RDONLY 或 RDWR
int pfs_ubifs_mount(pfs_ubifs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写
int pfs_ubifs_remount(pfs_ubifs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_ubifs_umount(pfs_ubifs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY 或 RDWR)
int pfs_ubifs_sbi_is_mounted(const pfs_ubifs_sbi_t *p_sbi);

/// @brief 是否允许写 (RDWR 且已挂载)
int pfs_ubifs_sbi_can_write(const pfs_ubifs_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 ubifs 布局
int pfs_ubifs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 ubifs 镜像
int pfs_ubifs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_UBIFS_H_

// end
