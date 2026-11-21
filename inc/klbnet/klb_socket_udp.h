// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_udp.h
/// @brief   UDP socket 操作接口 (基于 klb_socket_t)
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_SOCKET_UDP_H__
#define __KLB_SOCKET_UDP_H__

#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def KLB_SOCKET_F_REUSEADDR
/// @brief UDP 打开/绑定标志 (klb_socket_udp_open / klb_socket_udp_bind)
#define KLB_SOCKET_F_REUSEADDR      (1u << 0)   ///< SO_REUSEADDR
#define KLB_SOCKET_F_REUSEPORT      (1u << 1)   ///< SO_REUSEPORT
#define KLB_SOCKET_F_BROADCAST      (1u << 2)   ///< SO_BROADCAST (仅 IPv4)
#define KLB_SOCKET_F_V6ONLY         (1u << 3)   ///< AF_INET6: 置位仅 v6; 未置位尝试双栈


/// @enum  klb_socket_udp_recv_e
/// @brief UDP 收包判定
typedef enum klb_socket_udp_recv_e_
{
    KLB_SOCKET_UDP_RECV_DATA   = 0,     ///< 收到数据报 (长度为 0 也合法)
    KLB_SOCKET_UDP_RECV_AGAIN  = 1,     ///< 暂无数据, 需等待
    KLB_SOCKET_UDP_RECV_ERROR  = 2,     ///< 错误
}klb_socket_udp_recv_e;


/// @brief 创建 UDP fd (不 bind; 不强制 REUSEADDR)
/// @param [in]  family         KLB_SOCKET_FAMILY_V4 / KLB_SOCKET_FAMILY_V6
/// @return klb_socket_fd INVALID_SOCKET.失败; 非 INVALID_SOCKET.fd
KLB_API klb_socket_fd klb_socket_udp_family(int family);


/// @brief 创建 UDP socket 对象 (fd + 选项 + 可选 bind + 包装)
/// @param [in]  family         KLB_SOCKET_FAMILY_V4 / V6 / UNSPEC
/// @param [in]  *p_host        NULL/""/"*" 表任意地址; p_host 与 port 均为空则不 bind
/// @param [in]  port           0.临时端口
/// @param [in]  flags          KLB_SOCKET_F_*
/// @return klb_socket_t* NULL.失败; 非 NULL.socket 对象
/// @note 失败时不保留 fd; 成功后仍须 klb_socket_set_connected/reading 再入 netmulti
KLB_API klb_socket_t* klb_socket_udp_open(int family, const char* p_host, int port, uint32_t flags);


/// @brief 绑定本地地址 (失败不 assert; 供端口重试)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_host        NULL/""/"*" 表任意地址
/// @param [in]  port           端口
/// @param [in]  flags          KLB_SOCKET_F_*
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 已绑定时返回 KLB_NETCODE_BIND_FAIL
KLB_API int klb_socket_udp_bind(klb_socket_t* p_socket, const char* p_host, int port, uint32_t flags);


/// @brief 连接固定对端 (域名:端口)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_host        对端主机
/// @param [in]  port           对端端口
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_connect(klb_socket_t* p_socket, const char* p_host, int port);


/// @brief 连接固定对端 (地址)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_addr        对端地址
/// @param [in]  addr_len       p_addr 有效长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_connect_sa(klb_socket_t* p_socket,
    const struct sockaddr* p_addr, int addr_len);


/// @brief 非连接型发送
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_data        数据
/// @param [in]  len            数据长度
/// @param [in]  *p_addr        目标地址
/// @param [in]  addr_len       p_addr 有效长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len,
    const struct sockaddr* p_addr, int addr_len);


/// @brief 非连接型发送 (域名:端口)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_data        数据
/// @param [in]  len            数据长度
/// @param [in]  *p_host        目标主机
/// @param [in]  port           目标端口
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_sendto_host(klb_socket_t* p_socket, const uint8_t* p_data, int len,
    const char* p_host, int port);


/// @brief 连接型发送 (仅 klb_socket_udp_connect 后)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_data        数据
/// @param [in]  len            数据长度
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_send(klb_socket_t* p_socket, const uint8_t* p_data, int len);


/// @brief 接收一个数据报
/// @param [in]  *p_socket      UDP socket 对象
/// @param [out] *p_buf         接收缓冲
/// @param [in]  cap            p_buf 容量
/// @param [out] *p_addr        来源地址; 可为 NULL
/// @param [out] *p_addr_len    来源地址长度; 可为 NULL
/// @return int 大于等于 0.报文长度 (0 为合法空报文); 小于 0.klb_netcode_e
/// @note 内部更新 p_socket->peer
KLB_API int klb_socket_udp_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int cap,
    struct sockaddr* p_addr, int* p_addr_len);


/// @brief UDP 收包判定 (无 EOF 语义)
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  n               klb_socket_udp_recvfrom 返回值
/// @param [in]  cap            接收缓冲容量
/// @param [out] *p_truncated   是否可能被截断; 可为 NULL
/// @return int klb_socket_udp_recv_e
KLB_API int klb_socket_udp_recv_judge(klb_socket_t* p_socket, int n, int cap, bool* p_truncated);


/// @brief 开启/关闭 SO_BROADCAST
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  enable         true.开启; false.关闭
/// @return int 0.成功; 小于 0.klb_netcode_e
/// @note 仅 IPv4; IPv6 返回 KLB_NETCODE_NOTSUP
KLB_API int klb_socket_udp_set_broadcast(klb_socket_t* p_socket, bool enable);


/// @brief 加入 IPv4 组播组
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_group       组播地址 (点分十进制)
/// @param [in]  *p_if_ip       网卡地址; NULL/"" 表 INADDR_ANY
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_join_v4(klb_socket_t* p_socket, const char* p_group, const char* p_if_ip);


/// @brief 退出 IPv4 组播组
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_group       组播地址 (点分十进制)
/// @param [in]  *p_if_ip       网卡地址; NULL/"" 表 INADDR_ANY
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_leave_v4(klb_socket_t* p_socket, const char* p_group, const char* p_if_ip);


/// @brief 加入 IPv6 组播组
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_group       组播地址 (IPv6 文本)
/// @param [in]  ifindex        网卡 ifindex; 0 表默认
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_join_v6(klb_socket_t* p_socket, const char* p_group, unsigned int ifindex);


/// @brief 退出 IPv6 组播组
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_group       组播地址 (IPv6 文本)
/// @param [in]  ifindex        网卡 ifindex; 0 表默认
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_leave_v6(klb_socket_t* p_socket, const char* p_group, unsigned int ifindex);


/// @brief 设置组播 TTL / hops
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  ttl            TTL/hops (0~255); IPv4=IP_MULTICAST_TTL; IPv6=IPV6_MULTICAST_HOPS
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_set_mcast_ttl(klb_socket_t* p_socket, int ttl);


/// @brief 设置组播回环
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  enable         true.回环; false.不回环
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_set_mcast_loop(klb_socket_t* p_socket, bool enable);


/// @brief 设置组播出接口
/// @param [in]  *p_socket      UDP socket 对象
/// @param [in]  *p_if          IPv4: 点分 IP, NULL/""=ANY; IPv6: ifindex 十进制串, NULL/""=0
/// @return int 0.成功; 小于 0.klb_netcode_e
KLB_API int klb_socket_udp_set_mcast_if(klb_socket_t* p_socket, const char* p_if);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_SOCKET_UDP_H__

// end
