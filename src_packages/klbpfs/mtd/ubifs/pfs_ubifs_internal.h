// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ubifs_internal.h
/// @brief   ubifs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_UBIFS_INTERNAL_H_
#define _PFS_UBIFS_INTERNAL_H_

#include "ubifs/pfs_ubifs.h"
#include "ubifs/pfs_ubifs_key.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_ubifs_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_ubifs_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 从缓冲区读取 LE uint64
static inline uint64_t pfs_ubifs_le64(const uint8_t *p)
{
    return (uint64_t)pfs_ubifs_le32(p) | ((uint64_t)pfs_ubifs_le32(p + 4) << 32);
}

/// @brief 写入 LE uint16 到缓冲区
void pfs_ubifs_put_le16(uint8_t *p, uint16_t v);
/// @brief 写入 LE uint32 到缓冲区
void pfs_ubifs_put_le32(uint8_t *p, uint32_t v);
/// @brief 写入 LE uint64 到缓冲区
void pfs_ubifs_put_le64(uint8_t *p, uint64_t v);

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_ubifs_map_blkio_rc(int rc);

/// @brief 计算节点 CRC
uint32_t pfs_ubifs_crc_node(const void *node, int len);
/// @brief 填充节点公共头
void pfs_ubifs_prepare_node(void *node, int len, uint8_t node_type, uint64_t sqnum);

/// @brief 从 LEB 读取数据
int pfs_ubifs_leb_read(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len);
/// @brief 向 LEB 写入数据
int pfs_ubifs_leb_write(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, const void *buf, int len);
/// @brief 从 LEB 读取节点
int pfs_ubifs_read_node(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len);
/// @brief 向 LEB 写入节点
int pfs_ubifs_write_node(pfs_ubifs_sbi_t *p_sbi, int lnum, int offs, void *buf, int len);
/// @brief 校验节点 CRC 与 magic
int pfs_ubifs_check_node(const void *buf, int len);

/// @brief 分配扫描缓存
int pfs_ubifs_cache_alloc(pfs_ubifs_sbi_t *p_sbi);
/// @brief 释放扫描缓存
void pfs_ubifs_cache_free(pfs_ubifs_sbi_t *p_sbi);
/// @brief 扫描介质并重建索引
int pfs_ubifs_scan_rebuild(pfs_ubifs_sbi_t *p_sbi);
/// @brief 追加节点到日志
int pfs_ubifs_append_node(pfs_ubifs_sbi_t *p_sbi, void *node, int len);
/// @brief 刷写 master 节点
int pfs_ubifs_flush_mst(pfs_ubifs_sbi_t *p_sbi);

/// @brief 按 inum 查找 inode 缓存项
pfs_ubifs_ic_t *pfs_ubifs_ic_find(pfs_ubifs_sbi_t *p_sbi, uint64_t inum);
/// @brief 按父目录与名查找 dirent 缓存项
pfs_ubifs_dc_t *pfs_ubifs_dc_find(pfs_ubifs_sbi_t *p_sbi, uint64_t parent, const char *name, int nlen);
/// @brief 插入或更新 inode 缓存项
int pfs_ubifs_ic_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t inum, uint32_t mode, uint64_t size, uint32_t nlink, uint64_t sqnum);
/// @brief 插入或更新 dirent 缓存项
int pfs_ubifs_dc_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t parent, uint64_t inum, uint8_t type, const char *name, int nlen, uint64_t sqnum);
/// @brief 插入或更新数据引用缓存项
int pfs_ubifs_dref_upsert(pfs_ubifs_sbi_t *p_sbi, uint64_t inum, uint32_t block, uint32_t size, int lnum, int offs, int data_offs, uint64_t sqnum);

/// @brief 路径查找 inum
int pfs_ubifs_lookup_path(pfs_ubifs_sbi_t *p_sbi, const char *path, uint64_t *p_inum, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_ubifs_access(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_ubifs_mkdir(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 删除空目录
int pfs_ubifs_rmdir(pfs_ubifs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_ubifs_rename(pfs_ubifs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_ubifs_unlink(pfs_ubifs_sbi_t *p_sbi, const char *path);
/// @brief 创建文件
int pfs_ubifs_creat(pfs_ubifs_sbi_t *p_sbi, const char *path, uint64_t *p_inum);

/// @brief 打开文件
int pfs_ubifs_file_open(pfs_ubifs_sbi_t *p_sbi, const char *path, int mode, pfs_ubifs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_ubifs_file_close(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_ubifs_file_read(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_ubifs_file_write(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_ubifs_file_truncate(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_ubifs_file_sync(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_ubifs_file_get_size(pfs_ubifs_sbi_t *p_sbi, const pfs_ubifs_file_meta_t *p_fm);

/// @brief 打开目录
int pfs_ubifs_dir_open(pfs_ubifs_sbi_t *p_sbi, const char *path, pfs_ubifs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_ubifs_dir_close(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_ubifs_dir_readdir(pfs_ubifs_sbi_t *p_sbi, pfs_ubifs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

/// @brief 解压数据块
int pfs_ubifs_decompress(int compr_type, const void *in, int in_len, void *out, int *p_out_len);

#ifdef __cplusplus
}
#endif

#endif // _PFS_UBIFS_INTERNAL_H_

// end
