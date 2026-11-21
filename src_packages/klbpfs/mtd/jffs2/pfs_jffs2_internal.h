// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_jffs2_internal.h
/// @brief   jffs2 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    本模块自持 LE 读 (禁止 include portfs pfs_endian.h)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_JFFS2_INTERNAL_H_
#define _PFS_JFFS2_INTERNAL_H_

#include "jffs2/pfs_jffs2.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_jffs2_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_jffs2_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_jffs2_map_blkio_rc(int rc);

// scan

/// @brief 释放扫描缓存
void pfs_jffs2_scan_free(pfs_jffs2_sbi_t *p_sbi);
/// @brief 全介质线性扫描 (对照 scan.c 子集)
int pfs_jffs2_scan_medium(pfs_jffs2_sbi_t *p_sbi);
/// @brief 探测 flash 上 jffs2 magic
int pfs_jffs2_probe_magic(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t flash_size);

/// @brief 按 ino 查找 inode 缓存项
pfs_jffs2_ic_t *pfs_jffs2_ic_find(pfs_jffs2_sbi_t *p_sbi, uint32_t ino);
/// @brief 判断 inode 缓存项是否为目录
bool pfs_jffs2_ic_is_dir(const pfs_jffs2_ic_t *p_ic);

// write

/// @def PFS_JFFS2_WRITE_CHUNK_MAX
/// @brief 单次写入块上限
#define PFS_JFFS2_WRITE_CHUNK_MAX       4096

/// @brief 计算 CRC32
uint32_t pfs_jffs2_crc32(const void *buf, size_t len);
/// @brief 追加原始节点数据 (无擦除/GC)
int pfs_jffs2_append_raw(pfs_jffs2_sbi_t *p_sbi, const void *buf, uint32_t len);
/// @brief 发出 inode 节点
int pfs_jffs2_emit_inode(pfs_jffs2_sbi_t *p_sbi, uint32_t ino, uint32_t version, uint32_t mode, uint32_t isize, uint32_t offset, uint32_t dsize, const void *data, uint8_t compr);
/// @brief 发出 dirent 节点
int pfs_jffs2_emit_dirent(pfs_jffs2_sbi_t *p_sbi, uint32_t pino, uint32_t version, uint32_t ino, uint8_t type, const char *name, int nlen);
/// @brief 同步介质
int pfs_jffs2_sync_media(pfs_jffs2_sbi_t *p_sbi);

// namei

/// @brief 路径查找 ino
int pfs_jffs2_lookup_path(pfs_jffs2_sbi_t *p_sbi, const char *path, uint32_t *p_ino, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_jffs2_access(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_jffs2_mkdir(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建文件; perm 为 0777 权限位, 0 则 0644
int pfs_jffs2_creat(pfs_jffs2_sbi_t *p_sbi, const char *path, uint32_t perm, uint32_t *p_ino);
/// @brief 删除空目录
int pfs_jffs2_rmdir(pfs_jffs2_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_jffs2_rename(pfs_jffs2_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_jffs2_unlink(pfs_jffs2_sbi_t *p_sbi, const char *path);

// file

/// @brief 打开文件
int pfs_jffs2_file_open(pfs_jffs2_sbi_t *p_sbi, const char *path, int mode, pfs_jffs2_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_jffs2_file_close(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_jffs2_file_read(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_jffs2_file_write(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_jffs2_file_truncate(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_jffs2_file_sync(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_jffs2_file_get_size(pfs_jffs2_sbi_t *p_sbi, const pfs_jffs2_file_meta_t *p_fm);

// dir

/// @brief 打开目录
int pfs_jffs2_dir_open(pfs_jffs2_sbi_t *p_sbi, const char *path, pfs_jffs2_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_jffs2_dir_close(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_jffs2_dir_readdir(pfs_jffs2_sbi_t *p_sbi, pfs_jffs2_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_JFFS2_INTERNAL_H_

// end
