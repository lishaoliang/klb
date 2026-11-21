// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_yaffs_internal.h
/// @brief   yaffs 子模块内部 API
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_YAFFS_INTERNAL_H_
#define _PFS_YAFFS_INTERNAL_H_

#include "yaffs/pfs_yaffs.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief 从缓冲区读取 LE uint16
static inline uint16_t pfs_yaffs_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/// @brief 从缓冲区读取 LE uint32
static inline uint32_t pfs_yaffs_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/// @brief 将 blkio 返回码映射为本模块错误码
int pfs_yaffs_map_blkio_rc(int rc);

/// @brief 释放扫描缓存
void pfs_yaffs_scan_free(pfs_yaffs_sbi_t *p_sbi);
/// @brief 探测 flash 几何 (chunk/spare)
int pfs_yaffs_detect_geom(pfs_blkio_t *p_blkio, int64_t part_offset, int64_t flash_size, int *p_chunk, int *p_spare);
/// @brief 全介质扫描
int pfs_yaffs_scan_medium(pfs_yaffs_sbi_t *p_sbi);

/// @brief 按 obj_id 查找对象
pfs_yaffs_obj_t *pfs_yaffs_obj_find(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id);
/// @brief 判断对象是否为目录
bool pfs_yaffs_obj_is_dir(const pfs_yaffs_obj_t *p_obj);
/// @brief 判断对象是否可见 (未删除)
int pfs_yaffs_obj_visible(const pfs_yaffs_obj_t *p_obj);

// write

/// @brief 追加 packed 页 (无 GC/擦除)
int pfs_yaffs_emit_page(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id, uint32_t chunk_id, uint32_t n_bytes, const void *data);
/// @brief 发出 object header 页
int pfs_yaffs_emit_obj_hdr(pfs_yaffs_sbi_t *p_sbi, uint32_t obj_id, uint32_t type, uint32_t parent_id, uint32_t mode, uint32_t file_size, const char *name);
/// @brief 同步介质
int pfs_yaffs_sync_media(pfs_yaffs_sbi_t *p_sbi);

/// @brief 路径查找 obj_id
int pfs_yaffs_lookup_path(pfs_yaffs_sbi_t *p_sbi, const char *path, uint32_t *p_obj_id, bool *p_is_dir);
/// @brief 检查路径访问权限
int pfs_yaffs_access(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建目录
int pfs_yaffs_mkdir(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode);
/// @brief 创建文件
int pfs_yaffs_creat(pfs_yaffs_sbi_t *p_sbi, const char *path, uint32_t *p_obj_id);
/// @brief 删除空目录
int pfs_yaffs_rmdir(pfs_yaffs_sbi_t *p_sbi, const char *path);
/// @brief 重命名路径
int pfs_yaffs_rename(pfs_yaffs_sbi_t *p_sbi, const char *oldpath, const char *newpath);
/// @brief 删除文件
int pfs_yaffs_unlink(pfs_yaffs_sbi_t *p_sbi, const char *path);

/// @brief 打开文件
int pfs_yaffs_file_open(pfs_yaffs_sbi_t *p_sbi, const char *path, int mode, pfs_yaffs_file_meta_t *p_fm);
/// @brief 关闭文件
void pfs_yaffs_file_close(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm);
/// @brief 读取文件数据
int pfs_yaffs_file_read(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t pos, void *buf, int size, int *p_got);
/// @brief 写入文件数据
int pfs_yaffs_file_write(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t pos, const void *buf, int size, int *p_put);
/// @brief 截断文件
int pfs_yaffs_file_truncate(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm, int64_t size);
/// @brief 同步文件
int pfs_yaffs_file_sync(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_file_meta_t *p_fm);
/// @brief 获取文件大小
int64_t pfs_yaffs_file_get_size(pfs_yaffs_sbi_t *p_sbi, const pfs_yaffs_file_meta_t *p_fm);

/// @brief 打开目录
int pfs_yaffs_dir_open(pfs_yaffs_sbi_t *p_sbi, const char *path, pfs_yaffs_dir_meta_t *p_dm);
/// @brief 关闭目录
void pfs_yaffs_dir_close(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_dir_meta_t *p_dm);
/// @brief 读取目录项
int pfs_yaffs_dir_readdir(pfs_yaffs_sbi_t *p_sbi, pfs_yaffs_dir_meta_t *p_dm, pfs_dirent_t *p_ent);

#ifdef __cplusplus
}
#endif

#endif // _PFS_YAFFS_INTERNAL_H_

// end
