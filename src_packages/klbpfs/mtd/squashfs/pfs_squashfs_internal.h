// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_squashfs_internal.h
/// @brief   squashfs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    本模块自持 LE 读 (禁止 include portfs pfs_endian.h)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_SQUASHFS_INTERNAL_H_
#define _PFS_SQUASHFS_INTERNAL_H_

#include "squashfs/pfs_squashfs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_squashfs_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_squashfs_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 从缓冲区读取 LE uint64
static inline uint64_t pfs_squashfs_le64(const uint8_t *p)
{
    return (uint64_t)pfs_squashfs_le32(p) | ((uint64_t)pfs_squashfs_le32(p + 4) << 32);
}

/// @brief 将 raw block_size 解码为 uint32 (非法时 -1)
static inline int pfs_squashfs_block_size_u32(uint32_t raw)
{
    if ((raw >> 25) != 0u)
    {
        return -1;
    }
    return (int)raw;
}

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_squashfs_map_blkio_rc(int rc);

// block

/// @brief 读元数据流 (zlib 或未压缩)
int pfs_squashfs_read_metadata(pfs_squashfs_sbi_t *p_sbi, void *buf, uint64_t *p_block, int *p_offset,
        int length);
/// @brief 读数据块
int pfs_squashfs_read_data_block(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_block, int size_code,
        uint8_t *out, int out_cap, int *p_out_len);
/// @brief 读表数据
int pfs_squashfs_read_table(pfs_squashfs_sbi_t *p_sbi, uint64_t disk_off, void *buf, int length);
/// @brief 查找 fragment 磁盘块
int pfs_squashfs_frag_lookup(pfs_squashfs_sbi_t *p_sbi, unsigned int fragment, uint64_t *p_frag_block);

// inode

/// @brief 按 ino 读取 inode
int pfs_squashfs_read_inode(pfs_squashfs_sbi_t *p_sbi, uint64_t ino, pfs_squashfs_inode_info_t *p_info);
/// @brief 判断 inode 是否为目录
bool pfs_squashfs_inode_is_dir(const pfs_squashfs_inode_info_t *p_info);
/// @brief 判断 inode 是否为普通文件
bool pfs_squashfs_inode_is_reg(const pfs_squashfs_inode_info_t *p_info);

// namei

/// @brief 路径查找 inode
int pfs_squashfs_lookup_path(pfs_squashfs_sbi_t *p_sbi, const char *path, pfs_squashfs_inode_info_t *p_info);
/// @brief 检查路径访问权限
int pfs_squashfs_access(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_squashfs_mkdir(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 删除空目录
int pfs_squashfs_rmdir(pfs_squashfs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_squashfs_rename(pfs_squashfs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_squashfs_unlink(pfs_squashfs_sbi_t *p_sbi, const char *path);

// file

/// @brief 打开文件
int pfs_squashfs_file_open(pfs_squashfs_sbi_t *p_sbi, const char *path, int mode, pfs_squashfs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_squashfs_file_close(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_squashfs_file_read(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_squashfs_file_write(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_squashfs_file_truncate(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_squashfs_file_sync(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_squashfs_file_get_size(pfs_squashfs_sbi_t *p_sbi, const pfs_squashfs_file_meta_t *p_fm);

// dir

/// @brief 打开目录
int pfs_squashfs_dir_open(pfs_squashfs_sbi_t *p_sbi, const char *path, pfs_squashfs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_squashfs_dir_close(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_squashfs_dir_readdir(pfs_squashfs_sbi_t *p_sbi, pfs_squashfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_SQUASHFS_INTERNAL_H_

// end
