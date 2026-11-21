// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfsmtd.h
/// @brief   pfsmtd 对外接口总入口 (MTD 扩展库)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    pfsmtd 对外接口总入口 (应用默认: #include "pfsmtd.h")
///  \n 依赖: -I portfs/src; 仅可引用对外头 pfs.h / pfs_blkio.h / pfs_ops.h / pfs_part.h (禁止 base/* vfs/* 等)
///  \n 本头声明: pfsmtd_* (MTD/PFUB + get/mkfs/mkvol) + PFSMTD_* / REGISTER
///  \n 约定: 对外公开头只落 pfsmtd/src 根本文件; 禁止 #include "子目录/..."
///  \n 扩展挂接: 填 pfs_blkio_ops_t (MTD/UBI) 与/或 pfs_fs_ops_t
///  \n 支持目标 FS: squashfs / ubifs / jffs2 / yaffs2 / erofs / cramfs / romfs
///  \n 固有只读: squashfs / erofs / cramfs / romfs (均可 mkfs; squashfs/romfs 可 mkvol 灌树)
///  \n 可写目标: ubifs / jffs2 / yaffs2 (mkfs + mkvol 灌树)
///  \n 实现落 pfsmtd/src/; 公共辅助见 base/; MTD 实现见 mtd/; 目标 FS 见各子目录
///  \n 前缀: 本头公开函数一律 pfsmtd_*; 实现文件/内部可 pfs_mtd_* / pfs_<fs>_*; 勿并入 portfs/src
///  \n 内核对照见各子目录说明 (yaffs 无主线目录)
///////////////////////////////////////////////////////////////////////////
#ifndef _PFSMTD_H_
#define _PFSMTD_H_

#include "pfs.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////
// mtd / ubi

/// @def   PFSMTD_REGISTER_BLKIO_ALL
/// @brief 向 blkio 注册全部 pfsmtd blkio 后端 (当前仅 UBI/PFUB)
#define PFSMTD_REGISTER_BLKIO_ALL(_BLKIO) \
    do { \
        pfs_blkio_register((_BLKIO), PFSMTD_BLKIO_UBI, pfsmtd_get_ubi()); \
    } while (0);

/// @brief pfs_blkio_type_e 值 (对齐 pfs_blkio_type_e)
enum {
    PFSMTD_BLKIO_UBI = PFS_BLKIO_MTD_MIN + 1,   ///< UBI/PFUB blkio 类型
};

/// @brief 静态 ubi/PFUB ops 表 (供 register / open_ops / open_ex)
/// @note  打开/关闭走标准 blkio: init + open_ops(pfsmtd_get_ubi()) / REGISTER + open/open_ex; close/deinit
/// @note  实现: mtd/pfs_mtd.c; on-disk 常量: mtd/pfs_mtd_raw.h; 内核对照: include/uapi/mtd/; drivers/mtd/ubi/
PFS_API const pfs_blkio_ops_t *pfsmtd_get_ubi(void);

/// @brief 创建空 PFUB 镜像 (覆盖写)
/// @param [in] path          输出路径
/// @param [in] leb_size      LEB 字节数; 须 >=512 且 512 对齐
/// @param [in] leb_count     LEB 个数; >=1
/// @return int PFS_OK; PFS_EINVAL / PFS_EIO / PFS_ENOMEM
PFS_API int pfsmtd_ubi_create(const char *path, uint32_t leb_size, uint32_t leb_count);

/// @brief 查询已 open 的 ubi 后端 LEB 字节数
/// @param [in] *p_blkio      blkio 句柄
/// @return int 成功返回 LEB 字节数; 失败 <0
PFS_API int pfsmtd_ubi_leb_size(const pfs_blkio_t *p_blkio);

/// @brief 查询已 open 的 ubi 后端 LEB 个数
/// @param [in] *p_blkio      blkio 句柄
/// @return int 成功返回 LEB 个数; 失败 <0
PFS_API int pfsmtd_ubi_leb_count(const pfs_blkio_t *p_blkio);

/// @brief 按 LEB 读 (映射到虚拟线性偏移; 经 pfs_blkio_pread)
/// @param [in] *p_blkio      blkio 句柄
/// @param [in] lnum          LEB 序号
/// @param [in] offs          LEB 内偏移
/// @param [out] *buf         输出缓冲
/// @param [in] len           读取字节数
/// @return int 成功返回 len; 失败 PFS_BLKIO_RET_*
PFS_API int pfsmtd_ubi_leb_read(pfs_blkio_t *p_blkio, int lnum, int offs, void *buf, int len);

/// @brief 按 LEB 写 (映射到虚拟线性偏移; 经 pfs_blkio_pwrite)
/// @param [in] *p_blkio      blkio 句柄
/// @param [in] lnum          LEB 序号
/// @param [in] offs          LEB 内偏移
/// @param [in] *buf          输入数据
/// @param [in] len           写入字节数
/// @return int 成功返回 len; 失败 PFS_BLKIO_RET_*
PFS_API int pfsmtd_ubi_leb_write(pfs_blkio_t *p_blkio, int lnum, int offs, const void *buf, int len);

/// @brief 创建线性 raw 镜像 (全卷填 0xFF, 模拟 NOR 擦除态; 覆盖写)
/// @param [in] path          输出路径
/// @param [in] nbytes        卷字节数; 须 >=512 且 512 对齐
/// @return int PFS_OK; PFS_EINVAL / PFS_EIO / PFS_ENOMEM
PFS_API int pfsmtd_raw_create(const char *path, uint64_t nbytes);

////////////////////////////////////////////////////////////
// fs

/// @def   PFSMTD_REGISTER_FS_ALL
/// @brief 向 ctx 注册全部 pfsmtd 目标 FS ops 表
#define PFSMTD_REGISTER_FS_ALL(_CTX) \
    do { \
        pfs_ctx_register_fs((_CTX), PFSMTD_SQUASHFS, pfsmtd_get_squashfs()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_UBIFS, pfsmtd_get_ubifs()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_JFFS2, pfsmtd_get_jffs2()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_YAFFS, pfsmtd_get_yaffs()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_EROFS, pfsmtd_get_erofs()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_CRAMFS, pfsmtd_get_cramfs()); \
        pfs_ctx_register_fs((_CTX), PFSMTD_ROMFS, pfsmtd_get_romfs()); \
    } while (0);

/// @brief pfs_fs_type_e 值 (对齐 pfs_fs_type_e)
enum {
    PFSMTD_SQUASHFS = PFS_MTD_MIN + 1,    ///< squashfs
    PFSMTD_UBIFS    = PFS_MTD_MIN + 2,    ///< ubifs
    PFSMTD_JFFS2    = PFS_MTD_MIN + 3,    ///< jffs2
    PFSMTD_YAFFS    = PFS_MTD_MIN + 4,    ///< yaffs/yaffs2
    PFSMTD_EROFS    = PFS_MTD_MIN + 5,    ///< erofs
    PFSMTD_CRAMFS   = PFS_MTD_MIN + 6,    ///< cramfs
    PFSMTD_ROMFS    = PFS_MTD_MIN + 7,    ///< romfs
};

/// @brief 获取 squashfs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_squashfs();

/// @brief 获取 ubifs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_ubifs();

/// @brief 获取 jffs2 ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_jffs2();

/// @brief 获取 yaffs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_yaffs();

/// @brief 获取 erofs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_erofs();

/// @brief 获取 cramfs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_cramfs();

/// @brief 获取 romfs ops 表
PFS_API const pfs_fs_ops_t *pfsmtd_get_romfs();

/// @brief 按 PFSMTD_* 分派到各 pfs_*_mkfs
/// @param [in] *p_blkio      blkio 句柄
/// @param [in] part_offset   分区字节偏移
/// @param [in] vol_bytes     卷字节数
/// @param [in] fs_type       PFSMTD_* 类型
/// @return int PFS_OK; 未知类型 EINVAL; 未实现 EOPNOTSUPP
PFS_API int pfsmtd_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type);

/// @brief 预制卷选项 (mkvol)
typedef struct _pfsmtd_mkvol_opts_t {
    const char             *volume_id;                  ///< 卷标 UTF-8; NULL/空串用各栈默认
    uint32_t                jffs2_erase_size;           ///< jffs2 擦除块字节数; 0=线性 (仅 pfs 自测)
} pfsmtd_mkvol_opts_t;

/// @brief 按 PFSMTD_* 分派到各 pfs_*_mkvol (从 src_dir 预制/打包)
/// @param [in] *p_blkio      blkio 句柄
/// @param [in] part_offset   分区字节偏移
/// @param [in] vol_bytes     卷字节数; 可写类传给 mkfs; 打包类暂不参与布局
/// @param [in] fs_type       PFSMTD_* 类型
/// @param [in] src_dir       源目录树路径
/// @param [in] *opts         选项; 可 NULL
/// @return int PFS_OK; 未知类型 EINVAL; 未实现 EOPNOTSUPP
PFS_API int pfsmtd_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type, const char *src_dir, const pfsmtd_mkvol_opts_t *opts);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFSMTD_H_

// end
