// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    pfs_blkio.h
/// @brief   块设备 I/O facade 对外接口
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef _PFS_BLKIO_H_
#define _PFS_BLKIO_H_

#include "pfs_ops.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// @def   PFS_BLKIO_IO_SIZE_MAX
/// @brief 单次 I/O 最大字节数 (约 2GiB)
/// @note  pread/pwrite: offset 为 int64_t (大容量设备地址); 单次 size 为 int (约 2GiB 上限)
#define PFS_BLKIO_IO_SIZE_MAX  2147483647

/// @def   PFS_BLKIO_RET_OK
/// @brief API 返回值: 成功
/// @note  失败时 <0; pread/pwrite 成功返回传输字节数; sector_size* 成功返回扇区字节数
#define PFS_BLKIO_RET_OK        0

/// @def   PFS_BLKIO_RET_PARAM
/// @brief API 返回值: 参数非法 (NULL, 非法 flags/range/对齐/status 等)
#define PFS_BLKIO_RET_PARAM    -1

/// @def   PFS_BLKIO_RET_IO
/// @brief API 返回值: 系统 I/O 失败 (open/pread/pwrite/fsync/close; 见 last_error)
#define PFS_BLKIO_RET_IO       -2

/// @def   PFS_BLKIO_RET_STATE
/// @brief API 返回值: 状态/生命周期不满足 (未 open, 已 latch err, 重复 open)
#define PFS_BLKIO_RET_STATE    -3

/// @def   PFS_BLKIO_RET_PROBE
/// @brief API 返回值: 扇区探测失败 (路径/非块设备/ioctl/非常规扇区)
#define PFS_BLKIO_RET_PROBE    -4

/// @def   PFS_BLKIO_ALIGN_512
/// @brief O_DIRECT / NO_BUFFERING 对齐粒度 512 (open_direct 的 align_size 须为下列之一)
#define PFS_BLKIO_ALIGN_512    512

/// @def   PFS_BLKIO_ALIGN_1K
/// @brief O_DIRECT / NO_BUFFERING 对齐粒度 1024
#define PFS_BLKIO_ALIGN_1K     1024

/// @def   PFS_BLKIO_ALIGN_2K
/// @brief O_DIRECT / NO_BUFFERING 对齐粒度 2048
#define PFS_BLKIO_ALIGN_2K     2048

/// @def   PFS_BLKIO_ALIGN_4K
/// @brief O_DIRECT / NO_BUFFERING 对齐粒度 4096
#define PFS_BLKIO_ALIGN_4K     4096

/// @def   PFS_BLKIO_IMAGE_FIXED
/// @brief create_image flags: 预分配宿主 (VHD/VHDX Fixed)
#define PFS_BLKIO_IMAGE_FIXED      0x0001u

/// @def   PFS_BLKIO_IMAGE_DYNAMIC
/// @brief create_image flags: 瘦文件/稀疏 (VHD/VHDX Dynamic; qcow2 默认)
#define PFS_BLKIO_IMAGE_DYNAMIC    0x0002u

/// @def   PFS_BLKIO_IMAGE_FORCE
/// @brief create_image flags: 允许覆盖已存在 path (否则 facade 拒绝)
#define PFS_BLKIO_IMAGE_FORCE      0x0004u


/// @brief 状态位标志 (可组合); open 成功后 status==0 表示已打开且无生命周期/错误位
typedef enum _pfs_blkio_status_e {
    PFS_BLKIO_NOT_OPEN  = 0x0001,           ///< init 后, 首次 open 前
    PFS_BLKIO_CLOSED    = 0x0002,           ///< close 后

    PFS_BLKIO_ERR       = 0x0010,           ///< 未知错误
    PFS_BLKIO_ERR_READ  = 0x0020,           ///< 读错误
    PFS_BLKIO_ERR_WRITE = 0x0040,           ///< 写错误
    PFS_BLKIO_ERR_FSYNC = 0x0080,           ///< fsync 错误
} pfs_blkio_status_e;

/// @def   PFS_BLKIO_OPS_MAX
/// @brief 本句柄可注册的外部 blkio ops 表上限 (指针槽; 不拷贝表)
#define PFS_BLKIO_OPS_MAX  16

/// @brief 块设备句柄 (可扩展: ops 分派 + 后端 priv)
/// @note 非线程安全: 同一 pfs_blkio_t 须串行访问 (跨线程共享时由调用方加锁)
typedef struct _pfs_blkio_t {
    const pfs_blkio_ops_t *ops;     ///< 指向静态后端表; 未 open 为 NULL
    void                  *priv;    ///< 后端私有态; open 分配, close 释放

    const pfs_blkio_ops_t *blk_ops[PFS_BLKIO_OPS_MAX];  ///< 本句柄注册的外部 ops 表
    int                   blk_ops_count;                ///< 已注册数量

    int     status;      ///< pfs_blkio_status_e 位; 0=open 正常; 已 latch ERR_* 时阻塞 I/O
    int     last_error;  ///< Win: GetLastError(); Linux: errno; 无错误时为 0
    int     align_size;  ///< 0=缓冲模式 (open); open_direct 后为 512/1024/2048/4096
} pfs_blkio_t;

/// @brief 重置句柄为 NOT_OPEN
/// @param [in, out] *p_blkio    块设备句柄
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM 失败
/// @note 仅在未 open 时调用: 首次使用前, deinit 后; close 后可不 init 直接再次 open
///  \n 仍 open 时勿调用 (会泄漏 fd/HANDLE); 须先 close 或 deinit
///  \n 会清空本句柄已 register 的 blk_ops
PFS_API int pfs_blkio_init(pfs_blkio_t *p_blkio);

/// @brief 若仍 open 则 close; 不重置 status (通常不再复用该 struct); 不清空 blk_ops
/// @param [in, out] *p_blkio    块设备句柄
PFS_API void pfs_blkio_deinit(pfs_blkio_t *p_blkio);

/// @brief 向本句柄注册外部 blkio 扩展表 (如 pfsmtd); open 探测时优先于内建镜像序
/// @param [in, out] *p_blkio    块设备句柄
/// @param [in]      type        须与 ops->type 一致, 且 >= PFS_BLKIO_MTD_MIN
/// @param [in]      *ops        稳定静态/长寿命表; 句柄只保存指针
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM (非法/内建/重复/槽满)
/// @note 禁止覆盖库内建 RAW/VHD/...; 无 unregister (init 可清空)
PFS_API int pfs_blkio_register(pfs_blkio_t *p_blkio, pfs_blkio_type_e type, const pfs_blkio_ops_t *ops);

/// @brief 打开块设备 (自动探测镜像类型)
/// @param [in, out] *p_blkio      块设备句柄
/// @param [in]      *p_devpath     Windows 为 "\\\\.\\PhysicalDrive1"; Linux 为 "/dev/sda"
/// @param [in]      flags          PFS_BLKIO_O_RDONLY / PFS_BLKIO_O_WRONLY / PFS_BLKIO_O_RDWR (三选一)
/// @param [in]      sector_size    逻辑扇区 512/1024/2048/4096; Win 缓冲 open 分配 head/tail 扇区缓存; Linux 校验后忽略
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / PFS_BLKIO_RET_IO
/// @note Win 缓冲: WRONLY 仍会带读权限 (未对齐 pwrite 的 RMW 预读); 对外仍为只写语义
///  \n 探测序: 先本句柄 register (注册序), 再 vhdx -> vhd -> qcow2 -> vmdk -> vdi -> raw; 仅 PROBE 回退
PFS_API int pfs_blkio_open(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size);

/// @brief 强制指定 type 打开; 不探测回退其它类型
/// @param [in, out] *p_blkio      块设备句柄
/// @param [in]      *p_devpath     设备路径
/// @param [in]      flags          访问模式
/// @param [in]      sector_size    逻辑扇区大小
/// @param [in]      type           内建 RAW/VHD/VHDX/QCOW2/VMDK/VDI, 或本句柄已 register 的 MTD/USER
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / PFS_BLKIO_RET_IO / PFS_BLKIO_RET_PROBE
/// @note 未 register 的扩展类型请用 open_ops; 格式不符由该后端返回 PFS_BLKIO_RET_PROBE
PFS_API int pfs_blkio_open_ex(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size, pfs_blkio_type_e type);

/// @brief 用外部 ops 打开; 不走内建镜像探测
/// @param [in, out] *p_blkio      块设备句柄
/// @param [in]      *p_devpath     可 NULL (由 ops->open 约定)
/// @param [in]      flags          访问模式; 原样下传
/// @param [in]      sector_size    逻辑扇区大小; 原样下传
/// @param [in]      *ops           静态/长寿命表; type 须 >= PFS_BLKIO_MTD_MIN; 禁止冒充内建 RAW/VHD/...
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / 后端返回码
PFS_API int pfs_blkio_open_ops(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int sector_size, const pfs_blkio_ops_t *ops);

/// @brief 介质已由外部持有: 仅绑 ops+priv, 置已 open; 不调 ops->open
/// @param [in, out] *p_blkio    块设备句柄
/// @param [in]      *ops        静态/长寿命; type 须 >= PFS_BLKIO_MTD_MIN
/// @param [in]      *priv       非 NULL (is_open 要求)
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE
/// @note close 仍走 ops->close (须释放 priv); align_size=0 (缓冲语义)
PFS_API int pfs_blkio_open_attach(pfs_blkio_t *p_blkio, const pfs_blkio_ops_t *ops, void *priv);

/// @brief Linux O_DIRECT / Win FILE_FLAG_NO_BUFFERING 打开
/// @param [in, out] *p_blkio      块设备句柄
/// @param [in]      *p_devpath     设备路径
/// @param [in]      flags          访问模式
/// @param [in]      align_size     PFS_BLKIO_ALIGN_512 / _1K / _2K / _4K
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / PFS_BLKIO_RET_IO
/// @note open 成功后 pread/pwrite 要求 offset/size/buf 地址均为 align_size 整数倍; 未对齐为 PFS_BLKIO_RET_PARAM 且不 latch
PFS_API int pfs_blkio_open_direct(pfs_blkio_t *p_blkio, const char *p_devpath, pfs_blkio_oflags_e flags, int align_size);

/// @brief 句柄是否 open
/// @param [in] *p_blkio    块设备句柄
/// @return bool 已 open 时为 true
/// @note 忽略已 latch 的 ERR_*; 实际可否 I/O 以 pread/pwrite 前置检查为准
PFS_API bool pfs_blkio_is_open(pfs_blkio_t *p_blkio);

/// @brief 获取 open_direct 对齐粒度
/// @param [in] *p_blkio    块设备句柄
/// @return int 缓冲模式或未 open 时返回 0; 否则返回 open_direct 时传入的 align_size (512/1024/2048/4096)
PFS_API int pfs_blkio_align_size(const pfs_blkio_t *p_blkio);

/// @brief 从块设备读取
/// @param [in, out] *p_blkio    块设备句柄
/// @param [out]     *buf        输出缓冲
/// @param [in]      size        本次字节数, 0 或 1..PFS_BLKIO_IO_SIZE_MAX; size==0 不做 I/O 直接返回 0
/// @param [in]      offset      设备字节偏移 (int64_t, >=0)
/// @return int 成功: 返回 size (完整传输); PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / PFS_BLKIO_RET_IO
/// @note open_direct: offset/size/buf 须 align_size 对齐; 否则 PFS_BLKIO_RET_PARAM
///  \n Win 缓冲 open: 允许未对齐 offset/size; 内部按逻辑扇区整齐读写 (head/tail RMW)
///  \n PFS_BLKIO_RET_IO: latch READ err; buf 可能已部分写入; 须 set_status 或 reopen
PFS_API int pfs_blkio_pread(pfs_blkio_t *p_blkio, void *buf, int size, int64_t offset);

/// @brief 向块设备写入
/// @param [in, out] *p_blkio    块设备句柄
/// @param [in]      *buf        输入缓冲
/// @param [in]      size        本次字节数, 0 或 1..PFS_BLKIO_IO_SIZE_MAX; size==0 不做 I/O 直接返回 0
/// @param [in]      offset      设备字节偏移 (int64_t, >=0)
/// @return int 成功: 返回 size (完整传输); PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE / PFS_BLKIO_RET_IO
/// @note open_direct: offset/size/buf 须 align_size 对齐; 否则 PFS_BLKIO_RET_PARAM
///  \n Win 缓冲 open: 允许未对齐 offset/size; 内部按逻辑扇区整齐读写 (head/tail RMW)
///  \n PFS_BLKIO_RET_IO: latch WRITE err; 部分字节可能已落盘; 须 set_status 或 reopen
PFS_API int pfs_blkio_pwrite(pfs_blkio_t *p_blkio, const void *buf, int size, int64_t offset);

/// @brief 同步块设备句柄
/// @param [in, out] *p_blkio    块设备句柄
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_STATE 已 latch err/未 open; PFS_BLKIO_RET_IO 系统失败
PFS_API int pfs_blkio_fsync(pfs_blkio_t *p_blkio);

/// @brief 获取逻辑盘容量
/// @param [in] *p_blkio    块设备句柄
/// @return int64_t [单位:字节] 镜像为虚拟盘大小; 失败 <0 (PFS_BLKIO_RET_*)
PFS_API int64_t pfs_blkio_get_size(const pfs_blkio_t *p_blkio);

/// @brief 获取逻辑扇区字节数
/// @param [in] *p_blkio    块设备句柄
/// @return int 512/1024/2048/4096; 失败 <0
PFS_API int pfs_blkio_logical_sector_size(const pfs_blkio_t *p_blkio);

/// @brief 返回已 latch 的 status
/// @param [in, out] *p_blkio    块设备句柄
/// @return int 当前 status 位
/// @note I/O 错误会阻塞 pread/pwrite/fsync, 直至 set_status 或 reopen
PFS_API int pfs_blkio_status(pfs_blkio_t *p_blkio);

/// @brief 设置 status (可清除已 latch 的 I/O 错误)
/// @param [in, out] *p_blkio    块设备句柄
/// @param [in]      status      新 status; 无 ERR_* 时同步清零 last_error
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_PARAM 非法组合
/// @note 已 open 时禁止置 NOT_OPEN/CLOSED; 未 open 时禁止 status==0 或无 NOT_OPEN/CLOSED
PFS_API int pfs_blkio_set_status(pfs_blkio_t *p_blkio, int status);

/// @brief 获取最近一次系统错误码
/// @param [in] *p_blkio    块设备句柄
/// @return int Win 为 Win32 码, Linux 为 errno; 无错误时为 0
/// @note PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_STATE 时不改 last_error
PFS_API int pfs_blkio_last_error(pfs_blkio_t *p_blkio);

/// @brief 关闭块设备句柄
/// @param [in, out] *p_blkio    块设备句柄
/// @return int PFS_BLKIO_RET_OK 成功; PFS_BLKIO_RET_IO close 系统失败
/// @note 同一 p_blkio close 后可再次 open
///  \n PFS_BLKIO_RET_IO: 保留句柄供重试; latch ERR, 不置 CLOSED
PFS_API int pfs_blkio_close(pfs_blkio_t *p_blkio);

/// @brief 创建空虚拟容器镜像文件; 不经 open, facade 按 type 直调内建 create
/// @param [in] type         PFS_BLKIO_VHD / VHDX / QCOW2 等内建类型
/// @param [in] path         输出路径
/// @param [in] size_bytes   虚拟盘容量 [字节]; 须 512 对齐
/// @param [in] flags        PFS_BLKIO_IMAGE_* ; VHD/VHDX 缺省 layout 补 DYNAMIC
/// @param [in] param        预留扩展; 现行忽略, 须 NULL
/// @param [in] param_size   预留扩展; 现行须 0
/// @return int PFS_BLKIO_RET_OK; PARAM (含未实现/已存在); IO
PFS_API int pfs_blkio_create_image(pfs_blkio_type_e type, const char *path, uint64_t size_bytes, uint32_t flags, void *param, int param_size);

/// @brief 探测逻辑扇区大小
/// @param [in] *p_devpath    设备路径
/// @return int 成功返回扇区字节数 (512/1024/2048/4096); PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_PROBE
/// @note open_direct 的 align_size 应优先用此值
PFS_API int pfs_blkio_sector_size(const char *p_devpath);

/// @brief 探测物理扇区大小
/// @param [in] *p_devpath    设备路径
/// @return int 成功返回扇区字节数 (512/1024/2048/4096); PFS_BLKIO_RET_PARAM / PFS_BLKIO_RET_PROBE
/// @note 写对齐/性能优化用, 勿作为 open_direct 默认 align_size
PFS_API int pfs_blkio_sector_size_physical(const char *p_devpath);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PFS_BLKIO_H_

// end
