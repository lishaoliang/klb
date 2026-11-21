// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_romfs_internal.h
/// @brief   romfs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_ROMFS_INTERNAL_H_
#define _PFS_ROMFS_INTERNAL_H_

#include "romfs/pfs_romfs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_romfs_map_blkio_rc(int rc);

// super/inode

/// @brief 读取 super 与 root 位置 (对照 fs/romfs/super.c)
int pfs_romfs_read_super(pfs_blkio_t *p_blkio, int64_t part_offset, uint32_t *p_img_size, uint32_t *p_root_pos);
/// @brief 按偏移读取 filehdr 与可选名
int pfs_romfs_read_fh(pfs_romfs_sbi_t *p_sbi, uint32_t pos, pfs_romfs_fh_t *p_fh, char *name, int name_cap, int *p_nlen);
/// @brief 解析 filehdr 字段
int pfs_romfs_resolve_fh(pfs_romfs_sbi_t *p_sbi, uint32_t pos, pfs_romfs_fh_t *p_fh);

/// @brief 路径查找 filehdr
int pfs_romfs_lookup_path(pfs_romfs_sbi_t *p_sbi, const char *path, pfs_romfs_fh_t *p_fh, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_romfs_access(pfs_romfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_romfs_mkdir(pfs_romfs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 删除空目录
int pfs_romfs_rmdir(pfs_romfs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_romfs_rename(pfs_romfs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_romfs_unlink(pfs_romfs_sbi_t *p_sbi, const char *path);

/// @brief 打开文件
int pfs_romfs_file_open(pfs_romfs_sbi_t *p_sbi, const char *path, int mode, pfs_romfs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_romfs_file_close(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_romfs_file_read(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_romfs_file_write(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_romfs_file_truncate(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_romfs_file_sync(pfs_romfs_sbi_t *p_sbi, pfs_romfs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_romfs_file_get_size(pfs_romfs_sbi_t *p_sbi, const pfs_romfs_file_meta_t *p_fm);

/// @brief 打开目录
int pfs_romfs_dir_open(pfs_romfs_sbi_t *p_sbi, const char *path, pfs_romfs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_romfs_dir_close(pfs_romfs_sbi_t *p_sbi, pfs_romfs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_romfs_dir_readdir(pfs_romfs_sbi_t *p_sbi, pfs_romfs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_ROMFS_INTERNAL_H_

// end
