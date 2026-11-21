// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_yaffs.h
/// @brief   yaffs/yaffs2 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    yaffs/yaffs2 子模块公开头 (pfsmtd; YAFFS2 packed 镜像读写子集)
///  \n 目录: pfsmtd/src/yaffs/; 文件前缀 pfs_yaffs_*
///  \n 介质: raw 镜像 (chunk+spare); 主线无 fs/yaffs*; 出处见 pfs_yaffs_raw.h
///  \n 可写: mkfs 根对象头; mkdir/creat/EOF append; 无 GC/unlink/真 OOB ECC
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_YAFFS_H_
#define _PFS_YAFFS_H_

#include "pfs.h"
#include "yaffs/pfs_yaffs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_yaffs_sbi_t.mount_state)
typedef enum _pfs_yaffs_mount_state_e {
    PFS_YAFFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_YAFFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_YAFFS_MOUNT_RDWR     = 0x02,                   ///< 已挂载, 读写
} pfs_yaffs_mount_state_e;

/// @brief 数据 chunk 缓存 (chunk_id >= 1)
typedef struct _pfs_yaffs_dchunk_t {
    struct _pfs_yaffs_dchunk_t *next;
    uint32_t                chunk_id;
    uint32_t                seq;
    uint32_t                page; ///< 镜像页号 (0 起)
    uint32_t                n_bytes;
} pfs_yaffs_dchunk_t;

/// @brief 对象缓存
typedef struct _pfs_yaffs_obj_t {
    struct _pfs_yaffs_obj_t *next;
    uint32_t                obj_id;
    uint32_t                type;
    uint32_t                parent_id;
    uint32_t                mode;
    uint32_t                file_size;
    uint32_t                hdr_seq;
    uint32_t                hdr_page;
    int                     has_hdr;
    char                    name[PFS_YAFFS_MAX_NAME_LENGTH + 1];
    pfs_yaffs_dchunk_t     *chunks;
} pfs_yaffs_obj_t;

/// @brief yaffs superblock info
typedef struct _pfs_yaffs_sbi_t {
    pfs_yaffs_mount_state_e mount_state;
    pfs_blkio_t            *p_blkio;
    int64_t                 part_offset;
    int64_t                 flash_size;
    int                     chunk_size; ///< 数据区字节
    int                     spare_size; ///< OOB/spare 字节
    int                     page_size; ///< chunk_size + spare_size
    int64_t                 page_count;
    int64_t                 free_page; ///< 下一追加页号
    uint32_t                next_obj_id; ///< 下一对象 id (>=5)
    uint32_t                next_seq; ///< 下一 sequence
    pfs_yaffs_obj_t        *obj_list;
} pfs_yaffs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_yaffs_file_meta_t {
    uint32_t                    obj_id;
    uint32_t                    file_size;
    int                         writable;
} pfs_yaffs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_yaffs_dir_meta_t {
    uint32_t                    obj_id;
    int                         invent; ///< 0=.; 1=..; >=2 扫子对象
    pfs_yaffs_obj_t            *p_next;
} pfs_yaffs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_yaffs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_yaffs_init(pfs_yaffs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_yaffs_deinit(pfs_yaffs_sbi_t *p_sbi);

/// @brief 仅探测是否为 yaffs; 不修改 sbi
int pfs_yaffs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 扫描挂载; RDONLY 或 RDWR
int pfs_yaffs_mount(pfs_yaffs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写
int pfs_yaffs_remount(pfs_yaffs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_yaffs_umount(pfs_yaffs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY 或 RDWR)
int pfs_yaffs_sbi_is_mounted(const pfs_yaffs_sbi_t *p_sbi);

/// @brief 是否允许写 (RDWR 且已挂载)
int pfs_yaffs_sbi_can_write(const pfs_yaffs_sbi_t *p_sbi);

/// @brief 在 p_blkio 指定分区写入空 yaffs 布局 (根对象头)
int pfs_yaffs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 从 src_dir 预制 yaffs 镜像
int pfs_yaffs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_YAFFS_H_

// end
