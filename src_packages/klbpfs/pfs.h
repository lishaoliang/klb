// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs.h
/// @brief   pfs(port file system)
///          pfs is a file system that is used to store data on a port device
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
/// @note    pfs 对外接口总入口 (应用默认: #include "pfs.h")
///  \n 本头聚合: pfs_ops.h / pfs_blkio.h / pfs_part.h; 并声明 vfs 段 (ctx/mkfs/file/dir/path)
///  \n 分册头可单独 include; 实现: vfs/ (本头声明) / base (blkio facade) / partitions (part)
///  \n 约定: src 根对外 API 若返回错误码, 一律为 pfs_ret_e (PFS_OK / PFS_*); 见 pfs_strerror
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_H_
#define _PFS_H_

#include "pfs_ops.h"
#include "pfs_blkio.h"
#include "pfs_part.h"
#include "klbbase/klb_basecode.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////////
// pfs 版本
// 格式同 klb KLB_VERSION: 主.次.修订.YYYYMM; 改 libpfs 自有 C/头后修订 +1 并刷新当月 YYYYMM

/// @brief pfs 库版本字符串
#define PFS_VERSION             "0.1.0.202610"
#define PFS_VERSION_DATE        202610

/// @brief pfs 库版本 数值
///    编码: 主*10000000 + 子*10000 + 修订 (int32)
///    主版本: 3 位, 0-214
///    子版本: 3 位, 0-999
///    修订:   4 位, 0-9999
#define PFS_VERSION_NUMBER      10000

/// @brief pfs 库版本 字符串
PFS_API const char *pfs_version(void);

/// @brief pfs 库版本 数值 / 日期
PFS_API int pfs_version_number(void);
PFS_API int pfs_version_date(void);

////////////////////////////////////////////////////////////

/// @brief API 返回值 (失败时 <0; 对外统一用本 enum, 子模块错误在封装内映射)
/// @note 数值 = -errno; 一律取 KLB_BASECODE_* 同名常量
typedef enum _pfs_ret_e {
    PFS_OK                  =  0,                       ///< 成功
    PFS_EPERM               = KLB_BASECODE_EPERM,       ///< 不允许的操作 (EPERM)
    PFS_ENOENT              = KLB_BASECODE_ENOENT,      ///< 路径不存在 (ENOENT)
    PFS_EIO                 = KLB_BASECODE_EIO,         ///< I/O 失败 (EIO)
    PFS_ENOEXEC             = KLB_BASECODE_ENOEXEC,     ///< 非支持的文件系统 (ENOEXEC)
    PFS_ENOMEM              = KLB_BASECODE_ENOMEM,      ///< 内存不足 (ENOMEM)
    PFS_EBUSY               = KLB_BASECODE_EBUSY,       ///< 设备或资源忙 (EBUSY)
    PFS_EEXIST              = KLB_BASECODE_EEXIST,      ///< 目标已存在 (EEXIST)
    PFS_ENOTDIR             = KLB_BASECODE_ENOTDIR,     ///< 路径组件非目录 (ENOTDIR)
    PFS_EISDIR              = KLB_BASECODE_EISDIR,      ///< 目标是目录 (EISDIR)
    PFS_EINVAL              = KLB_BASECODE_EINVAL,      ///< 参数非法 (EINVAL)
    PFS_ENOSPC              = KLB_BASECODE_ENOSPC,      ///< 无剩余空间 (ENOSPC)
    PFS_EROFS               = KLB_BASECODE_EROFS,       ///< 只读文件系统 (EROFS)
    PFS_ERANGE              = KLB_BASECODE_ERANGE,      ///< 结果越界 (ERANGE)
    PFS_ENAMETOOLONG        = KLB_BASECODE_ENAMETOOLONG, ///< 文件名超长 (ENAMETOOLONG)
    PFS_ENOTEMPTY           = KLB_BASECODE_ENOTEMPTY,   ///< 目录非空 (ENOTEMPTY)
    PFS_EBADMSG             = KLB_BASECODE_EBADMSG,     ///< on-disk 元数据损坏 (EBADMSG)
    PFS_EILSEQ              = KLB_BASECODE_EILSEQ,      ///< 非法字节序列 (EILSEQ)
    PFS_EOPNOTSUPP          = KLB_BASECODE_EOPNOTSUPP,  ///< 未实现或不支持 (EOPNOTSUPP)
} pfs_ret_e;

/// @brief 将 pfs_ret_e / 同类负码转为只读描述串
/// @param [in] errnum    错误码
/// @return const char* 对齐 strerror; 均带 "[pfs] " 前缀; 未知码 "Unknown error"
PFS_API const char *pfs_strerror(int errnum);

////////////////////////////////////////////////////////////
// ctx / context handle

/// @def   PFS_MNT_RDONLY
/// @brief mount 模式: 只读挂载 (互斥; 传给 mount/remount; 取值对齐 fat/exfat MOUNT_*)
#define PFS_MNT_RDONLY              0x01

/// @def   PFS_MNT_RDWR
/// @brief mount 模式: 读写挂载
#define PFS_MNT_RDWR                0x02

/// @brief 不透明上下文句柄 / 一次一个卷 / 一次一个文件系统
typedef struct _pfs_ctx_t pfs_ctx_t;

/// @brief 分配并 init 空 ctx (未挂载)
/// @return pfs_ctx_t* 成功返回句柄; 失败返回 NULL
PFS_API pfs_ctx_t *pfs_ctx_create(void);

/// @brief deinit (若已挂载则 umount) 并释放
/// @param [in] *p_ctx    上下文句柄; 允许 p_ctx==NULL; 不 close p_blkio
PFS_API void pfs_ctx_destroy(pfs_ctx_t *p_ctx);

/// @brief 向本 ctx 注册外部 FS 扩展表 (如 pfsmtd); mount 探测时优先于内置栈
/// @param [in, out] *p_ctx       上下文句柄
/// @param [in]      fs_type      须与 p_fs_ops->fs_type 一致; 禁止覆盖库内建类型 (FAT/exFAT/NTFS/isofs/udf/ext*)
/// @param [in]      *p_fs_ops    稳定静态/长寿命表; ctx 只保存指针
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_EEXIST / PFS_EPERM(内建类型) / PFS_ERANGE(槽满)
/// @note 挂载时 sbi 优先落在 ctx->fs (sbi_buf); sbi_size 超过 sizeof(fs) 才堆分配
PFS_API int pfs_ctx_register_fs(pfs_ctx_t *p_ctx, pfs_fs_type_e fs_type, const pfs_fs_ops_t *p_fs_ops);

/// @brief 探测并挂载 (先 register_fs, 再 exFAT->NTFS->UDF->ISOFS->ext->FAT)
/// @param [in, out] *p_ctx         上下文句柄
/// @param [in]      *p_blkio        须已 open
/// @param [in]      part_offset     分区相对设备字节偏移
/// @param [in]      mode            PFS_MNT_RDONLY / PFS_MNT_RDWR
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_EBUSY / PFS_EIO / PFS_ENOEXEC / PFS_ENOMEM / PFS_EBADMSG / PFS_EOPNOTSUPP
PFS_API int pfs_ctx_mount(pfs_ctx_t *p_ctx, pfs_blkio_t *p_blkio, int64_t part_offset, int mode);

/// @brief 已挂载时切换只读/读写; 不重新解析 boot
/// @param [in, out] *p_ctx    上下文句柄
/// @param [in]      mode        PFS_MNT_RDONLY / PFS_MNT_RDWR
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_EBUSY
PFS_API int pfs_ctx_remount(pfs_ctx_t *p_ctx, int mode);

/// @brief 卸载; 清除 p_blkio, 置 PFS_NONE
/// @param [in, out] *p_ctx    上下文句柄
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_EBUSY
PFS_API int pfs_ctx_umount(pfs_ctx_t *p_ctx);

/// @brief 当前挂载的文件系统类型
/// @param [in] *p_ctx    上下文句柄
/// @return pfs_fs_type_e p_ctx==NULL 或未挂载返回 PFS_NONE
PFS_API pfs_fs_type_e pfs_ctx_fs_type(const pfs_ctx_t *p_ctx);

/// @brief 当前挂载的文件系统名称
/// @param [in] *p_ctx    上下文句柄
/// @return const char* p_ctx==NULL 或未挂载返回 NULL
PFS_API const char *pfs_ctx_fs_name(const pfs_ctx_t *p_ctx);


////////////////////////////////////////////////////////////
// make fs

/// @def   PFS_FAT12_VOL_BYTES_MAX
/// @brief 卷容量上限 [单位:字节] (按 512B 扇区与现行 mkfs 最大 AU)
/// @note  FAT12/16: MAX_FAT12/16 * 128sec * 512; FAT32: uint32 扇区数 * 512;
///  \n exFAT: EXFAT_MAX_NUM_CLUSTER * 最大簇 (512B 扇区下 32MiB)
#define PFS_FAT12_VOL_BYTES_MAX         ((uint64_t)0xFF4u * 128u * 512u)

/// @def   PFS_FAT16_VOL_BYTES_MAX
/// @brief FAT16 卷容量上限 [单位:字节]
#define PFS_FAT16_VOL_BYTES_MAX         ((uint64_t)0xFFF4u * 128u * 512u)

/// @def   PFS_FAT32_VOL_BYTES_MAX
/// @brief FAT32 卷容量上限 [单位:字节]
#define PFS_FAT32_VOL_BYTES_MAX         ((uint64_t)0xFFFFFFFFu * 512u)

/// @def   PFS_EXFAT_VOL_BYTES_MAX
/// @brief exFAT 卷容量上限 [单位:字节]
#define PFS_EXFAT_VOL_BYTES_MAX         ((uint64_t)0xFFFFFFF5u * (32u * 1024u * 1024u))

/// @def   PFS_NTFS_VOL_BYTES_MAX
/// @brief NTFS 卷容量上限 [单位:字节] (256 TiB soft cap)
#define PFS_NTFS_VOL_BYTES_MAX          ((uint64_t)256ULL << 40)

/// @def   PFS_ISOFS_VOL_BYTES_MAX
/// @brief ISOFS 卷容量上限 [单位:字节] (volume_space_size(u32)*2KiB)
#define PFS_ISOFS_VOL_BYTES_MAX         ((uint64_t)0xFFFFFFFFu * 2048u)

/// @def   PFS_UDF_VOL_BYTES_MAX
/// @brief UDF 卷映像容量上限 [单位:字节] (2KiB 逻辑块 soft cap)
#define PFS_UDF_VOL_BYTES_MAX           ((uint64_t)0xFFFFFFFFu * 2048u)

/// @def   PFS_UDF_PART_BYTES_MAX
/// @brief UDF 分区容量上限 [单位:字节] (PD.partitionLength(u32)*2KiB)
#define PFS_UDF_PART_BYTES_MAX          ((uint64_t)0xFFFFFFFFu * 2048u)

/// @def   PFS_EXT2_VOL_BYTES_MAX
/// @brief ext2 卷容量上限 [单位:字节]
#define PFS_EXT2_VOL_BYTES_MAX          ((uint64_t)1024u * 32768u * 4096u)

/// @def   PFS_EXT3_VOL_BYTES_MAX
/// @brief ext3 卷容量上限 [单位:字节]
#define PFS_EXT3_VOL_BYTES_MAX          PFS_EXT2_VOL_BYTES_MAX

/// @def   PFS_EXT4_VOL_BYTES_MAX
/// @brief ext4 卷容量上限 [单位:字节]
#define PFS_EXT4_VOL_BYTES_MAX          PFS_EXT2_VOL_BYTES_MAX

/// @brief 在已 open 的 p_blkio 上制作文件系统 (不经 ctx; 勿对已挂载卷调用)
/// @param [in] *p_blkio        已 open 的块设备句柄
/// @param [in] part_offset     分区相对设备字节偏移 (裸卷 0); vol_bytes 须为扇区整数倍
/// @param [in] vol_bytes       卷容量 [单位:字节]; 超过对应 PFS_*_VOL_BYTES_MAX 时截断为上限
/// @param [in] fs_type         PFS_FAT12 / PFS_FAT16 / PFS_FAT32 / PFS_EXFAT / PFS_NTFS / PFS_ISOFS(空卷) / PFS_UDF(空卷) / PFS_EXT2 / PFS_EXT3 / PFS_EXT4
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_EIO / PFS_ENOMEM / PFS_EOPNOTSUPP 等
PFS_API int pfs_mkfs(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type);

/// @brief 预制卷选项 (首期仅卷标; 各 FS 映射规则由实现决定)
typedef struct _pfs_mkvol_opts_t {
    const char             *volume_id;                  ///< 卷标 UTF-8; NULL/空串用各栈默认
} pfs_mkvol_opts_t;

/// @brief 按 fs_type 预制带内容的卷 (不经 ctx; 勿对已挂载卷调用)
/// @param [in] *p_blkio        已 open 的块设备句柄
/// @param [in] part_offset     分区相对设备字节偏移 (裸卷 0); vol_bytes 须为扇区整数倍
/// @param [in] vol_bytes       卷容量 [单位:字节]; 超过对应 PFS_*_VOL_BYTES_MAX 时截断为上限
/// @param [in] fs_type         目标文件系统类型
/// @param [in] *src_dir        宿主目录 UTF-8 路径, 须存在
/// @param [in] *opts           NULL 等同默认 (无卷标覆盖)
/// @return int PFS_OK 成功; PFS_EINVAL / PFS_ENOENT / PFS_EIO / PFS_ENOMEM / PFS_ENOSPC / PFS_EOPNOTSUPP 等
/// @note 可写 FS: 等价 mkfs + 将 src_dir 灌入; 只读 FS (如 isofs): 一次打包为映像
PFS_API int pfs_mkvol(pfs_blkio_t *p_blkio, int64_t part_offset, uint64_t vol_bytes, pfs_fs_type_e fs_type, const char *src_dir, const pfs_mkvol_opts_t *opts);


////////////////////////////////////////////////////////////
// file

/// @def   PFS_SEEK_SET
/// @brief fseek whence: SEEK_SET
#define PFS_SEEK_SET                0

/// @def   PFS_SEEK_CUR
/// @brief fseek whence: SEEK_CUR
#define PFS_SEEK_CUR                1

/// @def   PFS_SEEK_END
/// @brief fseek whence: SEEK_END
#define PFS_SEEK_END                2

/// @def   PFS_O_RD
/// @brief fopen mode: 只读(需存在文件); 访问位互斥 RD/RDWR, 可再或上 CREAT; 勿与 mount PFS_MNT_* 混用
#define PFS_O_RD                    0x01

/// @def   PFS_O_RDWR
/// @brief fopen mode: 读写(需存在文件)
#define PFS_O_RDWR                  0x02

/// @def   PFS_O_CREAT
/// @brief fopen mode: 不存在则创建 (须与 PFS_O_RDWR 同用)
#define PFS_O_CREAT                 0x04

/// @brief 不透明 file handle
typedef struct _pfs_file_t pfs_file_t;

/// @brief 打开文件
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @param [in] mode       PFS_O_RD / PFS_O_RDWR; 可选 | PFS_O_CREAT (须 RDWR; 不存在则建空文件)
/// @return pfs_file_t* 成功堆上 handle; 失败 NULL
/// @note RDWR/CREAT 须卷为读写挂载
PFS_API pfs_file_t *pfs_fopen(pfs_ctx_t *p_ctx, const char *path, int mode);

/// @brief 关闭并释放文件句柄
/// @param [in] *p_file    文件句柄
/// @return int PFS_OK 成功; 允许后续对已关闭指针勿再用
PFS_API int pfs_fclose(pfs_file_t *p_file);

/// @brief 读取文件
/// @param [out] *buf       输出缓冲
/// @param [in]  size       每项字节数
/// @param [in]  nmemb      项数
/// @param [in]  *p_file    文件句柄
/// @return int 成功: 已读写的 nmemb 个数 (>=0, 含短读/EOF); 出错: PFS_* 负码 (并置 ferror)
PFS_API int pfs_fread(void *buf, int size, int nmemb, pfs_file_t *p_file);

/// @brief 写入文件
/// @param [in] *buf        输入缓冲
/// @param [in] size        每项字节数
/// @param [in] nmemb       项数
/// @param [in] *p_file     文件句柄
/// @return int 成功: 已读写的 nmemb 个数 (>=0); 出错: PFS_* 负码 (并置 ferror)
PFS_API int pfs_fwrite(const void *buf, int size, int nmemb, pfs_file_t *p_file);

/// @brief 读取单字节
/// @param [in] *p_file    文件句柄
/// @return int 成功: 字节值 0..255; 出错: PFS_* 负码; EOF: -1 且 feof 非 0 (见 ferror/feof)
PFS_API int pfs_fgetc(pfs_file_t *p_file);

/// @brief 写入单字节
/// @param [in] c          字节值
/// @param [in] *p_file    文件句柄
/// @return int 成功: c&0xFF; 出错: PFS_* 负码
PFS_API int pfs_fputc(int c, pfs_file_t *p_file);

/// @brief 定位文件偏移
/// @param [in] *p_file    文件句柄
/// @param [in] offset     偏移量
/// @param [in] whence     PFS_SEEK_SET / CUR / END
/// @return int PFS_OK 成功
PFS_API int pfs_fseek(pfs_file_t *p_file, int64_t offset, int whence);

/// @brief 获取当前文件偏移
/// @param [in] *p_file    文件句柄
/// @return int64_t 当前偏移; 失败返回 -1
PFS_API int64_t pfs_ftell(pfs_file_t *p_file);

/// @brief 获取流错误码
/// @param [in] *p_file    文件句柄
/// @return int 0=无错; <0 为 PFS_*
PFS_API int pfs_ferror(pfs_file_t *p_file);

/// @brief 判断是否已到 EOF
/// @param [in] *p_file    文件句柄
/// @return int 非 0=已到 EOF
PFS_API int pfs_feof(pfs_file_t *p_file);

/// @brief 刷写缓冲
/// @param [in] *p_file    文件句柄
/// @return int PFS_OK 成功
PFS_API int pfs_fflush(pfs_file_t *p_file);

/// @brief 刷写并同步到介质
/// @param [in] *p_file    文件句柄
/// @return int PFS_OK 成功
PFS_API int pfs_fsync(pfs_file_t *p_file);

/// @brief 截断/扩展到指定大小
/// @param [in] *p_file    文件句柄
/// @param [in] size       目标大小 [单位:字节]
/// @return int PFS_OK 成功
PFS_API int pfs_ftruncate(pfs_file_t *p_file, int64_t size);

////////////////////////////////////////////////////////////
// dir

/// @def   PFS_NAME_MAX
/// @brief 目录项名最大 UTF-8 字节数 (不含 '\0'; 自持, 不依赖宿主 NAME_MAX/dirent.h)
#define PFS_NAME_MAX                255

/// @def   PFS_DT_UNKNOWN
/// @brief d_type: 未知 (自持; 语义对齐常见 DT_*; 接通后由 fat/exfat 属性映射)
#define PFS_DT_UNKNOWN              0

/// @def   PFS_DT_REG
/// @brief d_type: 普通文件
#define PFS_DT_REG                  1

/// @def   PFS_DT_DIR
/// @brief d_type: 目录
#define PFS_DT_DIR                  2

/// @brief 目录项
typedef struct _pfs_dirent_t {
    uint8_t                 d_type;                     ///< PFS_DT_*
    char                    d_name[PFS_NAME_MAX + 6];   ///< UTF-8, 含 '\0'
} pfs_dirent_t;

/// @brief 不透明 dir handle
typedef struct _pfs_dir_t pfs_dir_t;

/// @brief 打开目录
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @return pfs_dir_t* 成功返回堆上 handle; 失败 NULL
PFS_API pfs_dir_t *pfs_opendir(pfs_ctx_t *p_ctx, const char *path);

/// @brief 关闭并释放目录句柄
/// @param [in] *p_dir    目录句柄; 允许 p_dir==NULL
PFS_API void pfs_closedir(pfs_dir_t *p_dir);

/// @brief 读下一目录项
/// @param [in] *p_dir    目录句柄
/// @return pfs_dirent_t* 成功返回指向 p_dir 内部 entry (下次 readdir/closedir 前有效)
///  \n EOF: NULL 且 pfs_direrror==0; 出错: NULL 且 pfs_direrror<0
PFS_API pfs_dirent_t *pfs_readdir(pfs_dir_t *p_dir);

/// @brief 获取目录流错误码
/// @param [in] *p_dir    目录句柄
/// @return int 0=无错; <0 为 PFS_* (含 NULL 句柄时 PFS_EINVAL)
PFS_API int pfs_direrror(pfs_dir_t *p_dir);

////////////////////////////////////////////////////////////
// path (路径级操作; 无长寿命句柄; 实现 pfs_path.c)

/// @def   PFS_F_OK
/// @brief access mode: 仅存在性 (可组合; 勿与 mount PFS_MNT_*, fopen PFS_O_* 混用)
#define PFS_F_OK                    0x00

/// @def   PFS_R_OK
/// @brief access mode: 可读
#define PFS_R_OK                    0x01

/// @def   PFS_W_OK
/// @brief access mode: 可写
#define PFS_W_OK                    0x02

/// @def   PFS_X_OK
/// @brief access mode: 可执行 (FAT/exFAT 常忽略)
#define PFS_X_OK                    0x04


/// @brief 检查路径访问
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @param [in] mode       PFS_F_OK 或 R/W/X_OK 组合
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_access(pfs_ctx_t *p_ctx, const char *path, int mode);

/// @brief 创建目录
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @param [in] mode       保留 (FAT/exFAT 权限常忽略)
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_mkdir(pfs_ctx_t *p_ctx, const char *path, int mode);

/// @brief 删除空目录
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_rmdir(pfs_ctx_t *p_ctx, const char *path);

/// @brief 重命名路径
/// @param [in] *p_ctx       已挂载的上下文句柄
/// @param [in] *oldpath      原路径 UTF-8
/// @param [in] *newpath      新路径 UTF-8
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_rename(pfs_ctx_t *p_ctx, const char *oldpath, const char *newpath);

/// @brief 跨 ctx 重命名 (同 ctx 等价 pfs_rename; 异 ctx 仅普通文件 copy+unlink 源)
/// @param [in] *p_old_ctx   源已挂载 ctx
/// @param [in] *oldpath     源路径 UTF-8
/// @param [in] *p_new_ctx   目标已挂载 ctx
/// @param [in] *newpath     目标路径 UTF-8
/// @return int PFS_OK 成功; PFS_EISDIR 源为目录; PFS_EEXIST 目标已存在; 其它 PFS_*
PFS_API int pfs_rename_cross(pfs_ctx_t *p_old_ctx, const char *oldpath, pfs_ctx_t *p_new_ctx, const char *newpath);

/// @brief 删除文件或目录
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_remove(pfs_ctx_t *p_ctx, const char *path);

/// @brief 删除文件 (unlink)
/// @param [in] *p_ctx     已挂载的上下文句柄
/// @param [in] *path      路径 UTF-8
/// @return int PFS_OK 成功; 其它 PFS_* 负码
PFS_API int pfs_unlink(pfs_ctx_t *p_ctx, const char *path);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_H_

// end
