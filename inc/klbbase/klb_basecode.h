///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_basecode.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   基础模块 消息码 (klbbase; 与 net/HTTP/WS 码空间分离)
/// @version 0.3
/// @history 修改历史
///  \n [2026-11] 创建文件
///  \n [2026-11] PRIVATE_BASE 调整为 [8000,9999]
///  \n [2026-11] PRIVATE 迁至 [0x10000, 0x10000+1999]
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_BASECODE_H__
#define __KLB_BASECODE_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def KLB_BASECODE_PRIVATE_BASE
/// @brief klb 私有 basecode 基址
/// @note 新私有码 = KLB_BASECODE_PRIVATE_BASE + n; 上限见 KLB_BASECODE_PRIVATE_MAX
///   \n 私有区间 [0x10000, 0x10000+1999]; 与 net [0x10000+2000, 0x10000+2000+1999] 分区间
///   \n 分块(+相对基址): 通用 0x00 / string 0x20 / package 0x40 / mnp-smp 0x60 / codec-crypto 0x80
#define KLB_BASECODE_PRIVATE_BASE       0x10000

/// @def KLB_BASECODE_PRIVATE_MAX
/// @brief klb 私有 basecode 上限 (含)
/// @note 有效私有区间: [KLB_BASECODE_PRIVATE_BASE, KLB_BASECODE_PRIVATE_MAX]
#define KLB_BASECODE_PRIVATE_MAX        (0x10000 + 1999)


/// @enum  klb_basecode_e
/// @brief 基础模块 消息码 (string/package/mnp/smp/codec 等; 非 net 传输码)
/// @note 负段数值 = -errno, 对齐 linux asm-generic/errno*.h; klb_netcode/pfs 负段取本枚举同名常量
///   \n Win 等非 Linux 数值须由实现映射到本枚举, 勿直接返回宿主原值
///   \n 结果码约定(默认): 0 成功; <0 失败/重试; >=PRIVATE_BASE 细分失败
typedef enum klb_basecode_e_
{
    //////////////////////////////////////////////////////////////////////////
    // 负段: -errno (klb_netcode/pfs 取同名常量; 非完整表; 含 FS 常用码)
    KLB_BASECODE_EPERM          = -1,                       ///< 泛化失败 (EPERM)
    KLB_BASECODE_ENOENT         = -2,                       ///< 路径/对象不存在 (ENOENT)
    KLB_BASECODE_EINTR          = -4,                       ///< 被信号中断 (EINTR)
    KLB_BASECODE_EIO            = -5,                       ///< I/O 失败 (EIO)
    KLB_BASECODE_ENOEXEC        = -8,                       ///< 非支持的可执行/文件系统 (ENOEXEC)
    KLB_BASECODE_EBADF          = -9,                       ///< 坏的 fd (EBADF)
    KLB_BASECODE_EAGAIN         = -11,                      ///< 暂不可用/需重试 (EAGAIN)
    KLB_BASECODE_ENOMEM         = -12,                      ///< 资源不足 (ENOMEM)
    KLB_BASECODE_EACCES         = -13,                      ///< 权限不足 (EACCES)
    KLB_BASECODE_EBUSY          = -16,                      ///< 忙/不可用 (EBUSY)
    KLB_BASECODE_EEXIST         = -17,                      ///< 已存在/冲突 (EEXIST)
    KLB_BASECODE_ENOTDIR        = -20,                      ///< 路径组件非目录 (ENOTDIR)
    KLB_BASECODE_EISDIR         = -21,                      ///< 目标是目录 (EISDIR)
    KLB_BASECODE_EINVAL         = -22,                      ///< 参数非法 (EINVAL)
    KLB_BASECODE_ENOSPC         = -28,                      ///< 空间/容量不足 (ENOSPC)
    KLB_BASECODE_EROFS          = -30,                      ///< 只读文件系统 (EROFS)
    KLB_BASECODE_EPIPE          = -32,                      ///< 管道破裂/写对端已关 (EPIPE)
    KLB_BASECODE_ERANGE         = -34,                      ///< 结果越界 (ERANGE)
    KLB_BASECODE_ENAMETOOLONG   = -36,                      ///< 文件名超长 (ENAMETOOLONG)
    KLB_BASECODE_ENOTEMPTY      = -39,                      ///< 目录非空 (ENOTEMPTY)
    KLB_BASECODE_EBADMSG        = -74,                      ///< 坏消息/元数据损坏 (EBADMSG)
    KLB_BASECODE_EILSEQ         = -84,                      ///< 非法字节序列 (EILSEQ)
    KLB_BASECODE_EMSGSIZE       = -90,                      ///< 报文过大 (EMSGSIZE)
    KLB_BASECODE_EOPNOTSUPP     = -95,                      ///< 不支持/未实现 (EOPNOTSUPP)
    KLB_BASECODE_EAFNOSUPPORT   = -97,                      ///< 地址族不支持 (EAFNOSUPPORT)
    KLB_BASECODE_EADDRINUSE     = -98,                      ///< 地址已占用 (EADDRINUSE)
    KLB_BASECODE_EADDRNOTAVAIL  = -99,                      ///< 地址不可用 (EADDRNOTAVAIL)
    KLB_BASECODE_ENETUNREACH    = -101,                     ///< 网络不可达 (ENETUNREACH)
    KLB_BASECODE_ECONNABORTED   = -103,                     ///< 连接中止 (ECONNABORTED)
    KLB_BASECODE_ECONNRESET     = -104,                     ///< 连接复位 (ECONNRESET)
    KLB_BASECODE_ENOBUFS        = -105,                     ///< 无缓冲空间 (ENOBUFS)
    KLB_BASECODE_EISCONN        = -106,                     ///< 已连接 (EISCONN)
    KLB_BASECODE_ENOTCONN       = -107,                     ///< 未连接 (ENOTCONN)
    KLB_BASECODE_ETIMEDOUT      = -110,                     ///< 操作超时 (ETIMEDOUT)
    KLB_BASECODE_ECONNREFUSED   = -111,                     ///< 连接被拒绝 (ECONNREFUSED)
    KLB_BASECODE_EHOSTUNREACH   = -113,                     ///< 主机不可达 (EHOSTUNREACH)
    KLB_BASECODE_EALREADY       = -114,                     ///< 操作已在进行 (EALREADY)
    KLB_BASECODE_EINPROGRESS    = -115,                     ///< 操作进行中 (EINPROGRESS)
    KLB_BASECODE_ECANCELED      = -125,                     ///< 操作被取消 (ECANCELED)

    //////////////////////////////////////////////////////////////////////////
    // 0: 成功
    KLB_BASECODE_OK             = 0,                        ///< 正常

    //////////////////////////////////////////////////////////////////////////
    // A 通用 [BASE+0x00, BASE+0x1F]

    KLB_BASECODE_STATE          = KLB_BASECODE_PRIVATE_BASE + 0x00, ///< 状态非法
    KLB_BASECODE_RANGE          = KLB_BASECODE_PRIVATE_BASE + 0x01, ///< 区间/范围非法
    KLB_BASECODE_OVERFLOW       = KLB_BASECODE_PRIVATE_BASE + 0x02, ///< 超限/溢出
    KLB_BASECODE_CANCELED       = KLB_BASECODE_PRIVATE_BASE + 0x03, ///< 本地取消/中止操作
    KLB_BASECODE_LIMIT          = KLB_BASECODE_PRIVATE_BASE + 0x04, ///< 限额/配额
    KLB_BASECODE_NOTIMPL        = KLB_BASECODE_PRIVATE_BASE + 0x05, ///< 未实现

    //////////////////////////////////////////////////////////////////////////
    // B string [BASE+0x20, BASE+0x3F]

    KLB_BASECODE_STRING_TRUNC   = KLB_BASECODE_PRIVATE_BASE + 0x20, ///< 字符串截断
    KLB_BASECODE_STRING_EMPTY   = KLB_BASECODE_PRIVATE_BASE + 0x21, ///< 空串/缺内容

    //////////////////////////////////////////////////////////////////////////
    // C package [BASE+0x40, BASE+0x5F]

    KLB_BASECODE_PACKAGE_BAD    = KLB_BASECODE_PRIVATE_BASE + 0x40, ///< 包格式/键非法
    KLB_BASECODE_PACKAGE_NOENT  = KLB_BASECODE_PRIVATE_BASE + 0x41, ///< 包内条目不存在
    KLB_BASECODE_PACKAGE_IO     = KLB_BASECODE_PRIVATE_BASE + 0x42, ///< 包读写失败

    //////////////////////////////////////////////////////////////////////////
    // D mnp/smp [BASE+0x60, BASE+0x7F]

    KLB_BASECODE_MNP_PROTO      = KLB_BASECODE_PRIVATE_BASE + 0x60, ///< MNP/SMP 协议错误
    KLB_BASECODE_MNP_STATE      = KLB_BASECODE_PRIVATE_BASE + 0x61, ///< MNP/SMP 状态非法
    KLB_BASECODE_MNP_PEER       = KLB_BASECODE_PRIVATE_BASE + 0x62, ///< MNP/SMP 对端错误

    //////////////////////////////////////////////////////////////////////////
    // E codec/crypto [BASE+0x80, BASE+0x9F]

    KLB_BASECODE_CODEC_PROTO    = KLB_BASECODE_PRIVATE_BASE + 0x80, ///< 编解码协议/布局错误
    KLB_BASECODE_CODEC_NOSUP    = KLB_BASECODE_PRIVATE_BASE + 0x81, ///< 编解码不支持
    KLB_BASECODE_CRYPTO_FAIL    = KLB_BASECODE_PRIVATE_BASE + 0x82, ///< 加解密失败

    //////////////////////////////////////////////////////////////////////////
    // F 保留 [BASE+0xA0, PRIVATE_MAX]

    KLB_BASECODE_PRIVATE_END    = KLB_BASECODE_PRIVATE_MAX, ///< 私有段上限 (同 KLB_BASECODE_PRIVATE_MAX)
}klb_basecode_e;


/// @brief 将 klb_basecode_e 转为只读描述串
/// @param [in]  code               消息码
/// @return const char*             静态串; 带 "[klbbase] " 前缀; 未知为 "Unknown error"
/// @note 对齐 strerror; 覆盖已定义负段与私有段
KLB_API const char* klb_basecode_strerror(int code);


/// @brief 将宿主 errno 映射为 klb_basecode_e
/// @param [in]  syserr             宿主错误号 (Linux errno 或 Win 等价)
/// @return int                     klb_basecode_e; 未知为 KLB_BASECODE_EIO; 0 为 KLB_BASECODE_EPERM
KLB_API int klb_basecode_from_syserr(int syserr);


/// @brief 读取并映射最近一次宿主系统错误
/// @return int                     klb_basecode_e (同 klb_basecode_from_syserr)
/// @note 读 errno (Win 亦可映射 GetLastError 子集)
KLB_API int klb_basecode_last_syserr(void);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_BASECODE_H__

// end
