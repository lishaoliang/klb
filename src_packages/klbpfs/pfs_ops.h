// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ops.h
/// @brief   blkio 与 FS 扩展契约类型定义
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_OPS_H_
#define _PFS_OPS_H_

#include "pfs_export.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////////
// port file system type and block io type
typedef struct _pfs_blkio_t pfs_blkio_t;
typedef struct _pfs_dirent_t pfs_dirent_t;

/// @brief port file system block io type
/// @note 库内建: 0 .. PFS_BLKIO_TYPE_USER_MIN-1; 调用者自定义: [PFS_BLKIO_TYPE_USER_MIN, PFS_BLKIO_TYPE_MAX]
typedef enum _pfs_blkio_type_e {
    PFS_BLKIO_RAW           = 0,               ///< 线性块设备或普通文件当盘
    PFS_BLKIO_VHD           = 1,               ///< Microsoft VHD
    PFS_BLKIO_VHDX          = 2,               ///< Microsoft VHDX
    PFS_BLKIO_QCOW2         = 3,               ///< QEMU qcow2
    PFS_BLKIO_VMDK          = 4,               ///< VMware VMDK
    PFS_BLKIO_VDI           = 5,               ///< VirtualBox VDI

    PFS_BLKIO_MTD_MIN       = 0x200,           ///< 标准外扩展 pfs-mtd 起始

    PFS_BLKIO_TYPE_USER_MIN = 0x1000,          ///< 调用者自定义类型起始 (含)
    PFS_BLKIO_TYPE_MAX      = 0x7FFF,          ///< 类型码上限 (含; 自定义止于此)
} pfs_blkio_type_e;

/// @brief 打开访问模式 (三选一; 取值与 POSIX O_RDONLY/O_WRONLY/O_RDWR 一致)
typedef enum _pfs_blkio_oflags_e {
    PFS_BLKIO_O_RDONLY      = 0,                ///< 只读
    PFS_BLKIO_O_WRONLY      = 1,                ///< 只写
    PFS_BLKIO_O_RDWR        = 2,                ///< 读写
} pfs_blkio_oflags_e;

/// @brief port file system type
/// @note 库内建: 0 .. PFS_FS_TYPE_USER_MIN-1; 调用者自定义 OS/FS 类型: [PFS_FS_TYPE_USER_MIN, PFS_FS_TYPE_MAX]
typedef enum _pfs_fs_type_e {
    PFS_NONE                = 0,               ///< 未识别 / 未挂载
    PFS_FAT12               = 1,               ///< FAT12
    PFS_FAT16               = 2,               ///< FAT16
    PFS_FAT32               = 3,               ///< FAT32
    PFS_EXFAT               = 4,               ///< exFAT
    PFS_NTFS                = 5,               ///< NTFS

    PFS_ISOFS               = 10,              ///< ISO 9660 / isofs (只读; src/isofs)
    PFS_UDF                 = 11,              ///< UDF (Type1 子集可 RDWR; src/udf)

    PFS_EXT2                = 20,              ///< ext2 (src/ext4 共用; 子集)
    PFS_EXT3                = 21,              ///< ext3 (src/ext4 共用; 子集)
    PFS_EXT4                = 22,              ///< ext4 (src/ext4; 子集)

    PFS_MTD_MIN             = 0x200,           ///< 标准外扩展 pfs-mtd 起始

    PFS_FS_TYPE_USER_MIN    = 0x1000,          ///< 调用者自定义类型起始 (含)
    PFS_FS_TYPE_MAX         = 0x7FFF,          ///< 类型码上限 (含; 自定义止于此)
} pfs_fs_type_e;

////////////////////////////////////////////////////////////
// port file system block io

/// @brief blkio 可扩展后端操作表; facade 按表分派
/// @note raw / vhd / vhdx / qcow2 / vmdk / vdi 各填一张; facade 按表分派
///  \n 返回值: 对齐 PFS_BLKIO_RET_* (pread/pwrite 成功返回传输字节数)
///  \n offset: 逻辑盘字节偏移 (非宿主机文件偏移; 镜像后端自行映射)
///  \n priv: 后端私有态, 由该后端 open 分配, close 释放; 挂在 pfs_blkio_t.priv
///  \n 线程: 同一 pfs_blkio_t 串行
///  \n 镜像后端宜内嵌 raw 句柄读宿主文件
///  \n open_direct: 主要为 raw; 镜像可置 NULL (facade 返回 PARAM/STATE)
///  \n 探测序 (默认 open): 本句柄 register 优先, 再 vhdx -> vhd -> qcow2 -> vmdk -> vdi -> raw
///  \n open_ex(type): 强制单后端, 不回退; 扩展可先 register 再 open_ex, 或 open_ops/attach
///  \n register: 每句柄插表, 无 unregister (对称 pfs_ctx_register_fs; 非进程全局表)
typedef struct _pfs_blkio_ops_t {
    const char           *name;         ///< 如 "raw" / "vhd"
    pfs_blkio_type_e      type;

    /// @brief 打开路径并填后端 priv
    /// @param [in,out] p_blkio       块设备句柄
    /// @param [in]     path          宿主路径
    /// @param [in]     flags         PFS_BLKIO_O_*
    /// @param [in]     sector_size   缓冲 open 逻辑扇区 (raw); 镜像可忽略或校验
    /// @return int PFS_BLKIO_RET_OK 成功
    int     (*open)(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int sector_size);

    /// @brief Linux O_DIRECT / Win NO_BUFFERING 打开
    /// @param [in] align_size   PFS_BLKIO_ALIGN_512 / _1K / _2K / _4K; 镜像可 NULL
    int     (*open_direct)(pfs_blkio_t *p_blkio, const char *path, pfs_blkio_oflags_e flags, int align_size);

    /// @brief 释放 priv 与下层资源; 允许 p_blkio 已部分失败
    int     (*close)(pfs_blkio_t *p_blkio);

    /// @brief 逻辑盘读; 成功返回传输字节数; 失败 PFS_BLKIO_RET_*
    int     (*pread)(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);

    /// @brief 逻辑盘写; 成功返回传输字节数; 失败 PFS_BLKIO_RET_*
    int     (*pwrite)(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);

    int     (*fsync)(pfs_blkio_t *p_blkio);

    /// @brief 虚拟盘容量 [单位:字节]; 失败返回 <0 (如 PFS_BLKIO_RET_STATE)
    int64_t (*get_size)(const pfs_blkio_t *p_blkio);

    /// @brief 逻辑扇区字节数 (512/1024/2048/4096); 失败 <0
    int     (*logical_sector_size)(const pfs_blkio_t *p_blkio);
} pfs_blkio_ops_t;


////////////////////////////////////////////////////////////
// super: 卷生命周期

/// @brief FS 卷生命周期回调表 (superblock / mount)
typedef struct _pfs_fs_super_ops_t {
    int     sbi_size;                   ///< sizeof(对应 *_sbi_t)

    int     (*init)(void *p_sbi);
    void    (*deinit)(void *p_sbi);

    int     (*mount)(void *p_sbi, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);
    int     (*remount)(void *p_sbi, int mode);
    int     (*umount)(void *p_sbi);

    /// @brief 非 0=已挂载 (vfs 门禁用)
    int     (*is_mounted)(const void *p_sbi);

    /// @brief 非 0=可写 (vfs 门禁用)
    int     (*can_write)(const void *p_sbi);
} pfs_fs_super_ops_t;

////////////////////////////////////////////////////////////
// file: 打开文件数据面 (对齐 pfs_fopen..ftruncate 中需下推部分)

/// @brief FS 打开文件数据面回调表; p_fpriv 由 vfs 按 priv_size 提供存储
typedef struct _pfs_fs_file_ops_t {
    int     priv_size;                    ///< [单位:字节] p_fpriv 缓冲长度

    /// @brief 打开文件; mode 含 CREAT 时由 FS 创建空文件再打开
    int     (*open)(void *p_sbi, const char *path, int mode, void *p_fpriv);
    void    (*close)(void *p_sbi, void *p_fpriv);

    /// @brief 读; *p_got 为实际字节数; 短读合法 (近 EOF)
    int     (*read)(void *p_sbi, void *p_fpriv, int64_t pos, void *buf, int size, int *p_got);

    /// @brief 写; *p_put 为实际字节数
    int     (*write)(void *p_sbi, void *p_fpriv, int64_t pos, const void *buf, int size, int *p_put);

    int     (*truncate)(void *p_sbi, void *p_fpriv, int64_t size);

    /// @brief 目录项 / MFT 等元数据回写; 对应 pfs_fflush / fclose 写模式路径下推
    int     (*fdatasync)(void *p_sbi, void *p_fpriv);

    /// @brief 当前逻辑文件大小 [单位:字节]; 失败返回 -1
    int64_t (*get_size)(void *p_sbi, const void *p_fpriv);
} pfs_fs_file_ops_t;

////////////////////////////////////////////////////////////
// dir: 目录流 (对齐 pfs_opendir / closedir / readdir)

/// @brief FS 目录流回调表; p_dpriv 由 vfs 按 priv_size 提供存储
typedef struct _pfs_fs_dir_ops_t {
    int     priv_size;                    ///< [单位:字节] p_dpriv 缓冲长度

    int     (*open)(void *p_sbi, const char *path, void *p_dpriv);
    void    (*close)(void *p_sbi, void *p_dpriv);

    /// @brief 读下一目录项; 成功填 *p_ent 并返回 1; EOF 返回 0; 出错 PFS_* 负码
    int     (*readdir)(void *p_sbi, void *p_dpriv, pfs_dirent_t *p_ent);
} pfs_fs_dir_ops_t;

////////////////////////////////////////////////////////////
// path: 无长寿命句柄的路径操作 (对齐 pfs_access..unlink; 不含 remove)

/// @def   PFS_PATH_CHECK_CREAT
/// @brief check_path flags: 创建类 (mkdir/rename 目标/fopen|CREAT)
#define PFS_PATH_CHECK_CREAT        0x01u

/// @def   PFS_PATH_CHECK_EXIST
/// @brief check_path flags: 访问已有路径 (可较宽; 各 FS 自定)
#define PFS_PATH_CHECK_EXIST        0x02u

typedef struct _pfs_fs_path_ops_t {
    int     (*access)(void *p_sbi, const char *path, int mode);
    int     (*mkdir)(void *p_sbi, const char *path, int mode);
    int     (*rmdir)(void *p_sbi, const char *path);
    int     (*rename)(void *p_sbi, const char *oldpath, const char *newpath);
    int     (*unlink)(void *p_sbi, const char *path);

    /// @brief 路径/组件合法性; p_sbi 已挂载; flags 见 PFS_PATH_CHECK_*; 未实现可 NULL
    int     (*check_path)(void *p_sbi, const char *path, uint32_t flags);
} pfs_fs_path_ops_t;


////////////////////////////////////////////////////////////
// ctrl: 控制

/// @brief FS 控制回调表 (probe / mkfs / fsync / ioctl)
typedef struct _pfs_fs_ctrl_ops_t {
    /// @brief 仅探测; 不改 p_sbi; 成功 0
    int     (*probe)(pfs_blkio_t *p_blkio, int64_t part_offset);

    /// @brief 不经 sbi 格式化; 勿对已挂载卷调用; FAT12/16/32 分表时各指向 mkfs12/16/32
    int     (*mkfs)(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes);

    /// @brief 元数据同步
    int     (*fsync)(void *p_sbi);

    /// @brief 通常需已 mount (p_sbi 有效); 未实现可 NULL -> PFS_EOPNOTSUPP
    int     (*ioctl)(void *p_sbi, uint32_t cmd, void *arg, int arg_size);
} pfs_fs_ctrl_ops_t;


/// @brief 一种可挂载 FS 后端的完整对接表
/// @note 由 fat / exfat / ntfs (或 FAT12/16/32 分表) 提供静态常量表
///  \n vfs 持有 const pfs_fs_ops_t *; 挂载成功后按表分派, 替代按 fs_type switch
///  \n 与 pfs.h 分段对应: super~ctx, ctrl~probe/mkfs/fsync, file~file, dir~dir, path~path
///  \n 本表不含 pfs_ctx / pfs_file / pfs_dir 句柄本身; 那些仍由 vfs 分配与维护
typedef struct _pfs_fs_ops_t {
    const char             *name;                       ///< 调试/日志用短名, 如 "fat32" / "exfat" / "ntfs"
    pfs_fs_type_e           fs_type;                    ///< 挂载成功后写入 ctx 的类型; PFS_FAT12..PFS_EXT4 等

    pfs_fs_super_ops_t      super;                      ///< 卷: init/mount/umount 等
    pfs_fs_file_ops_t       file;                       ///< 打开文件数据面 (p_fpriv 私有态)
    pfs_fs_dir_ops_t        dir;                        ///< 目录流 (p_dpriv 私有态)
    pfs_fs_path_ops_t       path;                       ///< 无句柄路径操作 (access/mkdir/..)

    pfs_fs_ctrl_ops_t       ctrl;                       ///< 控制
} pfs_fs_ops_t;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_OPS_H_

// end
