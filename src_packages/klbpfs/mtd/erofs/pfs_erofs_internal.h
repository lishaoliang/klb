// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_erofs_internal.h
/// @brief   erofs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_EROFS_INTERNAL_H_
#define _PFS_EROFS_INTERNAL_H_

#include "erofs/pfs_erofs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_erofs_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_erofs_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 从缓冲区读取 LE uint64
static inline uint64_t pfs_erofs_le64(const uint8_t *p)
{
    return (uint64_t)pfs_erofs_le32(p) | ((uint64_t)pfs_erofs_le32(p + 4) << 32);
}

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_erofs_map_blkio_rc(int rc);

/// @brief 读取 superblock
int pfs_erofs_read_sb(pfs_blkio_t *p_blkio, int64_t part_offset, pfs_erofs_super_block_t *p_sb);
/// @brief 按 nid 读取 inode
int pfs_erofs_read_inode(pfs_erofs_sbi_t *p_sbi, uint64_t nid, pfs_erofs_inode_t *p_vi);
/// @brief 按偏移映射读取文件数据
int pfs_erofs_map_read(pfs_erofs_sbi_t *p_sbi, const pfs_erofs_inode_t *p_vi, int64_t pos, void *buf, int size, int *p_got);

/// @brief 路径查找 nid
int pfs_erofs_lookup_path(pfs_erofs_sbi_t *p_sbi, const char *path, uint64_t *p_nid, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_erofs_access(pfs_erofs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_erofs_mkdir(pfs_erofs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 删除空目录
int pfs_erofs_rmdir(pfs_erofs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_erofs_rename(pfs_erofs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_erofs_unlink(pfs_erofs_sbi_t *p_sbi, const char *path);

/// @brief 打开文件
int pfs_erofs_file_open(pfs_erofs_sbi_t *p_sbi, const char *path, int mode, pfs_erofs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_erofs_file_close(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_erofs_file_read(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_erofs_file_write(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_erofs_file_truncate(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_erofs_file_sync(pfs_erofs_sbi_t *p_sbi, pfs_erofs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_erofs_file_get_size(pfs_erofs_sbi_t *p_sbi, const pfs_erofs_file_meta_t *p_fm);

/// @brief 打开目录
int pfs_erofs_dir_open(pfs_erofs_sbi_t *p_sbi, const char *path, pfs_erofs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_erofs_dir_close(pfs_erofs_sbi_t *p_sbi, pfs_erofs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_erofs_dir_readdir(pfs_erofs_sbi_t *p_sbi, pfs_erofs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_EROFS_INTERNAL_H_

// end
