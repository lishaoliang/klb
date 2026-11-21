// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netmux.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   共享 listen + probe 分发中枢
///          单端口 accept 后嗅探协议, 按协议入队供上层分发
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETMUX_H__
#define __KLB_NETMUX_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klbnet/klblisten/klb_netlisten_conn.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_netmux_t
/// @brief  共享监听分发器
typedef struct klb_netmux_t_ klb_netmux_t;


/// @struct klb_netmux_socket_t
/// @brief  分发队列中的已探测 socket / accept 错误哨兵
/// @note 前三字段布局与 klb_netlisten_socket_t 一致, 可供 krtsp.new_serve 等复用
///       err_code!=0 时为错误事件 (fd 无效); addr_sa 为完整对端地址 (含 IPv6)
///       尾部 listen_port 供组监听/跨线程移交保留「从哪口进来」, 勿插到前三字段之前
typedef struct klb_netmux_socket_t_
{
    klb_socket_fd               fd;         ///< socket fd; 错误哨兵为 INVALID_SOCKET
    struct sockaddr_in          addr;       ///< 对端 IPv4 (IPv4-mapped 已折算; 纯 IPv6 为零)
    bool                        tls;        ///< 是否 TLS ClientHello
    int                         protocol;   ///< klb_protocol_e; TLS 归入 HTTP 时为 HTTP
    int                         err_code;   ///< 0=连接; 非0=accept 错误 (klb_netcode_e)
    struct sockaddr_storage     addr_sa;    ///< 完整对端地址 (AF_INET / AF_INET6)

    // 组监听 / 跨线程移交附加 (尾部追加; 不破坏前三字段布局)
    int                         listen_port;///< 本 mux 监听端口 (主机序); 0=未知或未填; 跨线程勿丢
}klb_netmux_socket_t;


/// @brief 队列就绪回调 (有新连接可 pop)
/// @param [in]  *p_mux             分发器
/// @param [in]  *ptr               用户指针
/// @return 无
typedef void(*klb_netmux_ready_cb)(klb_netmux_t* p_mux, void* ptr);


/// @brief 创建分发器
/// @param [in]  *p_multi           复用模块
/// @return klb_netmux_t*           分发器; 失败为 NULL
KLB_API klb_netmux_t* klb_netmux_create(klb_netmulti_t* p_multi);


/// @brief 销毁分发器
/// @param [in]  *p_mux             分发器
/// @return 无
/// @note 若已 open, 关闭 listen 与未取走的 fd; 进行中的 probe 中止
KLB_API void klb_netmux_destroy(klb_netmux_t* p_mux);


/// @brief 监听端口
/// @param [in]  *p_mux             分发器
/// @param [in]  port               端口
/// @param [in]  max_connect        listen backlog
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netmux_open(klb_netmux_t* p_mux, int port, int max_connect);


/// @brief 监听端口 (指定绑定地址)
/// @param [in]  *p_host            绑定地址; NULL/""/"0.0.0.0"/"*" 表示 INADDR_ANY
/// @param [in]  port               端口
/// @param [in]  max_connect        listen backlog
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netmux_open_addr(klb_netmux_t* p_mux, const char* p_host, int port, int max_connect);


/// @brief 监听端口 (指定绑定地址 + 族/套接字选项)
/// @param [in]  *p_opts            可选; NULL 等价 AF_INET
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netmux_open_addr_ex(klb_netmux_t* p_mux, const char* p_host, int port, int max_connect,
    const klb_netlisten_open_opts_t* p_opts);


/// @brief 以新绑定地址重开监听 (同端口; 既有已 accept 连接不受影响)
/// @param [in]  *p_mux             分发器
/// @param [in]  *p_host            新绑定地址; NULL/"" 表示 INADDR_ANY
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 端口沿用当前实际绑定端口 (port=0 时先回读); 重开期间新连接短暂不可达
KLB_API int klb_netmux_rebind(klb_netmux_t* p_mux, const char* p_host);


/// @brief 以新绑定地址重开监听 (保留族/选项)
/// @return int 0.成功; 非0.klb_netcode_e
KLB_API int klb_netmux_rebind_ex(klb_netmux_t* p_mux, const char* p_host, const klb_netlisten_open_opts_t* p_opts);


/// @brief 获取配置的绑定地址
/// @param [in]  *p_mux             分发器
/// @return const char* 绑定地址; INADDR_ANY 为空串; 未 open 为 NULL
KLB_API const char* klb_netmux_get_host(klb_netmux_t* p_mux);


/// @brief 回读监听本地地址 (getsockname)
/// @param [in]  *p_mux             分发器
/// @param [out] *p_addr            输出本地地址
/// @return int 0.成功; 非0.失败
KLB_API int klb_netmux_get_local_addr(klb_netmux_t* p_mux, struct sockaddr_in* p_addr);


/// @brief 回读监听本地地址 (AF_INET / AF_INET6)
/// @return int 0.成功; 非0.失败
KLB_API int klb_netmux_get_local_sa(klb_netmux_t* p_mux, struct sockaddr_storage* p_addr, socklen_t* p_addr_len);


/// @brief 取监听 native fd
/// @return klb_socket_fd; 未 open 为 INVALID_SOCKET
KLB_API klb_socket_fd klb_netmux_native_fd(klb_netmux_t* p_mux);


/// @brief 是否已 open
/// @param [in]  *p_mux             分发器
/// @return bool
KLB_API bool klb_netmux_is_open(klb_netmux_t* p_mux);


/// @brief 获取监听端口
/// @param [in]  *p_mux             分发器
/// @return int 端口; 未 open 为 0
KLB_API int klb_netmux_get_port(klb_netmux_t* p_mux);


/// @brief 设置 TLS 是否按 HTTP 投递
/// @param [in]  *p_mux             分发器
/// @param [in]  enable             true 时 TLS ClientHello 入队为 HTTP+tls
/// @return int 0
/// @note 默认 true; false 时按 RTSP/MNP/SMP (见 set_tls_as_rtsp / set_tls_as_mnp)
KLB_API int klb_netmux_set_tls_as_http(klb_netmux_t* p_mux, bool enable);


/// @brief 设置非 HTTP TLS 是否按 MNP 投递
/// @param [in]  *p_mux             分发器
/// @param [in]  enable             true 且 tls_as_http=false 且 tls_as_rtsp=false 时 TLS 入队为 MNP+tls
/// @return int 0
/// @note 默认 false (TLS 非 HTTP/RTSP 时投递 SMP); 与 https / rtsps 互斥
KLB_API int klb_netmux_set_tls_as_mnp(klb_netmux_t* p_mux, bool enable);


/// @brief 设置非 HTTP TLS 是否按 RTSP 投递
/// @param [in]  *p_mux             分发器
/// @param [in]  enable             true 且 tls_as_http=false 时 TLS 入队为 RTSP+tls
/// @return int 0
/// @note 默认 false; 优先于 tls_as_mnp; 与 https 互斥
KLB_API int klb_netmux_set_tls_as_rtsp(klb_netmux_t* p_mux, bool enable);


/// @brief 设置队列就绪回调
/// @param [in]  *p_mux             分发器
/// @param [in]  cb_ready           回调; 可为 NULL
/// @param [in]  *ptr               附加指针
/// @return int 0
KLB_API int klb_netmux_set_ready(klb_netmux_t* p_mux, klb_netmux_ready_cb cb_ready, void* ptr);


/// @brief 取出一个已探测连接
/// @param [in]  *p_mux             分发器
/// @return klb_netmux_socket_t*    队列头; 空为 NULL
/// @note 调用方负责 KLB_FREE, 并负责关闭未使用的 fd
KLB_API klb_netmux_socket_t* klb_netmux_pop(klb_netmux_t* p_mux);


/// @brief 队列长度
/// @param [in]  *p_mux             分发器
/// @return int
KLB_API int klb_netmux_queue_size(klb_netmux_t* p_mux);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETMUX_H__

// end
