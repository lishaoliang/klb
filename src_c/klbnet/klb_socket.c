// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_socket.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif


static int s_klb_socket_count = 0;


int klb_socket_init()
{
    if (0 < s_klb_socket_count)
    {
        s_klb_socket_count += 1;

        return 0;
    }

#ifdef _WIN32
    // windows上需要对WS2_32.DLL库进行初始化
    WSADATA wsa;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsa))
    {
        return 1;
    }
#elif defined(__EMSCRIPTEN__)

#else
    // recv函数在等待协议接收数据时网络断开了, 那么调用 recv 的进程会接收到一个SIGPIPE信号
    // 防止进程终止
    signal(SIGPIPE, SIG_IGN);
#endif

    s_klb_socket_count += 1;
    return 0;
}

void klb_socket_quit()
{
    if (0 < s_klb_socket_count)
    {
        s_klb_socket_count -= 1;

#ifdef _WIN32
        if (0 == s_klb_socket_count)
        {
            //windows上需要清除WS2_32.DLL库
            WSACleanup();
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////

void klb_socket_destroy(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    klb_socket_destroy_cb cb_destroy = p_socket->vtable.cb_destroy;
    if (cb_destroy)
    {
        cb_destroy(p_socket);
    }
}

void klb_socket_closing(klb_socket_t* p_socket)
{
    // 主动关闭状态
    if (KLB_SOCKET_OK == p_socket->status)
    {
        p_socket->status = KLB_SOCKET_CLOSEING;
    }

    klb_socket_set_reading(p_socket, false);    // 关闭读
    klb_socket_set_writing(p_socket, false);    // 关闭写
}

int klb_socket_send(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
    assert(NULL != p_socket);
    assert(NULL != p_socket->vtable.cb_send);

    return p_socket->vtable.cb_send(p_socket, p_data, len);
}

int klb_socket_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_socket->vtable.cb_recv);
    return p_socket->vtable.cb_recv(p_socket, p_buf, buf_len);
}

int klb_socket_recv_peek(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_socket->vtable.cb_recv_peek);
    return p_socket->vtable.cb_recv_peek(p_socket, p_buf, buf_len);
}

//////////////////////////////////////////////////////////////////////////

klb_socket_fd klb_socket_get_fd(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    return p_socket->fd;
}

void klb_socket_attach_fd(klb_socket_t* p_socket, klb_socket_fd fd)
{
    assert(NULL != p_socket);
    assert(INVALID_SOCKET == p_socket->fd);
    p_socket->fd = fd;
}

klb_socket_fd klb_socket_detach_fd(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    klb_socket_fd fd = p_socket->fd;
    p_socket->fd = INVALID_SOCKET;
    return fd;
}

/// @brief 设置是否首次次连接上
void klb_socket_set_connected(klb_socket_t* p_socket, bool connected)
{
    if (connected)
    {
        p_socket->connected = 0x1;
    }
    else
    {
        p_socket->connected = 0x0;
    }
}

/// @brief 获取是否首次次连接上
bool klb_socket_is_connected(klb_socket_t* p_socket)
{
    return (0x0 == p_socket->connected) ? false : true;
}

void klb_socket_set_writing(klb_socket_t* p_socket, bool sending)
{
    if (sending)
    {
        p_socket->writing = 0x1;
    }
    else
    {
        p_socket->writing = 0x0;
    }
}

bool klb_socket_is_writing(klb_socket_t* p_socket)
{
    return (0x0 == p_socket->writing) ? false : true;
}

void klb_socket_set_reading(klb_socket_t* p_socket, bool reading)
{
    if (reading)
    {
        p_socket->reading = 0x1;
    }
    else
    {
        p_socket->reading = 0x0;
    }
}

bool klb_socket_is_reading(klb_socket_t* p_socket)
{
    return (0x0 == p_socket->reading) ? false : true;
}

void klb_socket_set_tls(klb_socket_t* p_socket, bool tls)
{
    p_socket->tls = tls ? 0x1 : 0x0;
}

bool klb_socket_is_tls(klb_socket_t* p_socket)
{
    return (0x0 == p_socket->tls) ? false : true;
}

/// @brief 设置状态
void klb_socket_set_status(klb_socket_t* p_socket, int status)
{
    p_socket->status = status;
}

/// @brief 获取状态
uint16_t  klb_socket_get_status(klb_socket_t* p_socket)
{
    return p_socket->status;
}


/// @brief 非 TLS: recv 返回 <0 时是否为可等待错 (EAGAIN/EINTR 等)
static bool is_recv_wait_err_klb_socket(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();

    if ((WSAEWOULDBLOCK == err) || (WSAEINTR == err))
    {
        return true;
    }
#else
    if ((EAGAIN == errno) || (EWOULDBLOCK == errno) || (EINTR == errno))
    {
        return true;
    }
#endif

    return false;
}


/// @brief 非 TLS: 将系统 errno/WSA 映射为 klb_socket_status_e
static int map_recv_syserr_status_klb_socket(void)
{
#ifdef _WIN32
    int err = WSAGetLastError();

    if (WSAECONNRESET == err)
    {
        return KLB_SOCKET_ERR_RESET;
    }

    if (WSAECONNABORTED == err)
    {
        return KLB_SOCKET_ERR_ABORT;
    }

    if (WSAENOTCONN == err)
    {
        return KLB_SOCKET_DISCONNECT;
    }

    if (WSAETIMEDOUT == err)
    {
        return KLB_SOCKET_TIMEOUT;
    }
#else
    if (ECONNRESET == errno)
    {
        return KLB_SOCKET_ERR_RESET;
    }

    if (ECONNABORTED == errno)
    {
        return KLB_SOCKET_ERR_ABORT;
    }

    if (EPIPE == errno)
    {
        return KLB_SOCKET_ERR_PIPE;
    }

    if (ENOTCONN == errno)
    {
        return KLB_SOCKET_DISCONNECT;
    }

    if (ETIMEDOUT == errno)
    {
        return KLB_SOCKET_TIMEOUT;
    }
#endif

    return KLB_SOCKET_DISCONNECT;
}


/// @brief 读结束判定 (select 可读后, 对单次 recv 返回值)
/// @note 调用方须 while 排空; EOF/ERROR 后须 klb_socket_recv_closed
int klb_socket_recv_judge(klb_socket_t* p_socket, int n, int* p_status)
{
    int status = KLB_SOCKET_DISCONNECT;

    assert(NULL != p_socket);

    // step1. n>0: 有数据; 调用方入缓冲并 continue while 排空
    if (0 < n)
    {
        return KLB_SOCKET_RECV_DATA;
    }

    // step2. TLS 分支: WANT 常映射为返回 0, 须先看 status_rw
    if (klb_socket_is_tls(p_socket))
    {
        // step2.1 WANT_*: 暂不可读完, 连接未关
        if ((KLB_SOCKET_WAIT_READ == p_socket->status_rw)
            || (KLB_SOCKET_WAIT_WRITE == p_socket->status_rw))
        {
            return KLB_SOCKET_RECV_AGAIN;
        }

        // step2.2 RW_OK 且 n==0: 有序关 (FIN 或 PEER_CLOSE_NOTIFY 现行均映射为 0)
        if (0 == n)
        {
            status = KLB_SOCKET_CLOSED;
            if (NULL != p_status)
            {
                *p_status = status;
            }
            return KLB_SOCKET_RECV_EOF;
        }

        // step2.3 n<0: TLS 致命错; 优先沿用已写入 status
        status = (int)klb_socket_get_status(p_socket);
        if ((KLB_SOCKET_OK == status) || (KLB_SOCKET_CLOSEING == status))
        {
            status = KLB_SOCKET_ERR_PROTOCOL;
        }

        if (NULL != p_status)
        {
            *p_status = status;
        }
        return KLB_SOCKET_RECV_ERROR;
    }

    // step3. 非 TLS: 0=有序关, EAGAIN=缓存读完仍存活 (二者不混淆)
    // step3.1 n==0: 读就绪路径上对端 FIN, 不是 "缓冲读完"
    if (0 == n)
    {
        status = KLB_SOCKET_CLOSED;
        if (NULL != p_status)
        {
            *p_status = status;
        }
        return KLB_SOCKET_RECV_EOF;
    }

    // step3.2 n<0 且 EAGAIN/EINTR: 内核读缓存已空, 连接仍在
    if (is_recv_wait_err_klb_socket())
    {
        return KLB_SOCKET_RECV_AGAIN;
    }

    // step3.3 n<0 其它: RST/ABORT 等异常关
    status = map_recv_syserr_status_klb_socket();
    if (NULL != p_status)
    {
        *p_status = status;
    }
    return KLB_SOCKET_RECV_ERROR;
}


void klb_socket_recv_closed(klb_socket_t* p_socket, int status)
{
    assert(NULL != p_socket);

    if (KLB_SOCKET_OK == klb_socket_get_status(p_socket))
    {
        klb_socket_set_status(p_socket, status);
    }

    klb_socket_set_reading(p_socket, false);
    klb_socket_set_writing(p_socket, false);
}


/// @brief 写结束判定 (select 可写后, 对单次 send 返回值)
/// @note 与 klb_socket_recv_judge 对称; 调用方须 while 排空直到 AGAIN/ERROR
/// @note 写侧无 "send==0 = 有序关"; 有序关看读侧 FIN; 写致命多为 EPIPE/RST
int klb_socket_send_judge(klb_socket_t* p_socket, int n, int* p_status)
{
    int status = KLB_SOCKET_DISCONNECT;

    assert(NULL != p_socket);

    // step1. n>0: 本轮已写出若干字节
    //   调用方: 推进缓冲偏移, continue while, 直到发完或不能再写
    if (0 < n)
    {
        return KLB_SOCKET_SEND_DATA;
    }

    // step2. TLS 分支 (p_socket->tls)
    //   mbedtls 常把 WANT_READ/WANT_WRITE 映射为 send 返回 0, 须先看 status_rw
    //   禁止: 凡 n==0 即当关闭 (会与 WANT 冲突)
    if (klb_socket_is_tls(p_socket))
    {
        // step2.1 WANT_*: 握手或记录层暂不可完成写, 连接未关
        //   apply_want 已可能 set_reading / set_writing; 调用方保持队列, 等下次就绪
        if ((KLB_SOCKET_WAIT_READ == p_socket->status_rw)
            || (KLB_SOCKET_WAIT_WRITE == p_socket->status_rw))
        {
            return KLB_SOCKET_SEND_AGAIN;
        }

        // step2.2 RW_OK 且 n==0: 握手推进中 / 空写 / 暂无可出
        //   不当有序关 (有序关在读路径); 本轮停等即可
        if (0 == n)
        {
            return KLB_SOCKET_SEND_AGAIN;
        }

        // step2.3 n<0: TLS 层致命错 (协议/底层)
        //   优先沿用 TLS 已写入的 status; 仍为 OK 则落 ERR_PROTOCOL
        //   调用方须: klb_socket_recv_closed + push_netcode / 唤醒上层
        status = (int)klb_socket_get_status(p_socket);
        if ((KLB_SOCKET_OK == status) || (KLB_SOCKET_CLOSEING == status))
        {
            status = KLB_SOCKET_ERR_PROTOCOL;
        }

        if (NULL != p_status)
        {
            *p_status = status;
        }
        return KLB_SOCKET_SEND_ERROR;
    }

    // step3. 非 TLS (裸 TCP)
    // step3.1 n==0: POSIX 上少见; 不当对端有序关 (那是 recv==0)
    //   语义: 本轮不能再写出, 停等下次可写 (AGAIN)
    if (0 == n)
    {
        return KLB_SOCKET_SEND_AGAIN;
    }

    // step3.2 n<0 且 EAGAIN/EWOULDBLOCK/EINTR: 发送缓存满, 连接仍在
    //   调用方: break while, 保持 writing=1, 等 select 再可写
    if (is_recv_wait_err_klb_socket())
    {
        return KLB_SOCKET_SEND_AGAIN;
    }

    // step3.3 n<0 且其它 errno: 异常关闭
    //   常见: EPIPE (写时对端已关), ECONNRESET (RST), ECONNABORTED, ENOTCONN
    //   映射见 map_recv_syserr_status_klb_socket; 调用方须 recv_closed + 通知
    status = map_recv_syserr_status_klb_socket();
    if (NULL != p_status)
    {
        *p_status = status;
    }
    return KLB_SOCKET_SEND_ERROR;
}


/// @brief 设置 user data
void klb_socket_set_udata(klb_socket_t* p_socket, void* p_udata)
{
    p_socket->p_udata = p_udata;
}

/// @brief 获取 user data
void* klb_socket_get_udata(klb_socket_t* p_socket)
{
    return p_socket->p_udata;
}

//////////////////////////////////////////////////////////////////////////
void klb_socket_close(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

    if (INVALID_SOCKET != fd)
    {
#ifdef _WIN32
        int ret = closesocket(fd);
#else
        int ret = close(fd);
#endif
        assert(0 == ret);
    }
}

int klb_socket_set_reuseaddr(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

    int opt = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(int));

    assert(0 == ret);
    return ret;
}

int klb_socket_set_reuseport(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

#ifdef SO_REUSEPORT
    {
        int opt = 1;
        return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(int));
    }
#else
    (void)fd;
    return -1;
#endif
}

int klb_socket_set_tcp_nodelay(klb_socket_fd fd, bool enable)
{
    int opt = enable ? 1 : 0;

    assert(INVALID_SOCKET != fd);

    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(int));
}

int klb_socket_set_keepalive(klb_socket_fd fd, bool enable)
{
    int opt = enable ? 1 : 0;

    assert(INVALID_SOCKET != fd);

    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&opt, sizeof(int));
}

int klb_socket_set_ipv6only(klb_socket_fd fd, bool v6only)
{
    int opt = v6only ? 1 : 0;

    assert(INVALID_SOCKET != fd);

#ifdef IPV6_V6ONLY
    return setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&opt, sizeof(int));
#else
    (void)fd;
    (void)opt;
    return -1;
#endif
}

void klb_socket_set_block(klb_socket_fd fd, bool blocking)
{
    assert(INVALID_SOCKET != fd);

#ifdef _WIN32
    u_long value = blocking ? 0 : 1;    // 0. 阻塞模式; 非0,非阻塞模式
    ioctlsocket(fd, FIONBIO, &value);
#else
    int flags = fcntl(fd, F_GETFL);

    if (blocking)
    {
        flags &= ~(unsigned int)(O_NONBLOCK);
    }
    else
    {
        flags |= O_NONBLOCK;
    }

    fcntl(fd, F_SETFL, flags);
#endif
}


static int connect_check_wait(klb_socket_fd fd, int time_out)
{
    assert(INVALID_SOCKET != fd);
    assert(0 < time_out);

    // 检查socket是否连接上
    int ret = -1;

    fd_set  w_fds;
    FD_ZERO(&w_fds);
    FD_SET(fd, &w_fds);

    struct timeval tv;
    tv.tv_sec = time_out / 1000;  //取秒
    tv.tv_usec = (time_out % 1000) * 1000; //取微妙
    ret = select(fd + 1, NULL, &w_fds, NULL, &tv);

    if (ret <= 0) //异常或者超时
    {
        return -1;
    }
    else
    {
        if (FD_ISSET(fd, &w_fds))
        {
            int error = 0;
            socklen_t error_len = sizeof(error);
            if (0 != getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&error, &error_len) < 0)
            {
                return -1;
            }

            if (0 != error)
            {
                return -1;
            }

            return 0;
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

static klb_socket_fd connect_tcp(const struct sockaddr* p_addr, socklen_t addr_len, int time_out)
{
    klb_socket_fd fd = socket(p_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    // 端口释放后立即就可以被再次使用
    if (0 != klb_socket_set_reuseaddr(fd))
    {
        goto err_connect;
    }

    // 先设置为非阻塞方式连接服务器,防止在 connect 中耗时, 不可控制
    // 连接完毕后, 等待耗时 time_out
    // 最后将 0 <= time_out 的连接还原为阻塞方式
    klb_socket_set_block(fd, false);

    if (0 != connect(fd, p_addr, addr_len))
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (WSAEISCONN == err)
        {
            WSASetLastError(err); // 成功
        }
        else if ((WSAEWOULDBLOCK != err) && (WSAEALREADY != err) && (WSAEINVAL != err))
        {
            //多判断WSAEINVAL的理由如下:
            //Note In order to preserve backward compatibility, this error is reported as WSAEINVAL to 
            //Windows Sockets 1.1 applications that link to either Winsock.dll or Wsock32.dll.
            WSASetLastError(err); //失败
            goto err_connect;
        }
#else
        if (EISCONN == errno)
        {
            // 成功
        }
        else if ((EINPROGRESS != errno) && (EWOULDBLOCK != errno) && (EAGAIN != errno) && (EALREADY != errno))
        {
            goto err_connect;
        }
#endif
    }

    if (0 < time_out)
    {
        if (0 != connect_check_wait(fd, time_out))
        {
            goto err_connect;
        }

        klb_socket_set_block(fd, true);
    }

    return fd;

err_connect:
    KLB_SOCKET_CLOSE(fd);
    return INVALID_SOCKET;
}

klb_socket_fd klb_socket_connect(const char* p_host, int port, int time_out)
{
    assert(NULL != p_host);

    klb_socket_fd fd = INVALID_SOCKET;
    struct addrinfo* p_result = NULL;

    char str_port[16] = { 0 };
    snprintf(str_port, sizeof(str_port), "%d", port); // 服务端口
    str_port[sizeof(str_port) - 1] = '\0';

    int ret = getaddrinfo(p_host, str_port, NULL, &p_result);
    if (0 == ret)
    {
        struct addrinfo* p_next = p_result;
        while (NULL != p_next)
        {
            fd = connect_tcp(p_next->ai_addr, (socklen_t)p_next->ai_addrlen, time_out);
            if (INVALID_SOCKET != fd)
            {
                break;
            }

            p_next = p_next->ai_next;
        }
    }
    else
    {
        const char* p_err = gai_strerror(ret);
        //KLB_LOG("socket connect:[%s:%d] error!gai_strerror:%s\n", p_host, port, p_err);
    }

    KLB_FREE_BY(p_result, freeaddrinfo);
    return fd;
}

klb_socket_fd klb_socket_listen(int port, int max_connect)
{
    struct sockaddr_in addr = { 0 };

    // INADDR_ANY表示占用该设备所有网卡, 不需要知道设备IP地址
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    return klb_socket_listen_addr(&addr, max_connect);
}

klb_socket_fd klb_socket_listen_addr(const struct sockaddr_in* p_addr, int max_connect)
{
    struct sockaddr_storage sa = { 0 };

    assert(NULL != p_addr);

    memcpy(&sa, p_addr, sizeof(struct sockaddr_in));
    return klb_socket_listen_sa(&sa, (socklen_t)sizeof(struct sockaddr_in), max_connect, false, true);
}

klb_socket_fd klb_socket_listen_sa(const struct sockaddr_storage* p_addr, socklen_t addr_len,
    int max_connect, bool reuseport, bool ipv6only)
{
    klb_socket_fd fd = INVALID_SOCKET;
    int family = AF_INET;

    assert(NULL != p_addr);
    assert(0 < addr_len);

    family = p_addr->ss_family;
    if (AF_INET != family && AF_INET6 != family)
    {
        return INVALID_SOCKET;
    }

    fd = socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    // 端口释放后立即就可以被再次使用
    if (0 != klb_socket_set_reuseaddr(fd))
    {
        goto err_listen;
    }

    if (reuseport)
    {
        if (0 != klb_socket_set_reuseport(fd))
        {
            goto err_listen;
        }
    }

    if (AF_INET6 == family)
    {
        if (0 != klb_socket_set_ipv6only(fd, ipv6only))
        {
            // 部分平台无 IPV6_V6ONLY: 仅 dual(false) 时视为失败; v6only=true 可继续
            if (!ipv6only)
            {
                goto err_listen;
            }
        }
    }

    // 非阻塞
    klb_socket_set_block(fd, false);

    // 绑定
    if (0 != bind(fd, (const struct sockaddr*)p_addr, addr_len))
    {
        goto err_listen;
    }

    // 监听
    if (0 != listen(fd, max_connect))
    {
        goto err_listen;
    }

    return fd;

err_listen:
    KLB_SOCKET_CLOSE(fd);
    return INVALID_SOCKET;
}

int klb_socket_resolve_v4(const char* p_host, int port, struct sockaddr_in* p_addr)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(NULL != p_addr);

    if (0 != klb_socket_resolve_sa(p_host, port, AF_INET, &sa, &sa_len))
    {
        return -1;
    }

    if (AF_INET != sa.ss_family || sa_len < (socklen_t)sizeof(struct sockaddr_in))
    {
        return -1;
    }

    memcpy(p_addr, &sa, sizeof(struct sockaddr_in));
    return 0;
}

int klb_socket_resolve_sa(const char* p_host, int port, int family,
    struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    bool any = false;
    bool is_v6_any = false;
    int want_family = family;

    assert(NULL != p_addr);
    assert(NULL != p_addr_len);

    memset(p_addr, 0, sizeof(struct sockaddr_storage));
    *p_addr_len = 0;

    if (AF_INET != want_family && AF_INET6 != want_family && AF_UNSPEC != want_family)
    {
        return -1;
    }

    any = (NULL == p_host || '\0' == p_host[0]
        || 0 == strcmp(p_host, "*") || 0 == strcmp(p_host, "0.0.0.0"));
    is_v6_any = (NULL != p_host && 0 == strcmp(p_host, "::"));

    // 通配: dual/UNSPEC → AF_INET6; 显式 ipv4 → AF_INET; "::" → AF_INET6
    if (any || is_v6_any)
    {
        if (is_v6_any || AF_INET6 == want_family || AF_UNSPEC == want_family)
        {
            struct sockaddr_in6* p_v6 = (struct sockaddr_in6*)p_addr;

            p_v6->sin6_family = AF_INET6;
            p_v6->sin6_port = htons((uint16_t)port);
            p_v6->sin6_addr = in6addr_any;
            *p_addr_len = (socklen_t)sizeof(struct sockaddr_in6);
            return 0;
        }

        {
            struct sockaddr_in* p_v4 = (struct sockaddr_in*)p_addr;

            p_v4->sin_family = AF_INET;
            p_v4->sin_port = htons((uint16_t)port);
            p_v4->sin_addr.s_addr = htonl(INADDR_ANY);
            *p_addr_len = (socklen_t)sizeof(struct sockaddr_in);
            return 0;
        }
    }

    // 含 ':' 的字面量优先按 IPv6 (即使 family 为 AF_INET 也改走 v6)
    if (NULL != strchr(p_host, ':') && AF_INET == want_family)
    {
        want_family = AF_INET6;
    }

    {
        struct addrinfo hints;
        struct addrinfo* p_result = NULL;
        struct addrinfo* p_next = NULL;
        char str_port[16] = { 0 };
        bool found = false;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = want_family;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        snprintf(str_port, sizeof(str_port), "%d", port);
        str_port[sizeof(str_port) - 1] = '\0';

        if (0 != getaddrinfo(p_host, str_port, &hints, &p_result))
        {
            return -1;
        }

        p_next = p_result;
        while (NULL != p_next)
        {
            if ((AF_INET == p_next->ai_family || AF_INET6 == p_next->ai_family)
                && NULL != p_next->ai_addr
                && 0 < p_next->ai_addrlen
                && p_next->ai_addrlen <= (socklen_t)sizeof(struct sockaddr_storage))
            {
                memcpy(p_addr, p_next->ai_addr, p_next->ai_addrlen);
                *p_addr_len = (socklen_t)p_next->ai_addrlen;
                found = true;
                break;
            }

            p_next = p_next->ai_next;
        }

        freeaddrinfo(p_result);

        if (!found)
        {
            return -1;
        }
    }

    return 0;
}

int klb_socket_get_local_addr(klb_socket_fd fd, struct sockaddr_in* p_addr)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;

    assert(INVALID_SOCKET != fd);
    assert(NULL != p_addr);

    if (0 != klb_socket_get_local_sa(fd, &sa, &sa_len))
    {
        return -1;
    }

    if (AF_INET != sa.ss_family)
    {
        return -1;
    }

    memcpy(p_addr, &sa, sizeof(struct sockaddr_in));
    return 0;
}

int klb_socket_get_local_sa(klb_socket_fd fd, struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    socklen_t addr_size = (socklen_t)sizeof(struct sockaddr_storage);

    assert(INVALID_SOCKET != fd);
    assert(NULL != p_addr);

    memset(p_addr, 0, sizeof(struct sockaddr_storage));

    if (0 != getsockname(fd, (struct sockaddr*)p_addr, &addr_size))
    {
        return -1;
    }

    if (NULL != p_addr_len)
    {
        *p_addr_len = addr_size;
    }

    return 0;
}

int klb_socket_get_peer_sa(klb_socket_fd fd, struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    socklen_t addr_size = (socklen_t)sizeof(struct sockaddr_storage);

    assert(INVALID_SOCKET != fd);
    assert(NULL != p_addr);

    memset(p_addr, 0, sizeof(struct sockaddr_storage));

    if (0 != getpeername(fd, (struct sockaddr*)p_addr, &addr_size))
    {
        return -1;
    }

    if (NULL != p_addr_len)
    {
        *p_addr_len = addr_size;
    }

    return 0;
}

int klb_socket_format_addr(const struct sockaddr_storage* p_addr, char* p_ip, int ip_cap, int* p_port)
{
    assert(NULL != p_addr);
    assert(NULL != p_ip);
    assert(0 < ip_cap);

    p_ip[0] = '\0';

    if (AF_INET == p_addr->ss_family)
    {
        const struct sockaddr_in* p_v4 = (const struct sockaddr_in*)p_addr;
        const unsigned char* p_octet = (const unsigned char*)&p_v4->sin_addr.s_addr;

        if (16 > ip_cap)
        {
            return -1;
        }

        snprintf(p_ip, (size_t)ip_cap, "%u.%u.%u.%u",
            (unsigned)p_octet[0], (unsigned)p_octet[1], (unsigned)p_octet[2], (unsigned)p_octet[3]);
        p_ip[ip_cap - 1] = '\0';

        if (NULL != p_port)
        {
            *p_port = (int)ntohs(p_v4->sin_port);
        }

        return 0;
    }
    else if (AF_INET6 == p_addr->ss_family)
    {
        const struct sockaddr_in6* p_v6 = (const struct sockaddr_in6*)p_addr;

        if (NULL == inet_ntop(AF_INET6, &p_v6->sin6_addr, p_ip, (socklen_t)ip_cap))
        {
            return -1;
        }

        if (NULL != p_port)
        {
            *p_port = (int)ntohs(p_v6->sin6_port);
        }

        return 0;
    }

    return -1;
}

int klb_socket_format_addr_t(const klb_socket_addr_t* p_addr, char* p_ip, int ip_cap, int* p_port)
{
    struct sockaddr_storage sa = { 0 };

    assert(NULL != p_addr);
    assert(NULL != p_ip);

    if ((0 >= p_addr->len) || ((socklen_t)sizeof(sa) < p_addr->len))
    {
        return -1;
    }

    memcpy(&sa, &p_addr->addr, p_addr->len);

    return klb_socket_format_addr(&sa, p_ip, ip_cap, p_port);
}

int klb_socket_af_to_family(int af)
{
    if (AF_INET == af)
    {
        return KLB_SOCKET_FAMILY_V4;
    }
    else if (AF_INET6 == af)
    {
        return KLB_SOCKET_FAMILY_V6;
    }

    return KLB_SOCKET_FAMILY_UNSPEC;
}

int klb_socket_family_to_af(int family)
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

klb_socket_fd klb_socket_listen_unix(const char* p_path, int max_connect)
{
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    // Fixed Bug. [2026] Unix 域须 AF_UNIX + sun_family=AF_UNIX, 非 AF_INET
    klb_socket_fd fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    struct sockaddr_un addr = { 0 };
    int ret = 0;

    // 端口释放后立即就可以被再次使用
    if (0 != klb_socket_set_reuseaddr(fd))
    {
        goto err_listen;
    }

    // 非阻塞
    klb_socket_set_block(fd, false);

    // 绑定 
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, p_path, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(p_path);

    ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (0 != ret)
    {
        goto err_listen;
    }

    // 监听
    ret = listen(fd, max_connect);
    if (0 != ret)
    {
        goto err_listen;
    }

    return fd; // 监听成功

err_listen:
    KLB_SOCKET_CLOSE(fd);
    return INVALID_SOCKET; // 失败
#endif
}

klb_socket_fd klb_socket_connect_unix(const char* p_path, int time_out)
{
#ifdef _WIN32
    (void)p_path;
    (void)time_out;
    return INVALID_SOCKET;
#else
    klb_socket_fd fd = INVALID_SOCKET;
    struct sockaddr_un addr = { 0 };

    assert(NULL != p_path);
    if ('\0' == p_path[0])
    {
        return INVALID_SOCKET;
    }

    // step1. AF_UNIX + SOCK_STREAM
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    klb_socket_set_block(fd, false);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, p_path, sizeof(addr.sun_path) - 1u);
    addr.sun_path[sizeof(addr.sun_path) - 1u] = '\0';

    // step2. 非阻塞 connect + 可选等待
    if (0 != connect(fd, (struct sockaddr*)&addr, sizeof(addr)))
    {
        if (EISCONN == errno)
        {
            // connected
        }
        else if ((EINPROGRESS != errno) && (EWOULDBLOCK != errno) && (EAGAIN != errno) && (EALREADY != errno))
        {
            goto err_connect_unix;
        }
    }

    if (0 < time_out)
    {
        if (0 != connect_check_wait(fd, time_out))
        {
            goto err_connect_unix;
        }

        klb_socket_set_block(fd, true);
    }

    return fd;

err_connect_unix:
    KLB_SOCKET_CLOSE(fd);
    return INVALID_SOCKET;
#endif
}

klb_socket_fd klb_socket_accept(klb_socket_fd fd_listen, struct sockaddr_in* p_addr)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;
    klb_socket_fd fd = INVALID_SOCKET;

    assert(INVALID_SOCKET != fd_listen);

    fd = klb_socket_accept_sa(fd_listen, &sa, &sa_len);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    if (NULL != p_addr)
    {
        memset(p_addr, 0, sizeof(struct sockaddr_in));
        if (AF_INET == sa.ss_family)
        {
            memcpy(p_addr, &sa, sizeof(struct sockaddr_in));
        }
#ifdef IN6_IS_ADDR_V4MAPPED
        else if (AF_INET6 == sa.ss_family)
        {
            const struct sockaddr_in6* p_v6 = (const struct sockaddr_in6*)&sa;

            if (IN6_IS_ADDR_V4MAPPED(&p_v6->sin6_addr))
            {
                p_addr->sin_family = AF_INET;
                p_addr->sin_port = p_v6->sin6_port;
                memcpy(&p_addr->sin_addr, &p_v6->sin6_addr.s6_addr[12], 4);
            }
        }
#endif
    }

    return fd;
}

klb_socket_fd klb_socket_accept_sa(klb_socket_fd fd_listen, struct sockaddr_storage* p_addr, socklen_t* p_addr_len)
{
    socklen_t addr_size = (socklen_t)sizeof(struct sockaddr_storage);
    klb_socket_fd fd = INVALID_SOCKET;

    assert(INVALID_SOCKET != fd_listen);

    if (NULL != p_addr)
    {
        memset(p_addr, 0, sizeof(struct sockaddr_storage));
        fd = accept(fd_listen, (struct sockaddr*)p_addr, &addr_size);
    }
    else
    {
        fd = accept(fd_listen, NULL, NULL);
        addr_size = 0;
    }

    if (INVALID_SOCKET != fd)
    {
        klb_socket_set_reuseaddr(fd);
    }

    if (NULL != p_addr_len)
    {
        *p_addr_len = addr_size;
    }

    return fd;
}

//////////////////////////////////////////////////////////////////////////
// 异步 socket

/// @brief 销毁
static void klb_socket_async_destroy(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    // 关闭fd
    KLB_SOCKET_CLOSE(p_socket->fd);

    // 销毁
    KLB_FREE(p_socket);
}

/// @brief TCP发送数据
static int klb_socket_async_send(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
#ifdef _WIN32
    int ret = send(p_socket->fd, (const char*)p_data, len, 0);
#else
#ifndef __APPLE__
    int flags = p_socket->nonblock ? (MSG_DONTWAIT | MSG_NOSIGNAL) : 0;
    int ret = send(p_socket->fd, (const char*)p_data, len, flags);
#else
    int flags = p_socket->nonblock ? MSG_DONTWAIT : 0;
    int ret = send(p_socket->fd, (const char*)p_data, len, flags);
#endif
#endif

    return ret;
}

/// @brief TCP接收数据
static int klb_socket_async_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, 0);
#else
    int flags = p_socket->nonblock ? MSG_DONTWAIT : 0;
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, flags);
#endif

    return ret;
}

/// @brief TCP窥探接收数据
static int klb_socket_async_recv_peek(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, MSG_PEEK);
#else
    int flags = MSG_PEEK;
    if (0 != p_socket->nonblock)
    {
        flags |= MSG_DONTWAIT;
    }
    int ret = recv(p_socket->fd, (char*)p_buf, buf_len, flags);
#endif

    return ret;
}

/// @brief UDP发送数据
static int klb_socket_async_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
{
#ifdef _WIN32
    int ret = sendto(p_socket->fd, (const char*)p_data, len, 0, p_addr, addr_len);
#else
#ifndef __APPLE__
    int flags = p_socket->nonblock ? (MSG_DONTWAIT | MSG_NOSIGNAL) : 0;
    int ret = sendto(p_socket->fd, (const char*)p_data, len, flags, p_addr, addr_len);
#else
    int flags = p_socket->nonblock ? MSG_DONTWAIT : 0;
    int ret = sendto(p_socket->fd, (const char*)p_data, len, flags, p_addr, addr_len);
#endif
#endif

    return ret;
}

/// @brief UDP接收数据
static int klb_socket_async_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
#ifdef _WIN32
    int ret = recvfrom(p_socket->fd, (char*)p_buf, buf_len, 0, p_addr, p_addr_len);
#else
    int flags = p_socket->nonblock ? MSG_DONTWAIT : 0;

    socklen_t addr_len = *p_addr_len;
    int ret = recvfrom(p_socket->fd, (char*)p_buf, buf_len, flags, p_addr, &addr_len);

    *p_addr_len = addr_len;
#endif

    return ret;
}

/// @brief sockaddr_storage 转紧凑地址
static void addr_from_sa_klb_socket(klb_socket_addr_t* p_addr,
    const struct sockaddr_storage* p_sa, socklen_t sa_len)
{
    assert(NULL != p_addr);
    assert(NULL != p_sa);

    memset(p_addr, 0, sizeof(klb_socket_addr_t));

    if ((AF_INET == p_sa->ss_family) && ((socklen_t)sizeof(struct sockaddr_in) <= sa_len))
    {
        memcpy(&p_addr->addr, p_sa, sizeof(struct sockaddr_in));
        p_addr->len = (socklen_t)sizeof(struct sockaddr_in);
    }
    else if ((AF_INET6 == p_sa->ss_family) && ((socklen_t)sizeof(struct sockaddr_in6) <= sa_len))
    {
        memcpy(&p_addr->addr, p_sa, sizeof(struct sockaddr_in6));
        p_addr->len = (socklen_t)sizeof(struct sockaddr_in6);
    }
}

/// @brief 从 fd 回填地址族/类型/本端/对端
static void fill_from_fd_klb_socket(klb_socket_t* p_socket)
{
    struct sockaddr_storage sa = { 0 };
    socklen_t sa_len = 0;
    int sock_type = 0;
#ifdef _WIN32
    int type_len = (int)sizeof(sock_type);
#else
    socklen_t type_len = (socklen_t)sizeof(sock_type);
#endif

    assert(NULL != p_socket);

    // 地址族 / 本端
    if (0 == klb_socket_get_local_sa(p_socket->fd, &sa, &sa_len))
    {
        p_socket->family = (uint16_t)klb_socket_af_to_family((int)sa.ss_family);
        addr_from_sa_klb_socket(&p_socket->local, &sa, sa_len);
    }

    // 类型
    if (0 == getsockopt(p_socket->fd, SOL_SOCKET, SO_TYPE, (char*)&sock_type, &type_len))
    {
        if (SOCK_DGRAM == sock_type)
        {
            p_socket->type = KLB_SOCKET_TYPE_DGRAM;
        }
        else
        {
            p_socket->type = KLB_SOCKET_TYPE_STREAM;
        }
    }

    // 对端 (未连接时失败, 保持空)
    sa_len = 0;
    if (0 == klb_socket_get_peer_sa(p_socket->fd, &sa, &sa_len))
    {
        if (KLB_SOCKET_FAMILY_UNSPEC == p_socket->family)
        {
            p_socket->family = (uint16_t)klb_socket_af_to_family((int)sa.ss_family);
        }

        addr_from_sa_klb_socket(&p_socket->peer, &sa, sa_len);
    }
}

klb_socket_t* klb_socket_async_create(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = KLB_MALLOCZ(klb_socket_t, 1, 0);

    p_socket->vtable.cb_destroy = klb_socket_async_destroy;
    p_socket->vtable.cb_send = klb_socket_async_send;
    p_socket->vtable.cb_recv = klb_socket_async_recv;
    p_socket->vtable.cb_recv_peek = klb_socket_async_recv_peek;
    p_socket->vtable.cb_sendto = klb_socket_async_sendto;
    p_socket->vtable.cb_recvfrom = klb_socket_async_recvfrom;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = fd;

    // 回填地址族/类型/本端/对端
    fill_from_fd_klb_socket(p_socket);

    // 设置为非阻塞, 异步模式
    klb_socket_set_block(fd, false);

    return p_socket;
}

int klb_socket_family(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    return (int)p_socket->family;
}

int klb_socket_type(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    return (int)p_socket->type;
}

int klb_socket_local_addr(klb_socket_t* p_socket, klb_socket_addr_t* p_addr)
{
    assert(NULL != p_socket);
    assert(NULL != p_addr);

    if (0 == p_socket->local.len)
    {
        return -1;
    }

    *p_addr = p_socket->local;

    return 0;
}

int klb_socket_peer_addr(klb_socket_t* p_socket, klb_socket_addr_t* p_addr)
{
    assert(NULL != p_socket);
    assert(NULL != p_addr);

    if (0 == p_socket->peer.len)
    {
        return -1;
    }

    *p_addr = p_socket->peer;

    return 0;
}

int klb_socket_local_port(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    if (AF_INET == p_socket->local.addr.sa.sa_family)
    {
        return (int)ntohs(p_socket->local.addr.in4.sin_port);
    }
    else if (AF_INET6 == p_socket->local.addr.sa.sa_family)
    {
        return (int)ntohs(p_socket->local.addr.in6.sin6_port);
    }

    return -1;
}

int klb_socket_peer_port(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    if (AF_INET == p_socket->peer.addr.sa.sa_family)
    {
        return (int)ntohs(p_socket->peer.addr.in4.sin_port);
    }
    else if (AF_INET6 == p_socket->peer.addr.sa.sa_family)
    {
        return (int)ntohs(p_socket->peer.addr.in6.sin6_port);
    }

    return -1;
}

// end
