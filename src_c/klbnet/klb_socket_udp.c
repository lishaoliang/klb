///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket_udp.c
/// @brief   UDP socket 操作实现 (基于 klb_socket_t)
/// @version 0.1
/// @history 修改历史
///  \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#include "klbnet/klb_socket_udp.h"
#include "klbnet/klb_netcode.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#if defined(IPV6_JOIN_GROUP)
#define KLB_SOCKET_UDP_IPV6_JOIN    IPV6_JOIN_GROUP
#define KLB_SOCKET_UDP_IPV6_LEAVE   IPV6_LEAVE_GROUP
#elif defined(IPV6_ADD_MEMBERSHIP)
#define KLB_SOCKET_UDP_IPV6_JOIN    IPV6_ADD_MEMBERSHIP
#define KLB_SOCKET_UDP_IPV6_LEAVE   IPV6_DROP_MEMBERSHIP
#endif


#define KLB_SOCKET_UDP_FLAGS_ALL    (KLB_SOCKET_F_REUSEADDR | KLB_SOCKET_F_REUSEPORT \
    | KLB_SOCKET_F_BROADCAST | KLB_SOCKET_F_V6ONLY)


/// @brief 取最近一次 socket 错误
static int last_err_klb_socket_udp()
{
#ifdef _WIN32
    return (int)WSAGetLastError();
#else
    return errno;
#endif
}

/// @brief 平台错误转 klb_netcode_e
static int netcode_from_err_klb_socket_udp(int err)
{
#ifdef _WIN32
    if (WSAEWOULDBLOCK == err)
    {
        return KLB_NETCODE_EAGAIN;
    }
    else if (WSAEINTR == err)
    {
        return KLB_NETCODE_EINTR;
    }
    else if (WSAEMSGSIZE == err)
    {
        return KLB_NETCODE_EMSGSIZE;
    }
    else if (WSAEACCES == err)
    {
        return KLB_NETCODE_EACCES;
    }
    else if (WSAEADDRINUSE == err)
    {
        return KLB_NETCODE_ADDR_IN_USE;
    }
    else if (WSAEADDRNOTAVAIL == err)
    {
        return KLB_NETCODE_ADDR_NOT_AVAIL;
    }
    else if (WSAENETUNREACH == err)
    {
        return KLB_NETCODE_ENETUNREACH;
    }
    else if (WSAEHOSTUNREACH == err)
    {
        return KLB_NETCODE_EHOSTUNREACH;
    }
    else if (WSAECONNREFUSED == err)
    {
        return KLB_NETCODE_ECONNREFUSED;
    }
    else if (WSAECONNRESET == err)
    {
        return KLB_NETCODE_ECONNRESET;
    }
    else if (WSAEINVAL == err)
    {
        return KLB_NETCODE_EINVAL;
    }
    else if (WSAEAFNOSUPPORT == err)
    {
        return KLB_NETCODE_EAFNOSUPPORT;
    }
    else if (WSAENOBUFS == err)
    {
        return KLB_NETCODE_ENOBUFS;
    }
    else if (WSAENOTSOCK == err)
    {
        return KLB_NETCODE_EBADF;
    }

    return KLB_NETCODE_EPERM;
#else
    if (EAGAIN == err)
    {
        return KLB_NETCODE_EAGAIN;
    }
    else if (EINTR == err)
    {
        return KLB_NETCODE_EINTR;
    }
    else if (EMSGSIZE == err)
    {
        return KLB_NETCODE_EMSGSIZE;
    }
    else if (EACCES == err)
    {
        return KLB_NETCODE_EACCES;
    }
    else if (EADDRINUSE == err)
    {
        return KLB_NETCODE_ADDR_IN_USE;
    }
    else if (EADDRNOTAVAIL == err)
    {
        return KLB_NETCODE_ADDR_NOT_AVAIL;
    }
    else if (ENETUNREACH == err)
    {
        return KLB_NETCODE_ENETUNREACH;
    }
    else if (EHOSTUNREACH == err)
    {
        return KLB_NETCODE_EHOSTUNREACH;
    }
    else if (ECONNREFUSED == err)
    {
        return KLB_NETCODE_ECONNREFUSED;
    }
    else if (ECONNRESET == err)
    {
        return KLB_NETCODE_ECONNRESET;
    }
    else if (EINVAL == err)
    {
        return KLB_NETCODE_EINVAL;
    }
    else if (EAFNOSUPPORT == err)
    {
        return KLB_NETCODE_EAFNOSUPPORT;
    }
    else if (ENOBUFS == err)
    {
        return KLB_NETCODE_ENOBUFS;
    }
    else if (EBADF == err)
    {
        return KLB_NETCODE_EBADF;
    }

    return KLB_NETCODE_EPERM;
#endif
}

/// @brief 项目地址族转 AF_*
static int af_from_family_klb_socket_udp(int family)
{
    if (KLB_SOCKET_FAMILY_V4 == family)
    {
        return AF_INET;
    }
    else if (KLB_SOCKET_FAMILY_V6 == family)
    {
        return AF_INET6;
    }

    return AF_UNSPEC;
}

/// @brief 回填本端地址
static void set_local_klb_socket_udp(klb_socket_t* p_socket)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(NULL != p_socket);

    if (0 != klb_socket_get_local_sa(p_socket->fd, &sa, &sa_len))
    {
        return;
    }

    memset(&p_socket->local, 0, sizeof(klb_socket_addr_t));

    if ((AF_INET == sa.ss_family) && (sa_len >= (socklen_t)sizeof(struct sockaddr_in)))
    {
        memcpy(&p_socket->local.addr, &sa, sizeof(struct sockaddr_in));
        p_socket->local.len = (socklen_t)sizeof(struct sockaddr_in);
    }
    else if ((AF_INET6 == sa.ss_family) && (sa_len >= (socklen_t)sizeof(struct sockaddr_in6)))
    {
        memcpy(&p_socket->local.addr, &sa, sizeof(struct sockaddr_in6));
        p_socket->local.len = (socklen_t)sizeof(struct sockaddr_in6);
    }

    if (KLB_SOCKET_FAMILY_UNSPEC == p_socket->family)
    {
        p_socket->family = (AF_INET6 == sa.ss_family)
            ? KLB_SOCKET_FAMILY_V6 : KLB_SOCKET_FAMILY_V4;
    }
}

/// @brief 回填对端地址
static void set_peer_klb_socket_udp(klb_socket_t* p_socket,
    const struct sockaddr* p_addr, socklen_t addr_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_addr);

    memset(&p_socket->peer, 0, sizeof(klb_socket_addr_t));

    if ((AF_INET == p_addr->sa_family) && (addr_len >= sizeof(struct sockaddr_in)))
    {
        memcpy(&p_socket->peer.addr, p_addr, sizeof(struct sockaddr_in));
        p_socket->peer.len = (socklen_t)sizeof(struct sockaddr_in);
    }
    else if ((AF_INET6 == p_addr->sa_family) && (addr_len >= sizeof(struct sockaddr_in6)))
    {
        memcpy(&p_socket->peer.addr, p_addr, sizeof(struct sockaddr_in6));
        p_socket->peer.len = (socklen_t)sizeof(struct sockaddr_in6);
    }

    if (KLB_SOCKET_FAMILY_UNSPEC == p_socket->family)
    {
        p_socket->family = (AF_INET6 == p_addr->sa_family)
            ? KLB_SOCKET_FAMILY_V6 : KLB_SOCKET_FAMILY_V4;
    }
}

/// @brief 应用打开/绑定标志
static int apply_flags_klb_socket_udp(klb_socket_fd fd, int af, uint32_t flags)
{
    if (0 != (flags & KLB_SOCKET_F_REUSEADDR))
    {
        if (0 != klb_socket_set_reuseaddr(fd))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }
    }

    if (0 != (flags & KLB_SOCKET_F_REUSEPORT))
    {
        if (0 != klb_socket_set_reuseport(fd))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }
    }

    // AF_INET6: 置位 F_V6ONLY=仅 v6; 未置位尝试双栈 (与 listen_sa 对齐)
    if (AF_INET6 == af)
    {
        bool v6only = (0 != (flags & KLB_SOCKET_F_V6ONLY));

        if (0 != klb_socket_set_ipv6only(fd, v6only))
        {
            // 部分平台无 IPV6_V6ONLY: 仅 dual(false) 时视为失败
            if (!v6only)
            {
                return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
            }
        }
    }

    return 0;
}

klb_socket_fd klb_socket_udp_family(int family)
{
    int af = af_from_family_klb_socket_udp(family);

    if (AF_UNSPEC == af)
    {
        af = AF_INET;
    }

    return socket(af, SOCK_DGRAM, IPPROTO_UDP);
}

klb_socket_t* klb_socket_udp_open(int family, const char* p_host, int port, uint32_t flags)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;
    bool need_bind = (NULL != p_host) || (0 < port);
    int af = af_from_family_klb_socket_udp(family);
    klb_socket_fd fd = INVALID_SOCKET;
    klb_socket_t* p_socket = NULL;

    if ((0 > port) || (65535 < port))
    {
        return NULL;
    }

    if (0 != (flags & ~KLB_SOCKET_UDP_FLAGS_ALL))
    {
        return NULL;
    }

    // step1. 解析绑定地址
    if (need_bind)
    {
        if (0 != klb_socket_resolve_sa(p_host, port, af, &sa, &sa_len))
        {
            return NULL;
        }

        af = (int)sa.ss_family;
    }
    else if (AF_UNSPEC == af)
    {
        af = AF_INET;
    }

    // step2. 创建 fd + 选项
    fd = socket(af, SOCK_DGRAM, IPPROTO_UDP);
    if (INVALID_SOCKET == fd)
    {
        return NULL;
    }

    if (0 != apply_flags_klb_socket_udp(fd, af, flags))
    {
        goto err_open;
    }

    if (0 != (flags & KLB_SOCKET_F_BROADCAST))
    {
        int on = 1;

        if (AF_INET6 == af)
        {
            goto err_open;
        }

        if (0 != setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on)))
        {
            goto err_open;
        }
    }

    // step3. bind (可选)
    if (need_bind)
    {
        if (0 != bind(fd, (const struct sockaddr*)&sa, sa_len))
        {
            goto err_open;
        }
    }

    // step4. 包装
    p_socket = klb_socket_async_create(fd);
    if (0 != (flags & KLB_SOCKET_F_BROADCAST))
    {
        p_socket->bcast = 1;
    }

    return p_socket;

err_open:
    KLB_SOCKET_CLOSE(fd);
    return NULL;
}

int klb_socket_udp_bind(klb_socket_t* p_socket, const char* p_host, int port, uint32_t flags)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(NULL != p_socket);

    if ((0 > port) || (65535 < port))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (0 != (flags & ~KLB_SOCKET_UDP_FLAGS_ALL))
    {
        return KLB_NETCODE_EINVAL;
    }

    // 已有本端端口视为已绑定
    if (0 < klb_socket_local_port(p_socket))
    {
        return KLB_NETCODE_BIND_FAIL;
    }

    int af = af_from_family_klb_socket_udp((int)p_socket->family);

    if (0 != klb_socket_resolve_sa(p_host, port, af, &sa, &sa_len))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    int ret = apply_flags_klb_socket_udp(p_socket->fd, (int)sa.ss_family, flags);
    if (0 != ret)
    {
        return ret;
    }

    if (0 != bind(p_socket->fd, (const struct sockaddr*)&sa, sa_len))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    set_local_klb_socket_udp(p_socket);

    return 0;
}

int klb_socket_udp_connect(klb_socket_t* p_socket, const char* p_host, int port)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(NULL != p_socket);
    assert(NULL != p_host);

    if ((0 > port) || (65535 < port))
    {
        return KLB_NETCODE_EINVAL;
    }

    int af = af_from_family_klb_socket_udp((int)p_socket->family);

    if (0 != klb_socket_resolve_sa(p_host, port, af, &sa, &sa_len))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    return klb_socket_udp_connect_sa(p_socket, (const struct sockaddr*)&sa, (int)sa_len);
}

int klb_socket_udp_connect_sa(klb_socket_t* p_socket,
    const struct sockaddr* p_addr, int addr_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_addr);

    if (0 >= addr_len)
    {
        return KLB_NETCODE_EINVAL;
    }

    if (0 != connect(p_socket->fd, p_addr, (socklen_t)addr_len))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    set_peer_klb_socket_udp(p_socket, p_addr, (socklen_t)addr_len);
    set_local_klb_socket_udp(p_socket);

    p_socket->connected = 1;

    return 0;
}

int klb_socket_udp_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len,
    const struct sockaddr* p_addr, int addr_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_data);
    assert(NULL != p_addr);
    assert(0 <= len);

    int ret = p_socket->vtable.cb_sendto(p_socket, p_data, len, p_addr, addr_len);
    if (0 > ret)
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }
    else if (ret != len)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    set_peer_klb_socket_udp(p_socket, p_addr, (socklen_t)addr_len);

    if (0 >= klb_socket_local_port(p_socket))
    {
        set_local_klb_socket_udp(p_socket);
    }

    return 0;
}

int klb_socket_udp_sendto_host(klb_socket_t* p_socket, const uint8_t* p_data, int len,
    const char* p_host, int port)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(NULL != p_socket);
    assert(NULL != p_host);

    if ((0 > port) || (65535 < port))
    {
        return KLB_NETCODE_EINVAL;
    }

    int af = af_from_family_klb_socket_udp((int)p_socket->family);

    if (0 != klb_socket_resolve_sa(p_host, port, af, &sa, &sa_len))
    {
        return KLB_NETCODE_DNS_FAIL;
    }

    return klb_socket_udp_sendto(p_socket, p_data, len,
        (const struct sockaddr*)&sa, (int)sa_len);
}

int klb_socket_udp_send(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
    assert(NULL != p_socket);
    assert(NULL != p_data);
    assert(0 <= len);

    if (0 == p_socket->connected)
    {
        return KLB_NETCODE_ENOTCONN;
    }

    int ret = p_socket->vtable.cb_send(p_socket, p_data, len);
    if (0 > ret)
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }
    else if (ret != len)
    {
        return KLB_NETCODE_EMSGSIZE;
    }

    return 0;
}

int klb_socket_udp_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int cap,
    struct sockaddr* p_addr, int* p_addr_len)
{
    struct sockaddr_storage sa = { 0 };
    int sa_len = (int)sizeof(sa);

    assert(NULL != p_socket);
    assert(NULL != p_buf);
    assert(0 < cap);

    int ret = p_socket->vtable.cb_recvfrom(p_socket, p_buf, cap,
        (struct sockaddr*)&sa, &sa_len);
    if (0 > ret)
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    set_peer_klb_socket_udp(p_socket, (const struct sockaddr*)&sa, (socklen_t)sa_len);

    if (NULL != p_addr)
    {
        if (sa_len > (int)sizeof(struct sockaddr_storage))
        {
            sa_len = (int)sizeof(struct sockaddr_storage);
        }

        memcpy(p_addr, &sa, (size_t)sa_len);
    }

    if (NULL != p_addr_len)
    {
        *p_addr_len = sa_len;
    }

    return ret;
}

int klb_socket_udp_recv_judge(klb_socket_t* p_socket, int n, int cap, bool* p_truncated)
{
    assert(NULL != p_socket);

    if (NULL != p_truncated)
    {
        *p_truncated = false;
    }

    if (0 <= n)
    {
        if ((NULL != p_truncated) && (0 < cap) && (n >= cap))
        {
            *p_truncated = true;
        }

        return KLB_SOCKET_UDP_RECV_DATA;
    }
    else if (KLB_NETCODE_EAGAIN == n)
    {
        return KLB_SOCKET_UDP_RECV_AGAIN;
    }

    return KLB_SOCKET_UDP_RECV_ERROR;
}

int klb_socket_udp_set_broadcast(klb_socket_t* p_socket, bool enable)
{
    int on = enable ? 1 : 0;

    assert(NULL != p_socket);

    if (KLB_SOCKET_FAMILY_V6 == p_socket->family)
    {
        return KLB_NETCODE_EOPNOTSUPP;
    }

    if (0 != setsockopt(p_socket->fd, SOL_SOCKET, SO_BROADCAST,
        (const char*)&on, sizeof(on)))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    p_socket->bcast = (enable ? 1 : 0);

    return 0;
}

/// @brief 组播组地址 / 网卡地址解析
static int mcast_mreq_klb_socket_udp(const char* p_group, const char* p_if_ip,
    struct ip_mreq* p_mreq)
{
    assert(NULL != p_group);
    assert(NULL != p_mreq);

    memset(p_mreq, 0, sizeof(struct ip_mreq));

    if (1 != inet_pton(AF_INET, p_group, &p_mreq->imr_multiaddr))
    {
        return KLB_NETCODE_EINVAL;
    }

    if ((NULL == p_if_ip) || ('\0' == p_if_ip[0]))
    {
        p_mreq->imr_interface.s_addr = htonl(INADDR_ANY);
    }
    else if (1 != inet_pton(AF_INET, p_if_ip, &p_mreq->imr_interface))
    {
        return KLB_NETCODE_EINVAL;
    }

    return 0;
}

int klb_socket_udp_join_v4(klb_socket_t* p_socket, const char* p_group, const char* p_if_ip)
{
    struct ip_mreq mreq;

    assert(NULL != p_socket);

    int ret = mcast_mreq_klb_socket_udp(p_group, p_if_ip, &mreq);
    if (0 != ret)
    {
        return ret;
    }

    if (0 != setsockopt(p_socket->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
        (const char*)&mreq, sizeof(mreq)))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    p_socket->mcast = 1;

    return 0;
}

int klb_socket_udp_leave_v4(klb_socket_t* p_socket, const char* p_group, const char* p_if_ip)
{
    struct ip_mreq mreq;

    assert(NULL != p_socket);

    int ret = mcast_mreq_klb_socket_udp(p_group, p_if_ip, &mreq);
    if (0 != ret)
    {
        return ret;
    }

    if (0 != setsockopt(p_socket->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
        (const char*)&mreq, sizeof(mreq)))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    p_socket->mcast = 0;

    return 0;
}

/// @brief 填充 ipv6_mreq
static int mcast_mreq6_klb_socket_udp(const char* p_group, unsigned int ifindex,
    struct ipv6_mreq* p_mreq)
{
    assert(NULL != p_group);
    assert(NULL != p_mreq);

    memset(p_mreq, 0, sizeof(struct ipv6_mreq));

    if (1 != inet_pton(AF_INET6, p_group, &p_mreq->ipv6mr_multiaddr))
    {
        return KLB_NETCODE_EINVAL;
    }

    p_mreq->ipv6mr_interface = ifindex;

    return 0;
}

int klb_socket_udp_join_v6(klb_socket_t* p_socket, const char* p_group, unsigned int ifindex)
{
    struct ipv6_mreq mreq;

    assert(NULL != p_socket);
    assert(NULL != p_group);

#ifndef KLB_SOCKET_UDP_IPV6_JOIN
    (void)ifindex;
    (void)mreq;
    return KLB_NETCODE_EOPNOTSUPP;
#else
    if (KLB_SOCKET_FAMILY_V6 != p_socket->family)
    {
        return KLB_NETCODE_EINVAL;
    }

    int ret = mcast_mreq6_klb_socket_udp(p_group, ifindex, &mreq);
    if (0 != ret)
    {
        return ret;
    }

    if (0 != setsockopt(p_socket->fd, IPPROTO_IPV6, KLB_SOCKET_UDP_IPV6_JOIN,
        (const char*)&mreq, sizeof(mreq)))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    p_socket->mcast = 1;

    return 0;
#endif
}

int klb_socket_udp_leave_v6(klb_socket_t* p_socket, const char* p_group, unsigned int ifindex)
{
    struct ipv6_mreq mreq;

    assert(NULL != p_socket);
    assert(NULL != p_group);

#ifndef KLB_SOCKET_UDP_IPV6_JOIN
    (void)ifindex;
    (void)mreq;
    return KLB_NETCODE_EOPNOTSUPP;
#else
    if (KLB_SOCKET_FAMILY_V6 != p_socket->family)
    {
        return KLB_NETCODE_EINVAL;
    }

    int ret = mcast_mreq6_klb_socket_udp(p_group, ifindex, &mreq);
    if (0 != ret)
    {
        return ret;
    }

    if (0 != setsockopt(p_socket->fd, IPPROTO_IPV6, KLB_SOCKET_UDP_IPV6_LEAVE,
        (const char*)&mreq, sizeof(mreq)))
    {
        return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
    }

    p_socket->mcast = 0;

    return 0;
#endif
}

int klb_socket_udp_set_mcast_ttl(klb_socket_t* p_socket, int ttl)
{
    int val = ttl;

    assert(NULL != p_socket);

    if ((0 > ttl) || (255 < ttl))
    {
        return KLB_NETCODE_EINVAL;
    }

    if (KLB_SOCKET_FAMILY_V6 == p_socket->family)
    {
#ifdef IPV6_MULTICAST_HOPS
        if (0 != setsockopt(p_socket->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
            (const char*)&val, sizeof(val)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
#else
        return KLB_NETCODE_EOPNOTSUPP;
#endif
    }
    else if (KLB_SOCKET_FAMILY_V4 == p_socket->family)
    {
        if (0 != setsockopt(p_socket->fd, IPPROTO_IP, IP_MULTICAST_TTL,
            (const char*)&val, sizeof(val)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
    }

    return KLB_NETCODE_EINVAL;
}

int klb_socket_udp_set_mcast_loop(klb_socket_t* p_socket, bool enable)
{
    int val = enable ? 1 : 0;

    assert(NULL != p_socket);

    if (KLB_SOCKET_FAMILY_V6 == p_socket->family)
    {
#ifdef IPV6_MULTICAST_LOOP
        if (0 != setsockopt(p_socket->fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
            (const char*)&val, sizeof(val)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
#else
        return KLB_NETCODE_EOPNOTSUPP;
#endif
    }
    else if (KLB_SOCKET_FAMILY_V4 == p_socket->family)
    {
        if (0 != setsockopt(p_socket->fd, IPPROTO_IP, IP_MULTICAST_LOOP,
            (const char*)&val, sizeof(val)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
    }

    return KLB_NETCODE_EINVAL;
}

int klb_socket_udp_set_mcast_if(klb_socket_t* p_socket, const char* p_if)
{
    assert(NULL != p_socket);

    if (KLB_SOCKET_FAMILY_V4 == p_socket->family)
    {
        struct in_addr if_addr;

        memset(&if_addr, 0, sizeof(if_addr));

        if ((NULL == p_if) || ('\0' == p_if[0]))
        {
            if_addr.s_addr = htonl(INADDR_ANY);
        }
        else if (1 != inet_pton(AF_INET, p_if, &if_addr))
        {
            return KLB_NETCODE_EINVAL;
        }

        if (0 != setsockopt(p_socket->fd, IPPROTO_IP, IP_MULTICAST_IF,
            (const char*)&if_addr, sizeof(if_addr)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
    }
    else if (KLB_SOCKET_FAMILY_V6 == p_socket->family)
    {
#ifdef IPV6_MULTICAST_IF
        unsigned int ifindex = 0;
        char* p_end = NULL;

        if ((NULL != p_if) && ('\0' != p_if[0]))
        {
            unsigned long parsed = strtoul(p_if, &p_end, 10);

            if ((NULL == p_end) || ('\0' != *p_end) || (0xFFFFFFFFUL < parsed))
            {
                return KLB_NETCODE_EINVAL;
            }

            ifindex = (unsigned int)parsed;
        }

        if (0 != setsockopt(p_socket->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
            (const char*)&ifindex, sizeof(ifindex)))
        {
            return netcode_from_err_klb_socket_udp(last_err_klb_socket_udp());
        }

        return 0;
#else
        (void)p_if;
        return KLB_NETCODE_EOPNOTSUPP;
#endif
    }

    return KLB_NETCODE_EINVAL;
}

// end
