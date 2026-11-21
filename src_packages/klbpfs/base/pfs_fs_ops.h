// Doc Encode : UTF-8 BOM, Unix(LF)

///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_fs_ops.h
/// @brief   pfs_fs_* VFS 封装
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
/// 子文件系统对接 VFS 的完整操作表
/// fat / exfat / ntfs / isofs / udf 各填一张 (或 FAT12/16/32 分表); vfs 按表分派, 不再 switch 调具体符号
/// ext 表已填但未进默认 mount probe
//
/// 约定:
/// - 返回值: 成功 0 (PFS_OK) 或 PFS_* 负码; 适配层负责把子模块 RET_* 映到 pfs_ret_e
/// - p_sbi / p_fpriv / p_dpriv: 由 vfs 持有存储, 长度见 super.sbi_size / file.priv_size / dir.priv_size
/// - mode (mount): PFS_MNT_RDONLY / PFS_MNT_RDWR
/// - mode (fopen): PFS_O_RD / PFS_O_RDWR, 可选 | PFS_O_CREAT
/// - mode (access): PFS_F_OK 或 R/W/X_OK 组合
/// - 文件 pos / seek / tell / feof / ferror: 由 vfs 维护; FS 只按传入 pos 读写
/// - path.check_path: vfs 在 mkdir/rename(CREAT)/fopen(CREAT) 等入口调用; flags 见 PFS_PATH_CHECK_*
/// - 典型序: ctrl.probe -> super.init -> super.mount; 失败或卸载: super.umount -> super.deinit
#ifndef _PFS_FS_OPS_H_
#define _PFS_FS_OPS_H_

#include "pfs.h"
#include "pfs_ops.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////////
// 虚接口函数封装 (调 p_fs_ops->* ; 返回值映到 pfs_ret_e; 表/回调缺失为 PFS_EINVAL / PFS_EOPNOTSUPP)

int pfs_fs_super_init(const pfs_fs_ops_t *p_fs_ops, void *p_sbi);
void pfs_fs_super_deinit(const pfs_fs_ops_t *p_fs_ops, void *p_sbi);

int pfs_fs_super_mount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);
int pfs_fs_super_remount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, int mode);
int pfs_fs_super_umount(const pfs_fs_ops_t *p_fs_ops, void *p_sbi);

int pfs_fs_super_is_mounted(const pfs_fs_ops_t *p_fs_ops, const void *p_sbi);
int pfs_fs_super_can_write(const pfs_fs_ops_t *p_fs_ops, const void *p_sbi);


int pfs_fs_file_open(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode, void *p_fpriv);
void pfs_fs_file_close(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv);

int pfs_fs_file_read(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got);
int pfs_fs_file_write(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put);

int pfs_fs_file_truncate(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv, int64_t size);
int pfs_fs_file_fdatasync(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_fpriv);
int64_t pfs_fs_file_get_size(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const void *p_fpriv);


int pfs_fs_dir_open(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, void *p_dpriv);
void pfs_fs_dir_close(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_dpriv);
int pfs_fs_dir_readdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent);


int pfs_fs_path_access(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode);
int pfs_fs_path_mkdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, int mode);
int pfs_fs_path_rmdir(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path);
int pfs_fs_path_rename(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *oldpath, const char *newpath);
int pfs_fs_path_unlink(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path);
int pfs_fs_path_check_path(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, const char *path, uint32_t flags);


int pfs_fs_ctrl_probe(const pfs_fs_ops_t *p_fs_ops, pfs_blkio_t *p_blkio, int64_t part_offset);
int pfs_fs_ctrl_mkfs(const pfs_fs_ops_t *p_fs_ops, pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

int pfs_fs_ctrl_fsync(const pfs_fs_ops_t *p_fs_ops, void *p_sbi);
int pfs_fs_ctrl_ioctl(const pfs_fs_ops_t *p_fs_ops, void *p_sbi, uint32_t cmd, void *arg, int arg_size);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_FS_OPS_H_

// end
