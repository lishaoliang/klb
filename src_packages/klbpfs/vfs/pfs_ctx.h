// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_ctx.h
/// @brief   VFS 上下文完整定义 (模块内部)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    vfs 内部头; 对外仅见 src/pfs.h 不透明 pfs_ctx_t
///  \n 一次挂载一个卷; fs 与 fs_type 对应
///  \n 挂载态/几何等细节在各 sbi 内 (见各 sbi.mount_state)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_CTX_H_
#define _PFS_CTX_H_

#include "pfs.h"
#include "fat/pfs_fat.h"
#include "exfat/pfs_exfat.h"
#include "ntfs/pfs_ntfs.h"
#include "isofs/pfs_isofs.h"
#include "udf/pfs_udf.h"
#include "ext4/pfs_ext4.h"
#include "base/pfs_fs_ops.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @def   PFS_CTX_FS_OPS_MAX
/// @brief 本 ctx 可注册的外部 FS 表上限
#define PFS_CTX_FS_OPS_MAX          64

/// @def   PFS_CTX_SBI_BYTES_MAX
/// @brief 外部 sbi 优先落在 fs.sbi_buf; 亦抬高 union 下限; 实际可用为 sizeof(fs) (>= 本宏与各内建 sbi)
#define PFS_CTX_SBI_BYTES_MAX       8192

/// @brief VFS 挂载上下文 (完整定义)
struct _pfs_ctx_t {
    pfs_fs_type_e           fs_type;                    ///< NONE / FAT* / EXFAT / NTFS / UDF / ISOFS / EXT* / 外部
    pfs_blkio_t             *p_blkio;                   ///< 借用块设备; destroy 不 close
    int64_t                 part_offset;                ///< [单位:字节] 分区相对设备偏移; 裸卷/整盘为 0

    const pfs_fs_ops_t     *p_fs_ops;                   ///< 当前挂载的 FS 操作表
    void                   *p_sbi;                      ///< 当前挂载的 FS 句柄 (通常指向 fs)
    int                     sbi_owned;                  ///< 非 0: p_sbi 堆分配 (仅 sbi 大于 sizeof(fs) 时)

    const pfs_fs_ops_t     *fs_ops[PFS_CTX_FS_OPS_MAX]; ///< 本 ctx 注册的外部 FS 表 (指针, 不拷贝)
    int                     fs_ops_count;               ///< 已注册数量

    // 内建 typed 成员 + sbi_buf: 外部结构体复用本 union 内存 (p_sbi=&fs)
    union {
        uint8_t             sbi_buf[PFS_CTX_SBI_BYTES_MAX]; ///< 外部 sbi 落盘区

        pfs_fat_sbi_t       fat;                        ///< fs_type == PFS_FAT12 / FAT16 / FAT32
        pfs_exfat_sbi_t     exfat;                      ///< fs_type == PFS_EXFAT
        pfs_ntfs_sbi_t      ntfs;                       ///< fs_type == PFS_NTFS
        pfs_isofs_sbi_t     isofs;                      ///< fs_type == PFS_ISOFS
        pfs_udf_sbi_t       udf;                        ///< fs_type == PFS_UDF
        pfs_ext4_sbi_t      ext;                        ///< fs_type == PFS_EXT2 / EXT3 / EXT4
    } fs;
};

/// @brief 重置为 PFS_NONE
/// @param [in, out] *p_ctx    上下文句柄
/// @return int PFS_OK 成功; PFS_EINVAL 失败
int pfs_ctx_init(pfs_ctx_t *p_ctx);

/// @brief 若已挂载则 umount; 不 close p_blkio
/// @param [in, out] *p_ctx    上下文句柄
void pfs_ctx_deinit(pfs_ctx_t *p_ctx);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_CTX_H_

// end
