///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include <assert.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#endif


static int s_klb_socket_init = 0;


int klb_socket_init()
{
    if (0 < s_klb_socket_init)
    {
        s_klb_socket_init += 1;

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

    // tls
    klb_socket_tls_init();

    s_klb_socket_init += 1;
    return 0;
}

void klb_socket_quit()
{
    if (0 < s_klb_socket_init)
    {
        s_klb_socket_init -= 1;

        // tls
        klb_socket_tls_quit();

#ifdef _WIN32
        if (0 == s_klb_socket_init)
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

int klb_socket_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_socket->vtable.cb_sendto);
    return p_socket->vtable.cb_sendto(p_socket, p_data, len, p_addr, addr_len);
}

int klb_socket_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
    assert(NULL != p_socket);
    assert(NULL != p_socket->vtable.cb_recvfrom);
    return p_socket->vtable.cb_recvfrom(p_socket, p_buf, buf_len, p_addr, p_addr_len);
}

//////////////////////////////////////////////////////////////////////////

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

void klb_socket_set_sending(klb_socket_t* p_socket, bool sending)
{
    if (sending)
    {
        p_socket->sending = 0x1;
    }
    else
    {
        p_socket->sending = 0x0;
    }
}

bool klb_socket_is_sending(klb_socket_t* p_socket)
{
    return (0x0 == p_socket->sending) ? false : true;
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

static klb_socket_fd connect_tcp(const struct sockaddr_in* p_addr, int time_out)
{
    klb_socket_fd fd = socket(p_addr->sin_family/*AF_INET*/, SOCK_STREAM, IPPROTO_TCP);
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

    if (0 != connect(fd, p_addr, sizeof(struct sockaddr_in)))
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
    snprintf(str_port, 12, "%d", port); // 服务端口

    int ret = getaddrinfo(p_host, str_port, NULL, &p_result);
    if (0 == ret)
    {
        struct addrinfo* p_next = p_result;
        while (NULL != p_next)
        {
            fd = connect_tcp((struct sockaddr_in*)(p_next->ai_addr), time_out);
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
    klb_socket_fd fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    // 端口释放后立即就可以被再次使用
    if (0 != klb_socket_set_reuseaddr(fd))
    {
        goto err_listen;
    }

    // 非阻塞
    klb_socket_set_block(fd, false);

    // 绑定
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY表示占用该设备所有网卡, 不需要知道设备IP地址
    addr.sin_port = htons(port);

    int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
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
}

klb_socket_fd klb_socket_listen_unix(const char* p_path, int max_connect)
{
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    klb_socket_fd fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
    }

    // 端口释放后立即就可以被再次使用
    if (0 != klb_socket_set_reuseaddr(fd))
    {
        goto err_listen;
    }

    // 非阻塞
    klb_socket_set_block(fd, false);

    // 绑定
    struct sockaddr_un addr = { 0 };
    addr.sun_family = AF_INET;
    strncpy(addr.sun_path, p_path, sizeof(addr.sun_path) - 1);

    unlink(p_path);

    int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
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

klb_socket_fd klb_socket_accept(klb_socket_fd fd_listen, struct sockaddr_in* p_addr)
{
    assert(INVALID_SOCKET != fd_listen);

    int addr_size = sizeof(struct sockaddr_in);
    klb_socket_fd fd = accept(fd_listen, (struct sockaddr*)p_addr, &addr_size);

    if (INVALID_SOCKET != fd)
    {
        // 端口释放后立即就可以被再次使用
        klb_socket_set_reuseaddr(fd);
    }

    return fd;
}

//////////////////////////////////////////////////////////////////////////
// 异步socket

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

klb_socket_t* klb_socket_async_create(klb_socket_fd fd)
{
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = KLB_MALLOC(klb_socket_t, 1, 0);
    KLB_MEMSET(p_socket, 0, sizeof(klb_socket_t));

    p_socket->vtable.cb_destroy = klb_socket_async_destroy;
    p_socket->vtable.cb_send = klb_socket_async_send;
    p_socket->vtable.cb_recv = klb_socket_async_recv;
    p_socket->vtable.cb_sendto = klb_socket_async_sendto;
    p_socket->vtable.cb_recvfrom = klb_socket_async_recvfrom;

    p_socket->status = KLB_SOCKET_OK;
    p_socket->fd = fd;

    // 设置为非阻塞, 异步模式
    klb_socket_set_block(fd, false);

    return p_socket;
}
