///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netcode.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   网络模块 消息码 (传输层; 含 HTTP status / WS close 别名)
/// @version 0.7
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETCODE_H__
#define __KLB_NETCODE_H__

#include "klb_type.h"
#include "klbbase/klb_basecode.h"
#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def KLB_NETCODE_PRIVATE_BASE
/// @brief klb 私有 netcode 基址
/// @note 新私有码 = KLB_NETCODE_PRIVATE_BASE + n; 上限见 KLB_NETCODE_PRIVATE_MAX
///   \n 段序按数值: 负段 / socket[0,64) / HTTP[100,599] / WS[1000,4999] / 私有[0x10000+2000, +3999]
///   \n 协议别名段(禁作私有码占用): HTTP status; WS close
///   \n [0, KLB_SOCKET_STATUS_MAX) 沿用 klb_socket_status_e
///   \n 私有分块(+相对基址): 流控 0 / 连接 32 / TLS 64 / listen 96 / 协议 128 / 会话 160
#define KLB_NETCODE_PRIVATE_BASE        (0x10000 + 2000)

/// @def KLB_NETCODE_PRIVATE_MAX
/// @brief klb 私有 netcode 上限 (含)
/// @note 有效私有区间: [KLB_NETCODE_PRIVATE_BASE, KLB_NETCODE_PRIVATE_MAX]
#define KLB_NETCODE_PRIVATE_MAX         (0x10000 + 2000 + 1999)


/// @enum  klb_netcode_e
/// @brief 网络模块 消息码 (传输/连接/流控; 另含 HTTP status / WS close 别名)
/// @note 负段 = KLB_BASECODE_* 同名常量 (数值 -errno); 非完整表
///   \n WinSock 等非 Linux 数值须由实现映射到本枚举, 勿直接返回 WSA* 原值
///   \n 结果码约定(默认): 0 成功; <0 失败/重试; >=PRIVATE_BASE 细分失败
///   \n 例外: send/recv 等返回字节数的 API 另见各自 @return
typedef enum klb_netcode_e_
{
    //////////////////////////////////////////////////////////////////////////
    // 负段: 取自 KLB_BASECODE_* (同名同值)
    KLB_NETCODE_EPERM           = KLB_BASECODE_EPERM,       ///< 泛化失败 (EPERM)
    KLB_NETCODE_ENOENT          = KLB_BASECODE_ENOENT,      ///< 路径/对象不存在 (ENOENT)
    KLB_NETCODE_EINTR           = KLB_BASECODE_EINTR,       ///< 被信号中断 (EINTR)
    KLB_NETCODE_EIO             = KLB_BASECODE_EIO,         ///< I/O 失败 (EIO)
    KLB_NETCODE_EBADF           = KLB_BASECODE_EBADF,       ///< 坏的 fd (EBADF)
    KLB_NETCODE_EAGAIN          = KLB_BASECODE_EAGAIN,      ///< 暂不可用/需重试 (EAGAIN)
    KLB_NETCODE_ENOMEM          = KLB_BASECODE_ENOMEM,      ///< 资源不足 (ENOMEM)
    KLB_NETCODE_EACCES          = KLB_BASECODE_EACCES,      ///< 权限不足 (EACCES)
    KLB_NETCODE_EBUSY           = KLB_BASECODE_EBUSY,       ///< 忙/不可用 (EBUSY)
    KLB_NETCODE_EEXIST          = KLB_BASECODE_EEXIST,      ///< 已存在/冲突 (EEXIST)
    KLB_NETCODE_EINVAL          = KLB_BASECODE_EINVAL,      ///< 参数非法 (EINVAL)
    KLB_NETCODE_EPIPE           = KLB_BASECODE_EPIPE,       ///< 管道破裂/写对端已关 (EPIPE)
    KLB_NETCODE_EMSGSIZE        = KLB_BASECODE_EMSGSIZE,    ///< 报文过大 (EMSGSIZE)
    KLB_NETCODE_EOPNOTSUPP      = KLB_BASECODE_EOPNOTSUPP,  ///< 不支持/未实现 (EOPNOTSUPP)
    KLB_NETCODE_EAFNOSUPPORT    = KLB_BASECODE_EAFNOSUPPORT, ///< 地址族不支持 (EAFNOSUPPORT)
    KLB_NETCODE_EADDRINUSE      = KLB_BASECODE_EADDRINUSE,  ///< 地址已占用 (EADDRINUSE)
    KLB_NETCODE_EADDRNOTAVAIL   = KLB_BASECODE_EADDRNOTAVAIL, ///< 地址不可用 (EADDRNOTAVAIL)
    KLB_NETCODE_ENETUNREACH     = KLB_BASECODE_ENETUNREACH, ///< 网络不可达 (ENETUNREACH)
    KLB_NETCODE_ECONNABORTED    = KLB_BASECODE_ECONNABORTED, ///< 连接中止 (ECONNABORTED)
    KLB_NETCODE_ECONNRESET      = KLB_BASECODE_ECONNRESET,  ///< 连接复位 (ECONNRESET)
    KLB_NETCODE_ENOBUFS         = KLB_BASECODE_ENOBUFS,     ///< 无缓冲空间 (ENOBUFS)
    KLB_NETCODE_EISCONN         = KLB_BASECODE_EISCONN,     ///< 已连接 (EISCONN)
    KLB_NETCODE_ENOTCONN        = KLB_BASECODE_ENOTCONN,    ///< 未连接 (ENOTCONN)
    KLB_NETCODE_ETIMEDOUT       = KLB_BASECODE_ETIMEDOUT,   ///< 操作超时 (ETIMEDOUT)
    KLB_NETCODE_ECONNREFUSED    = KLB_BASECODE_ECONNREFUSED, ///< 连接被拒绝 (ECONNREFUSED)
    KLB_NETCODE_EHOSTUNREACH    = KLB_BASECODE_EHOSTUNREACH, ///< 主机不可达 (EHOSTUNREACH)
    KLB_NETCODE_EALREADY        = KLB_BASECODE_EALREADY,    ///< 操作已在进行 (EALREADY)
    KLB_NETCODE_EINPROGRESS     = KLB_BASECODE_EINPROGRESS, ///< 操作进行中 (EINPROGRESS)
    KLB_NETCODE_ECANCELED       = KLB_BASECODE_ECANCELED,   ///< 操作被取消 (ECANCELED)

    //////////////////////////////////////////////////////////////////////////
    // [0 ~ KLB_SOCKET_STATUS_MAX), 沿用 klb_socket_status_e (别名, 数值同源)
    KLB_NETCODE_OK              = KLB_SOCKET_OK,            ///< 正常
    KLB_NETCODE_DISCONNECT      = KLB_SOCKET_DISCONNECT,    ///< 连接断开(读写socket错误引发)
    KLB_NETCODE_TIMEOUT         = KLB_SOCKET_TIMEOUT,       ///< 超时
    KLB_NETCODE_CLOSED          = KLB_SOCKET_CLOSED,        ///< 连接已关闭
    KLB_NETCODE_ERR_PROTOCOL    = KLB_SOCKET_ERR_PROTOCOL,  ///< 协议错误
    KLB_NETCODE_ERR_RESET       = KLB_SOCKET_ERR_RESET,     ///< 对端复位 (RST 等)
    KLB_NETCODE_ERR_REFUSED     = KLB_SOCKET_ERR_REFUSED,   ///< 连接被拒绝
    KLB_NETCODE_ERR_UNREACH     = KLB_SOCKET_ERR_UNREACH,   ///< 网络/主机不可达
    KLB_NETCODE_ERR_ABORT       = KLB_SOCKET_ERR_ABORT,     ///< 连接中止 (ECONNABORTED 等)
    KLB_NETCODE_ERR_PIPE        = KLB_SOCKET_ERR_PIPE,      ///< 写时对端已关 (EPIPE 等)
    KLB_NETCODE_ERR_INVAL       = KLB_SOCKET_ERR_INVAL,     ///< 参数或状态非法
    KLB_NETCODE_ERR_ACCESS      = KLB_SOCKET_ERR_ACCESS,    ///< 权限/访问被拒
    KLB_NETCODE_ERR_MAX         = KLB_SOCKET_ERR_MAX,       ///< 错误最大值 (含以上 ERR_*)
    KLB_NETCODE_CLOSEING        = KLB_SOCKET_CLOSEING,      ///< 主动关闭
    KLB_NETCODE_CONNECT         = KLB_SOCKET_CONNECT,       ///< 连接成功
    KLB_NETCODE_SOCKET_MAX      = KLB_SOCKET_STATUS_MAX,    ///< socket 段上限: 2^6

    //////////////////////////////////////////////////////////////////////////
    // H HTTP status [100, 599] (协议状态码别名; 与 http_parser HTTP_STATUS_* 同值)

    KLB_NETCODE_HTTP_CONTINUE                       = 100,  ///< Continue
    KLB_NETCODE_HTTP_SWITCHING_PROTOCOLS            = 101,  ///< Switching Protocols
    KLB_NETCODE_HTTP_PROCESSING                     = 102,  ///< Processing
    KLB_NETCODE_HTTP_OK                             = 200,  ///< OK
    KLB_NETCODE_HTTP_CREATED                        = 201,  ///< Created
    KLB_NETCODE_HTTP_ACCEPTED                       = 202,  ///< Accepted
    KLB_NETCODE_HTTP_NON_AUTHORITATIVE_INFORMATION  = 203,  ///< Non-Authoritative Information
    KLB_NETCODE_HTTP_NO_CONTENT                     = 204,  ///< No Content
    KLB_NETCODE_HTTP_RESET_CONTENT                  = 205,  ///< Reset Content
    KLB_NETCODE_HTTP_PARTIAL_CONTENT                = 206,  ///< Partial Content
    KLB_NETCODE_HTTP_MULTI_STATUS                   = 207,  ///< Multi-Status
    KLB_NETCODE_HTTP_ALREADY_REPORTED               = 208,  ///< Already Reported
    KLB_NETCODE_HTTP_IM_USED                        = 226,  ///< IM Used
    KLB_NETCODE_HTTP_MULTIPLE_CHOICES               = 300,  ///< Multiple Choices
    KLB_NETCODE_HTTP_MOVED_PERMANENTLY              = 301,  ///< Moved Permanently
    KLB_NETCODE_HTTP_FOUND                          = 302,  ///< Found
    KLB_NETCODE_HTTP_SEE_OTHER                      = 303,  ///< See Other
    KLB_NETCODE_HTTP_NOT_MODIFIED                   = 304,  ///< Not Modified
    KLB_NETCODE_HTTP_USE_PROXY                      = 305,  ///< Use Proxy
    KLB_NETCODE_HTTP_TEMPORARY_REDIRECT             = 307,  ///< Temporary Redirect
    KLB_NETCODE_HTTP_PERMANENT_REDIRECT             = 308,  ///< Permanent Redirect
    KLB_NETCODE_HTTP_BAD_REQUEST                    = 400,  ///< Bad Request
    KLB_NETCODE_HTTP_UNAUTHORIZED                   = 401,  ///< Unauthorized
    KLB_NETCODE_HTTP_PAYMENT_REQUIRED               = 402,  ///< Payment Required
    KLB_NETCODE_HTTP_FORBIDDEN                      = 403,  ///< Forbidden
    KLB_NETCODE_HTTP_NOT_FOUND                      = 404,  ///< Not Found
    KLB_NETCODE_HTTP_METHOD_NOT_ALLOWED             = 405,  ///< Method Not Allowed
    KLB_NETCODE_HTTP_NOT_ACCEPTABLE                 = 406,  ///< Not Acceptable
    KLB_NETCODE_HTTP_PROXY_AUTHENTICATION_REQUIRED  = 407,  ///< Proxy Authentication Required
    KLB_NETCODE_HTTP_REQUEST_TIMEOUT                = 408,  ///< Request Timeout
    KLB_NETCODE_HTTP_CONFLICT                       = 409,  ///< Conflict
    KLB_NETCODE_HTTP_GONE                           = 410,  ///< Gone
    KLB_NETCODE_HTTP_LENGTH_REQUIRED                = 411,  ///< Length Required
    KLB_NETCODE_HTTP_PRECONDITION_FAILED            = 412,  ///< Precondition Failed
    KLB_NETCODE_HTTP_PAYLOAD_TOO_LARGE              = 413,  ///< Payload Too Large
    KLB_NETCODE_HTTP_URI_TOO_LONG                   = 414,  ///< URI Too Long
    KLB_NETCODE_HTTP_UNSUPPORTED_MEDIA_TYPE         = 415,  ///< Unsupported Media Type
    KLB_NETCODE_HTTP_RANGE_NOT_SATISFIABLE          = 416,  ///< Range Not Satisfiable
    KLB_NETCODE_HTTP_EXPECTATION_FAILED             = 417,  ///< Expectation Failed
    KLB_NETCODE_HTTP_MISDIRECTED_REQUEST            = 421,  ///< Misdirected Request
    KLB_NETCODE_HTTP_UNPROCESSABLE_ENTITY           = 422,  ///< Unprocessable Entity
    KLB_NETCODE_HTTP_LOCKED                         = 423,  ///< Locked
    KLB_NETCODE_HTTP_FAILED_DEPENDENCY              = 424,  ///< Failed Dependency
    KLB_NETCODE_HTTP_UPGRADE_REQUIRED               = 426,  ///< Upgrade Required
    KLB_NETCODE_HTTP_PRECONDITION_REQUIRED          = 428,  ///< Precondition Required
    KLB_NETCODE_HTTP_TOO_MANY_REQUESTS              = 429,  ///< Too Many Requests
    KLB_NETCODE_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431, ///< Request Header Fields Too Large
    KLB_NETCODE_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS  = 451,  ///< Unavailable For Legal Reasons
    KLB_NETCODE_HTTP_INTERNAL_SERVER_ERROR          = 500,  ///< Internal Server Error
    KLB_NETCODE_HTTP_NOT_IMPLEMENTED                = 501,  ///< Not Implemented
    KLB_NETCODE_HTTP_BAD_GATEWAY                    = 502,  ///< Bad Gateway
    KLB_NETCODE_HTTP_SERVICE_UNAVAILABLE            = 503,  ///< Service Unavailable
    KLB_NETCODE_HTTP_GATEWAY_TIMEOUT                = 504,  ///< Gateway Timeout
    KLB_NETCODE_HTTP_VERSION_NOT_SUPPORTED          = 505,  ///< HTTP Version Not Supported
    KLB_NETCODE_HTTP_VARIANT_ALSO_NEGOTIATES        = 506,  ///< Variant Also Negotiates
    KLB_NETCODE_HTTP_INSUFFICIENT_STORAGE           = 507,  ///< Insufficient Storage
    KLB_NETCODE_HTTP_LOOP_DETECTED                  = 508,  ///< Loop Detected
    KLB_NETCODE_HTTP_NOT_EXTENDED                   = 510,  ///< Not Extended
    KLB_NETCODE_HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511, ///< Network Authentication Required

    //////////////////////////////////////////////////////////////////////////
    // W WebSocket close [1000, 4999] (RFC6455; 与 KLB_WEBSOCKET_CLOSE_* 常用同值)

    KLB_NETCODE_WS_NORMAL           = 1000,                     ///< Normal Closure
    KLB_NETCODE_WS_GOING_AWAY       = 1001,                     ///< Going Away
    KLB_NETCODE_WS_PROTO            = 1002,                     ///< Protocol Error
    KLB_NETCODE_WS_UNSUPPORTED      = 1003,                     ///< Unsupported Data
    KLB_NETCODE_WS_NOSTATUS         = 1005,                     ///< No Status Rcvd (保留)
    KLB_NETCODE_WS_ABNORMAL         = 1006,                     ///< Abnormal Closure (保留)
    KLB_NETCODE_WS_UTF8             = 1007,                     ///< Invalid frame payload data
    KLB_NETCODE_WS_POLICY           = 1008,                     ///< Policy Violation
    KLB_NETCODE_WS_TOOBIG           = 1009,                     ///< Message Too Big
    KLB_NETCODE_WS_MANDATORY_EXT    = 1010,                     ///< Mandatory Ext.
    KLB_NETCODE_WS_INTERNAL         = 1011,                     ///< Internal Server Error
    KLB_NETCODE_WS_SERVICE_RESTART  = 1012,                     ///< Service Restart
    KLB_NETCODE_WS_TRY_AGAIN        = 1013,                     ///< Try Again Later
    KLB_NETCODE_WS_BAD_GATEWAY      = 1014,                     ///< Bad Gateway
    KLB_NETCODE_WS_TLS_HANDSHAKE    = 1015,                     ///< TLS handshake (保留)

    //////////////////////////////////////////////////////////////////////////
    // A 流控 [BASE+0, BASE+31]

    KLB_NETCODE_WBUF_EMPTY      = KLB_NETCODE_PRIVATE_BASE + 0, ///< 写缓存空
    KLB_NETCODE_RBUF_FULL       = KLB_NETCODE_PRIVATE_BASE + 1, ///< 读缓存满
    KLB_NETCODE_WBUF_FULL       = KLB_NETCODE_PRIVATE_BASE + 2, ///< 写缓存满

    //////////////////////////////////////////////////////////////////////////
    // B 连接 [BASE+32, BASE+63]

    KLB_NETCODE_CONNECT_FAIL    = KLB_NETCODE_PRIVATE_BASE + 32, ///< 建连失败 (泛化)
    KLB_NETCODE_CONNECT_REFUSED = KLB_NETCODE_PRIVATE_BASE + 33, ///< 对端拒绝连接
    KLB_NETCODE_DNS_FAIL        = KLB_NETCODE_PRIVATE_BASE + 34, ///< 主机名解析失败
    KLB_NETCODE_CONNECT_TIMEOUT = KLB_NETCODE_PRIVATE_BASE + 35, ///< 建连超时
    KLB_NETCODE_CONNECT_HOST    = KLB_NETCODE_PRIVATE_BASE + 36, ///< 主机可达性失败 (非 DNS)

    //////////////////////////////////////////////////////////////////////////
    // C TLS [BASE+64, BASE+95]

    KLB_NETCODE_TLS_HANDSHAKE   = KLB_NETCODE_PRIVATE_BASE + 64, ///< TLS 握手失败
    KLB_NETCODE_TLS_CERT        = KLB_NETCODE_PRIVATE_BASE + 65, ///< TLS 证书校验失败
    KLB_NETCODE_TLS_WANT_IO     = KLB_NETCODE_PRIVATE_BASE + 66, ///< TLS 非阻塞握手未完成 (可选上抛)
    KLB_NETCODE_TLS_ALERT       = KLB_NETCODE_PRIVATE_BASE + 67, ///< TLS 对端 alert 关连
    KLB_NETCODE_TLS_VERIFY      = KLB_NETCODE_PRIVATE_BASE + 68, ///< TLS 主机名/SNI 校验失败

    //////////////////////////////////////////////////////////////////////////
    // D listen / mux [BASE+96, BASE+127]

    KLB_NETCODE_BIND_FAIL       = KLB_NETCODE_PRIVATE_BASE + 96, ///< bind 失败
    KLB_NETCODE_LISTEN_FAIL     = KLB_NETCODE_PRIVATE_BASE + 97, ///< listen 失败
    KLB_NETCODE_ACCEPT_FAIL     = KLB_NETCODE_PRIVATE_BASE + 98, ///< accept 失败
    KLB_NETCODE_QUEUE_OVERFLOW  = KLB_NETCODE_PRIVATE_BASE + 99, ///< 分发/挂起队列溢出
    KLB_NETCODE_PROBE_FAIL      = KLB_NETCODE_PRIVATE_BASE + 100, ///< 协议嗅探失败
    KLB_NETCODE_ADDR_IN_USE     = KLB_NETCODE_PRIVATE_BASE + 101, ///< 地址已占用 (EADDRINUSE)
    KLB_NETCODE_ADDR_NOT_AVAIL  = KLB_NETCODE_PRIVATE_BASE + 102, ///< 地址不可用 (EADDRNOTAVAIL)

    //////////////////////////////////////////////////////////////////////////
    // E 协议传输 [BASE+128, BASE+159] (帧/解析; 库语义, 非 HTTP/WS 协议码)

    KLB_NETCODE_PROTO_PARSE     = KLB_NETCODE_PRIVATE_BASE + 128, ///< 协议帧解析错误
    KLB_NETCODE_PROTO_TOO_LARGE = KLB_NETCODE_PRIVATE_BASE + 129, ///< 协议帧/体过大
    KLB_NETCODE_PROTO_UNEXPECTED = KLB_NETCODE_PRIVATE_BASE + 130, ///< 非预期帧/状态机错误
    KLB_NETCODE_PROTO_VERSION   = KLB_NETCODE_PRIVATE_BASE + 131, ///< 协议版本不支持

    //////////////////////////////////////////////////////////////////////////
    // G 会话/限额 [BASE+160, BASE+191]

    KLB_NETCODE_CANCELED        = KLB_NETCODE_PRIVATE_BASE + 160, ///< 本地取消/中止操作
    KLB_NETCODE_LIMIT           = KLB_NETCODE_PRIVATE_BASE + 161, ///< 连接数/速率等限额

    //////////////////////////////////////////////////////////////////////////
    // F 保留 [BASE+192, PRIVATE_MAX]

    KLB_NETCODE_PRIVATE_END     = KLB_NETCODE_PRIVATE_MAX,  ///< 私有段上限 (同 KLB_NETCODE_PRIVATE_MAX)
}klb_netcode_e;


/// @brief 将 klb_netcode_e / klb_socket_status_e 转为只读描述串
/// @param [in]  code               消息码
/// @return const char*             静态串; 带 "[klbnet] " 前缀; 未知为 "Unknown error"
/// @note 对齐 strerror; 覆盖 socket 段与私有段已定义码
KLB_API const char* klb_netcode_strerror(int code);


/// @brief 将宿主 errno / WSAGetLastError 映射为 klb_netcode_e
/// @param [in]  syserr             宿主错误号 (Linux errno 或 Win WSA*)
/// @return int                     klb_netcode_e; 未知为 KLB_NETCODE_EIO; 0 为 KLB_NETCODE_EPERM
KLB_API int klb_netcode_from_syserr(int syserr);


/// @brief 读取并映射最近一次宿主 socket/系统错误
/// @return int                     klb_netcode_e (同 klb_netcode_from_syserr)
/// @note Win 读 WSAGetLastError; 其它读 errno
KLB_API int klb_netcode_last_syserr(void);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_NETCODE_H__

// end
