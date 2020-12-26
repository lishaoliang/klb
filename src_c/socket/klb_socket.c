///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_socket.c
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   文件简要描述
///////////////////////////////////////////////////////////////////////////
#include "socket/klb_socket.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include "string/sds.h"
#include <assert.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <signal.h>
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

    s_klb_socket_init += 1;
    return 0;
}

void klb_socket_quit()
{
    if (0 < s_klb_socket_init)
    {
        s_klb_socket_init -= 1;

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

//void klb_socket_destroy(klb_socket_t* p_socket)
//{
//    assert(NULL != p_socket);
//    assert(NULL != p_socket->vtable.cb_destroy);
//
//    klb_socket_destroy_cb cb_destroy = p_socket->vtable.cb_destroy;
//    assert(NULL != cb_destroy);
//
//    cb_destroy(p_socket);
//}

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

void klb_socket_set_block(klb_socket_fd fd, bool blocking)
{
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
    // 检查socket是否连接上
    int ret = -1;

    fd_set  w_fds;
    FD_ZERO(&w_fds);
    FD_SET(fd, &w_fds);

    if (time_out <= 0)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(fd + 1, NULL, &w_fds, NULL, &tv);
    }
    else
    {
        struct timeval tv;
        tv.tv_sec = time_out / 1000;  //取秒
        tv.tv_usec = (time_out % 1000) * 1000; //取微妙
        ret = select(fd + 1, NULL, &w_fds, NULL, &tv);
    }

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

static klb_socket_fd connect_tcp(struct sockaddr_in* p_addr, int time_out)
{
    klb_socket_fd fd = socket(p_addr->sin_family/*AF_INET*/, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == fd)
    {
        return INVALID_SOCKET;
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

    if (0 != connect_check_wait(fd, time_out))
    {
        goto err_connect;
    }

    if (0 <= time_out)
    {
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
    sds p_port = sdsfromlonglong(port); // 服务端口

    int ret = getaddrinfo(p_host, p_port, NULL, &p_result);
    if (0 == ret)
    {
        struct addrinfo* p_next = p_result;
        while (NULL != p_next)
        {
            fd = connect_tcp((struct sockaddr_in*)p_next->ai_addr, time_out);
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
        KLB_LOG("socket connect:[%s:%d] error!gai_strerror:%s\n", p_host, port, p_err);
    }

    KLB_FREE_BY(p_port, sdsfree);
    KLB_FREE_BY(p_result, freeaddrinfo);
    return fd;
}
