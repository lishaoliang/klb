// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_cramfs_internal.h
/// @brief   cramfs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CRAMFS_INTERNAL_H_
#define _PFS_CRAMFS_INTERNAL_H_

#include "cramfs/pfs_cramfs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_cramfs_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_cramfs_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_cramfs_map_blkio_rc(int rc);

/// @brief 读取并解析 superblock 与 root inode
int pfs_cramfs_read_super(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t *p_sb_rel, pfs_cramfs_super_t *p_sb, pfs_cramfs_inode_t *p_root);
/// @brief 按绝对偏移读取 inode 与可选名
int pfs_cramfs_read_inode_at(pfs_cramfs_sbi_t *p_sbi, uint32_t abs_off, pfs_cramfs_inode_t *p_ino, char *name, int name_cap, int *p_nlen);
/// @brief 读取压缩页数据
int pfs_cramfs_read_page(pfs_cramfs_sbi_t *p_sbi, const pfs_cramfs_inode_t *p_ino, uint32_t data_off, uint32_t page_idx, uint8_t *out_page, int *p_out_len);

/// @brief 路径查找 inode
int pfs_cramfs_lookup_path(pfs_cramfs_sbi_t *p_sbi, const char *path, pfs_cramfs_inode_t *p_ino, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_cramfs_access(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_cramfs_mkdir(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 删除空目录
int pfs_cramfs_rmdir(pfs_cramfs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_cramfs_rename(pfs_cramfs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_cramfs_unlink(pfs_cramfs_sbi_t *p_sbi, const char *path);

/// @brief 打开文件
int pfs_cramfs_file_open(pfs_cramfs_sbi_t *p_sbi, const char *path, int mode, pfs_cramfs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_cramfs_file_close(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_cramfs_file_read(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_cramfs_file_write(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_cramfs_file_truncate(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_cramfs_file_sync(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_cramfs_file_get_size(pfs_cramfs_sbi_t *p_sbi, const pfs_cramfs_file_meta_t *p_fm);

/// @brief 打开目录
int pfs_cramfs_dir_open(pfs_cramfs_sbi_t *p_sbi, const char *path, pfs_cramfs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_cramfs_dir_close(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_cramfs_dir_readdir(pfs_cramfs_sbi_t *p_sbi, pfs_cramfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_CRAMFS_INTERNAL_H_

// end
