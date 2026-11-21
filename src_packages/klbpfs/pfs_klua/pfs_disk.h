// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_disk.h
/// @brief   kpfs 多分区 disk 会话 (blkio + 每分区 pfs_ctx_t)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    仅 src_klua 内使用; 不经 pfs_klua.h 导出
///  \n mount 容器镜像/块设备, probe 分区表, 每槽独立 ctx/mount
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_DISK_H_
#define _PFS_DISK_H_

#include "pfs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @brief 单分区槽 (mount 后填充; ctx 堆分配)
typedef struct pfs_disk_part_t {
    pfs_ctx_t       *p_ctx;         ///< 本分区 ctx; NULL 表示 create 失败
    int64_t          part_offset;     ///< [单位:字节] 相对整盘
    uint64_t         start_lba;     ///< [单位:扇区]
    uint64_t         size_lba;      ///< [单位:扇区]
    int              mounted;       ///< 非 0=已 mount
    int              mount_mode;      ///< PFS_MNT_* ; 未 mount 为 0
    int              mount_err;     ///< 末次 mount 失败码 (PFS_*); 成功为 PFS_OK
    pfs_fs_type_e    fs_type;       ///< mount 后有效; 否则 PFS_NONE
} pfs_disk_part_t;

/// @brief 分区 mount 风格未指定时沿用 default_mnt_style
#define PFS_DISK_MNT_STYLE_DFLT     (-1)

/// @brief mount 选项 (NULL 等同默认)
typedef struct pfs_disk_mount_opts_t {
    int              default_mnt_style; ///< PFS_KLUA_MNT_STYLE_*; 默认 r
    int              part_mnt_style[PFS_PART_ENTRY_MAX]; ///< 每槽; DFLT 用 default
    int              sector_size;   ///< 512/1K/2K/4K; 0 表示 512
    int64_t          part_offset;   ///< 无分区表时裸卷字节偏移 (默认 0)
} pfs_disk_mount_opts_t;

/// @brief 多分区 disk 会话
typedef struct pfs_disk_t {
    pfs_blkio_t              blkio;
    int                      blkio_open;    ///< 非 0=blkio 已 open
    char                    *image_path;    ///< mount 时镜像路径; deinit 释放
    int                      sector_size;   ///< 逻辑扇区字节
    int                      part_count;    ///< 有效分区槽数 (>=1)
    pfs_part_scheme_e        part_scheme;   ///< probe 结果
    pfs_disk_part_t          parts[PFS_PART_ENTRY_MAX];
} pfs_disk_t;

/// @brief 清零 disk; 不释放已 mount 资源 (mount 前或 deinit 后调用)
void pfs_disk_init(pfs_disk_t *p_disk);

/// @brief umount/destroy 全部分区 ctx, close blkio
void pfs_disk_deinit(pfs_disk_t *p_disk);

/// @brief 挂载镜像/块设备, probe 并 mount 各分区
/// @param [in, out] *p_disk         须 pfs_disk_init 后
/// @param [in]      *p_image_path   设备或镜像路径 UTF-8
/// @param [in]      *p_opts          NULL=默认 (default_mnt_style r, sector 512, part_offset 0)
/// @return int PFS_OK 成功 (至少 blkio open; 分区 mount 失败保留 mount_err); PFS_EINVAL / PFS_EIO 等
/// @note 已 mount 时先 umount 再重挂
int pfs_disk_mount(pfs_disk_t *p_disk, const char *p_image_path, const pfs_disk_mount_opts_t *p_opts);

/// @brief 将已 mount 分区 remount 到 mnt_style; 失败分区保持原 mount_mode
/// @param [in, out] *p_disk      已 mount 的 disk
/// @param [in]      part_index   <0 全部已 mount 分区; >=0 单槽 0-based
/// @param [in]      mnt_style    0=r, 1=rw ("w" 在 Lua 层归一为 rw)
/// @return int part_index<0: 至少一个成功 PFS_OK, 全败末次码, 无可试 PFS_EBUSY; >=0: 单槽 PFS_OK/错误码
int pfs_disk_remount(pfs_disk_t *p_disk, int part_index, int mnt_style);

/// @brief 卸载 disk 或单分区; part_index<0 整盘 umount, >=0 单槽 0-based
/// @param [in, out] *p_disk  disk 句柄
void pfs_disk_umount(pfs_disk_t *p_disk, int part_index);

/// @brief part_index<0: 非 0=整盘已 mount; >=0: 单槽 0-based 是否 mounted
int pfs_disk_is_mounted(const pfs_disk_t *p_disk, int part_index);

/// @brief 分区槽数 (未 mount 为 0)
int pfs_disk_part_count(const pfs_disk_t *p_disk);

/// @brief 查询分区槽 (只读); part_index 1-based
/// @return const pfs_disk_part_t* 成功; NULL 参数或序号非法
const pfs_disk_part_t *pfs_disk_part_info(const pfs_disk_t *p_disk, int part_index);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_DISK_H_

// end
