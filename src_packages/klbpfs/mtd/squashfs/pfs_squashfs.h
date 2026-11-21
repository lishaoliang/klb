// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_squashfs.h
/// @brief   squashfs 子模块公开头
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    squashfs 子模块公开头 (pfs mtd 目标 FS; 只读镜像; zlib/v4 读 + mkfs/mkvol 可 gzip/未压缩)
///  \n 目录: pfsmtd/src/squashfs/; 文件前缀 pfs_squashfs_*
///  \n 仅可引用 portfs src 根对外头; 库内公共用 base/pfsmtd_*
///  \n 内核对照: linux-7.1.2/fs/squashfs/
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_SQUASHFS_H_
#define _PFS_SQUASHFS_H_

#include "pfs.h"
#include "squashfs/pfs_squashfs_raw.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief mount 模式 (互斥; 仅 pfs_squashfs_sbi_t.mount_state)
typedef enum _pfs_squashfs_mount_state_e {
    PFS_SQUASHFS_UNMOUNTED      = 0x00,                   ///< 未挂载
    PFS_SQUASHFS_MOUNT_RDONLY   = 0x01,                   ///< 已挂载, 只读
    PFS_SQUASHFS_MOUNT_RDWR     = 0x02,                   ///< 只读 FS; mount 拒绝 RDWR
} pfs_squashfs_mount_state_e;

/// @brief 内存 inode 摘要 (lookup/open 结果)
typedef struct _pfs_squashfs_inode_info_t {
    uint64_t                    ino; ///< MKINODE 定位
    uint32_t                    inode_number;
    uint16_t                    type; ///< PFS_SQUASHFS_*_TYPE
    uint16_t                    mode;
    uint64_t                    file_size; ///< 目录为 dir 表字节数
    uint64_t                    start; ///< 数据/目录相对表起点
    uint32_t                    offset; ///< 目录: 表内偏移; 文件: fragment 内偏移
    uint64_t                    block_list_block; ///< 文件: block_list 元数据位置
    int                         block_list_offset;
    uint32_t                    fragment; ///< 或 INVALID_FRAG
    uint64_t                    frag_block;
    int                         frag_size; ///< squashfs_block_size 编码; <0 无效
    uint16_t                    dir_idx_cnt;
    uint64_t                    dir_idx_block;
    int                         dir_idx_offset;
    uint32_t                    parent_inode;
} pfs_squashfs_inode_info_t;

/// @brief squashfs superblock info
typedef struct _pfs_squashfs_sbi_t {
    pfs_squashfs_mount_state_e  mount_state;
    pfs_blkio_t                *p_blkio;
    int64_t                     part_offset; ///< [单位:字节]
    uint32_t                    block_size;
    uint16_t                    block_log;
    uint16_t                    compression;
    uint16_t                    flags;
    uint16_t                    s_major;
    uint16_t                    s_minor;
    uint32_t                    inodes;
    uint32_t                    fragments;
    uint64_t                    bytes_used;
    uint64_t                    root_inode;
    uint64_t                    inode_table;
    uint64_t                    directory_table;
    uint64_t                    fragment_table_start;
    uint64_t                    id_table_start;
    uint64_t                   *fragment_index; ///< 挂载时加载; 可 NULL
    unsigned int                fragment_index_count;
    // 单槽元数据缓存 (解压后)
    uint64_t                    meta_block; ///< INVALID_BLK=空
    uint64_t                    meta_next;
    int                         meta_len;
    uint8_t                    *meta_data; ///< METADATA_SIZE
    uint8_t                    *comp_buf; ///< max(block_size, METADATA_SIZE)
    uint8_t                    *data_buf; ///< block_size (文件块解压)
    int                         data_buf_index; ///< 缓存的逻辑块号; -1 无
    uint64_t                    data_buf_ino; ///< 缓存所属文件 ino
} pfs_squashfs_sbi_t;

/// @brief 打开文件元数据
typedef struct _pfs_squashfs_file_meta_t {
    pfs_squashfs_inode_info_t   info;
} pfs_squashfs_file_meta_t;

/// @brief 打开目录元数据
typedef struct _pfs_squashfs_dir_meta_t {
    pfs_squashfs_inode_info_t   info;
    uint64_t                    block; ///< 当前目录元数据绝对起点
    int                         offset;
    int                         length; ///< 已消费内部长度 (含 . / .. 合成)
    int                         invent; ///< 0=发 "."; 1=发 ".."; 2+=磁盘项
    uint32_t                    remaining; ///< 当前 header 剩余 entry
    pfs_squashfs_dir_header_t   hdr;
    int                         hdr_valid;
} pfs_squashfs_dir_meta_t;

/// @brief vfs 对接表
extern const pfs_fs_ops_t g_pfs_squashfs_fs_ops;

/// @brief 重置为 UNMOUNTED
int pfs_squashfs_init(pfs_squashfs_sbi_t *p_sbi);

/// @brief 释放内资源; 不 close p_blkio
void pfs_squashfs_deinit(pfs_squashfs_sbi_t *p_sbi);

/// @brief 仅探测是否为 squashfs; 不修改 sbi
int pfs_squashfs_probe(pfs_blkio_t *p_blkio, int64_t part_offset);

/// @brief 读取并解析 super; 挂载只读卷
int pfs_squashfs_mount(pfs_squashfs_sbi_t *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写; RDWR 拒绝
int pfs_squashfs_remount(pfs_squashfs_sbi_t *p_sbi, int mode);

/// @brief 卸载; 清除 p_blkio, 置 UNMOUNTED
int pfs_squashfs_umount(pfs_squashfs_sbi_t *p_sbi);

/// @brief 是否已挂载 (RDONLY)
int pfs_squashfs_sbi_is_mounted(const pfs_squashfs_sbi_t *p_sbi);

/// @brief 是否允许写; squashfs 恒为 0
int pfs_squashfs_sbi_can_write(const pfs_squashfs_sbi_t *p_sbi);

/// @def   PFS_SQUASHFS_COMPRESS_DEFAULT
/// @brief mkfs/mkvol 压缩选项: 默认 = GZIP
#define PFS_SQUASHFS_COMPRESS_DEFAULT   0

/// @def   PFS_SQUASHFS_COMPRESS_NONE
/// @brief mkfs/mkvol 压缩选项: 未压缩
#define PFS_SQUASHFS_COMPRESS_NONE      1

/// @def   PFS_SQUASHFS_COMPRESS_GZIP
/// @brief mkfs/mkvol 压缩选项: zlib (COMP_ZLIB)
#define PFS_SQUASHFS_COMPRESS_GZIP      2

/// @brief 在 p_blkio 指定分区写入空 squashfs 布局 (默认 gzip 压缩)
/// @note  compress: PFS_SQUASHFS_COMPRESS_*; DEFAULT/0 -> gzip; 仅 NONE/GZIP; 其它 EINVAL
/// @note  数据与 fragment 可 gzip; 元数据表首版仍未压缩; super.compression 恒为 ZLIB
int pfs_squashfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

/// @brief 在 p_blkio 指定分区写入空 squashfs 布局 (指定 compress)
int pfs_squashfs_mkfs_ex(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, int compress);

/// @brief 从 src_dir 预制 squashfs 镜像 (默认 gzip 压缩)
int pfs_squashfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, const char *src_dir, const char *volume_id);

/// @brief 从 src_dir 预制 squashfs 镜像 (指定 compress)
int pfs_squashfs_mkvol_ex(pfs_blkio_t *p_blkio, int64_t part_offset, const char *src_dir, const char *volume_id,
        int compress);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_SQUASHFS_H_

// end
