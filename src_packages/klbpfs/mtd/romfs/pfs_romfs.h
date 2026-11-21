// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_romfs.h
/// @brief   romfs 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    romfs 子模块公开头 (pfsmtd; 无压缩只读子集)
///  \n 目录: pfsmtd/src/romfs/; 对照 ../linux-7.1.2/fs/romfs/
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ROMFS_H_
#define _PFS_ROMFS_H_

#include "pfs.h"
#include "romfs/pfs_romfs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_romfs_sbi_t.mount_state)
typedef enum _pfs_romfs_mount_state_e {
    PFS_ROMFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_ROMFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_ROMFS_MOUNT_RDWR     = 0x02,                   ///< 只读 FS; RDWR 拒绝
} pfs_romfs_mount_state_e;

/// @brief romfs superblock info
typedef struct _pfs_romfs_sbi_t {
    pfs_romfs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    uint32_t                img_size; ///< super.size
    uint32_t                root_pos; ///< 根目录 filehdr 偏移
} pfs_romfs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_romfs_file_meta_t {
    pfs_romfs_fh_t              fh;
} pfs_romfs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_romfs_dir_meta_t {
    pfs_romfs_fh_t              fh; ///< 目录自身 (已解 HRD)
    uint32_t                    cur; ///< 当前子项 filehdr; 0=结束
} pfs_romfs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_romfs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_romfs_init(pfs_romfs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_romfs_deinit(pfs_romfs_sbi_t *p_sbi);

/// @brief 仅探测是否为 romfs; 不修改 sbi
int pfs_romfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 读取并解析 super; 挂载只读卷
int pfs_romfs_mount(pfs_romfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写; RDWR 拒绝
int pfs_romfs_remount(pfs_romfs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_romfs_umount(pfs_romfs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY)
int pfs_romfs_sbi_is_mounted(const pfs_romfs_sbi_t *p_sbi);

/// @brief 是否允许写; romfs 恒为 0
int pfs_romfs_sbi_can_write(const pfs_romfs_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 romfs 布局
int pfs_romfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 romfs 镜像
int pfs_romfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_ROMFS_H_

// end
