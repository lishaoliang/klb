// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_xpc.h
/// @brief   (XPC)本机跨进程过程调用
///          组装 IPC 传输(默认 msgq; 可选 unix / npipe / sysmq), 向上提供类 LPC 的
///          post/request/response/notify
/// @version 0.6
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_XPC_H__
#define __KLB_XPC_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_XPC_PATH_MAX
/// @brief 路径/名称缓冲上限(含结尾0); 与 unix sun_path 对齐
/// @note  C 仅定义此最大值; 业务短名策略由脚本层自行约定; 默认拼接仍可用短名
#define KLB_XPC_PATH_MAX            108

/// @def   KLB_XPC_NAME_BUF
/// @brief 模块名缓冲长度(含结尾0); 同 PATH_MAX
#define KLB_XPC_NAME_BUF            KLB_XPC_PATH_MAX

/// @def   KLB_XPC_NAME_LEN
/// @brief 模块名最大 strlen (不含结尾0) = NAME_BUF-1
#define KLB_XPC_NAME_LEN            (KLB_XPC_NAME_BUF - 1)

/// @def   KLB_XPC_TRANSPORT_MSGQ
/// @brief 传输: 命名消息队列(默认; SHM 环缓 klb_msgq; 全平台)
#define KLB_XPC_TRANSPORT_MSGQ      0

/// @def   KLB_XPC_TRANSPORT_UNIX
/// @brief 传输: Unix domain SOCK_STREAM(仅非 Win)
/// @note  默认路径 "/tmp/klb.xpc.<name>"; 帧=klb_xpc_hdr+payload;
///        发送侧按消息 connect/write/close, 对标 msgq open/send/destroy
#define KLB_XPC_TRANSPORT_UNIX      1

/// @def   KLB_XPC_TRANSPORT_NPIPE
/// @brief 传输: 命名管道(klb_npipe; 全平台)
/// @note  默认路径 POSIX "/tmp/klb.xpc.n.<name>";
///        Win "\\\\.\\pipe\\klb.xpc.n.<name>"; 帧同 hdr+payload;
///        发送侧按消息 open/write/destroy 对端 pipe
#define KLB_XPC_TRANSPORT_NPIPE     2

/// @def   KLB_XPC_TRANSPORT_SYSMQ
/// @brief 传输: 系统级命名消息队列(klb_sysmq / POSIX mqueue)
/// @note  默认 inbox 名 "xpc.in.<name>"(经 sysmq 规范为 /klb.xpc.in.<name>);
///        帧=整消息 klb_xpc_hdr+payload; Win/无 mqueue 返回 NULL;
///        与 TRANSPORT_MSGQ(SHM) 并列, 非同一后端
#define KLB_XPC_TRANSPORT_SYSMQ     3


/// @def   KLB_XPC_POST
/// @brief POST: 无响应
#define KLB_XPC_POST                10

/// @def   KLB_XPC_REQUEST
/// @brief REQUEST: 请求, 须 RESPONSE
#define KLB_XPC_REQUEST             11

/// @def   KLB_XPC_RESPONSE
/// @brief RESPONSE: 对 REQUEST 的回应
#define KLB_XPC_RESPONSE            12

/// @def   KLB_XPC_NOTIFY
/// @brief NOTIFY: 通知
#define KLB_XPC_NOTIFY              13


/// @struct klb_xpc_t
/// @brief  XPC 端点(本机跨进程)
typedef struct klb_xpc_t_ klb_xpc_t;


/// @brief 创建 XPC 端点(服务侧; msgq)
/// @param [in] *p_name         模块名(非空); 长度 <= KLB_XPC_NAME_LEN
/// @return klb_xpc_t* 对象; 失败返回 NULL
/// @note 底层 inbox: msgq 名 "xpc.in.<name>"; 仅创建方拆除内核对象
KLB_API klb_xpc_t* klb_xpc_create(const char* p_name);


/// @brief 创建 XPC 端点(服务侧; unix socket)
/// @param [in] *p_name         模块名(非空); 长度 <= KLB_XPC_NAME_LEN
/// @param [in] *p_path         监听路径; NULL/空=默认 "/tmp/klb.xpc.<name>"
/// @return klb_xpc_t* 对象; 失败返回 NULL(含 Win)
/// @note 后续若再增传输, 按类型新增 create_<type>, 勿再做通用 create_ex
KLB_API klb_xpc_t* klb_xpc_create_unix(const char* p_name, const char* p_path);


/// @brief 打开对端模块(客户端; msgq)
/// @param [in] *p_name         对端已 create 的模块名(非空)
/// @return klb_xpc_t* 对象; 对端不存在或失败返回 NULL
/// @note 本地另建临时 inbox 以收 RESPONSE; is_owner=false
KLB_API klb_xpc_t* klb_xpc_open(const char* p_name);


/// @brief 打开对端模块(客户端; unix socket)
/// @param [in] *p_name         对端模块名
/// @param [in] *p_path         对端路径; NULL/空=默认 "/tmp/klb.xpc.<name>"
/// @return klb_xpc_t* 对象; 失败返回 NULL(含 Win)
KLB_API klb_xpc_t* klb_xpc_open_unix(const char* p_name, const char* p_path);


/// @brief 创建 XPC 端点(服务侧; 命名管道 npipe)
/// @param [in] *p_name         模块名(非空); 长度 <= KLB_XPC_NAME_LEN
/// @param [in] *p_path         管道路径; NULL/空=默认见 KLB_XPC_TRANSPORT_NPIPE
/// @return klb_xpc_t* 对象; 失败返回 NULL
KLB_API klb_xpc_t* klb_xpc_create_npipe(const char* p_name, const char* p_path);


/// @brief 打开对端模块(客户端; 命名管道 npipe)
/// @param [in] *p_name         对端模块名
/// @param [in] *p_path         对端路径; NULL/空=默认见 KLB_XPC_TRANSPORT_NPIPE
/// @return klb_xpc_t* 对象; 失败返回 NULL
KLB_API klb_xpc_t* klb_xpc_open_npipe(const char* p_name, const char* p_path);


/// @brief 创建 XPC 端点(服务侧; 系统 mq / klb_sysmq)
/// @param [in] *p_name         模块名(非空); 长度 <= KLB_XPC_NAME_LEN
/// @param [in] *p_path         mq 名; NULL/空=默认 "xpc.in.<name>"
/// @return klb_xpc_t* 对象; 失败返回 NULL(含 Win / 无 mqueue)
KLB_API klb_xpc_t* klb_xpc_create_sysmq(const char* p_name, const char* p_path);


/// @brief 打开对端模块(客户端; 系统 mq / klb_sysmq)
/// @param [in] *p_name         对端模块名
/// @param [in] *p_path         对端 mq 名; NULL/空=默认 "xpc.in.<name>"
/// @return klb_xpc_t* 对象; 失败返回 NULL(含 Win / 无 mqueue)
KLB_API klb_xpc_t* klb_xpc_open_sysmq(const char* p_name, const char* p_path);


/// @brief 销毁本地端点句柄
/// @param [in] *p_xpc          对象
/// @return 无
KLB_API void klb_xpc_destroy(klb_xpc_t* p_xpc);


/// @brief 向目标模块 POST(无响应)
/// @param [in] *p_xpc          本端
/// @param [in] *p_dst          目标模块名
/// @param [in] *p_data         载荷; 可为 NULL 当 len=0
/// @param [in] len             字节数
/// @return int 0 成功; 非0 失败
KLB_API int klb_xpc_post(klb_xpc_t* p_xpc, const char* p_dst, const void* p_data, uint32_t len);


/// @brief 向目标模块 REQUEST 并等待 RESPONSE
/// @param [in]  *p_xpc         本端
/// @param [in]  *p_dst         目标模块名
/// @param [in]  *p_data        请求载荷
/// @param [in]  len            请求字节数
/// @param [out] *p_buf         回应缓冲
/// @param [in]  buf_size       缓冲大小
/// @param [out] *p_out_len     可选; 实际回应长度
/// @param [in]  timeout_ms     超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时
/// @note 等待期间收到的其它消息进入本端 pending, 供后续 recv 取出
KLB_API int klb_xpc_request(klb_xpc_t* p_xpc, const char* p_dst,
                            const void* p_data, uint32_t len,
                            void* p_buf, uint32_t buf_size, uint32_t* p_out_len,
                            uint32_t timeout_ms);


/// @brief 接收一条消息
/// @param [in]  *p_xpc         本端
/// @param [out] *p_type        消息类型: KLB_XPC_*
/// @param [out] *p_src         来源名缓冲; 容量 >= KLB_XPC_NAME_BUF
/// @param [out] *p_buf         载荷缓冲
/// @param [in]  buf_size       缓冲大小
/// @param [out] *p_out_len     可选; 实际载荷长度
/// @param [out] *p_corr_id     可选; REQUEST/RESPONSE 相关 id; 其它类型为 0
/// @param [in]  timeout_ms     超时; 0=不等待; (uint32_t)-1=一直等
/// @return int 0 成功; 非0 失败/超时/空
KLB_API int klb_xpc_recv(klb_xpc_t* p_xpc, int* p_type, char* p_src,
                         void* p_buf, uint32_t buf_size, uint32_t* p_out_len,
                         uint32_t* p_corr_id, uint32_t timeout_ms);


/// @brief 对 REQUEST 回 RESPONSE
/// @param [in] *p_xpc          本端
/// @param [in] *p_dst          目标(请求来源)模块名
/// @param [in] corr_id         与 recv 取得的 corr_id 一致
/// @param [in] *p_data         回应载荷
/// @param [in] len             字节数
/// @return int 0 成功; 非0 失败
KLB_API int klb_xpc_response(klb_xpc_t* p_xpc, const char* p_dst, uint32_t corr_id,
                             const void* p_data, uint32_t len);


/// @brief 向目标模块 NOTIFY
/// @param [in] *p_xpc          本端
/// @param [in] *p_dst          目标模块名
/// @param [in] *p_data         载荷
/// @param [in] len             字节数
/// @return int 0 成功; 非0 失败
KLB_API int klb_xpc_notify(klb_xpc_t* p_xpc, const char* p_dst, const void* p_data, uint32_t len);


/// @brief 是否为创建方(服务侧 create)
/// @param [in] *p_xpc          对象
/// @return bool true=创建方
KLB_API bool klb_xpc_is_owner(klb_xpc_t* p_xpc);


/// @brief 本端模块名
/// @param [in] *p_xpc          对象
/// @return const char* 本地名; 永不为 NULL
KLB_API const char* klb_xpc_name(klb_xpc_t* p_xpc);


#ifdef __cplusplus
}
#endif

#endif // __KLB_XPC_H__

// end
