#include "socket/klb_socket_base.h"
#include "mem/klb_mem.h"
#include <assert.h>


/// @brief 销毁
static void klb_socket_base_destroy(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    KLB_FREE(p_socket);
}

/// @brief TCP发送数据
static int klb_socket_base_send(klb_socket_t* p_socket, const uint8_t* p_data, int len)
{
#ifdef _WIN32
    int ret = send(p_socket->fd, (const char*)p_data, len, 0);
#else
#ifndef __APPLE__
    int flags = p_socket->nonblock ? (MSG_DONTWAIT | MSG_NOSIGNAL) : 0;
    int ret = send(p_socket->fd, (const char*)p_data, len, flags);
#else
    int flags = p_socket->nonblock ? MSG_DONTWAIT: 0;
    int ret = send(p_socket->fd, (const char*)p_data, len, flags);
#endif
#endif

    return ret;
}

/// @brief TCP接收数据
static int klb_socket_base_recv(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len)
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
static int klb_socket_base_sendto(klb_socket_t* p_socket, const uint8_t* p_data, int len, const struct sockaddr* p_addr, int addr_len)
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
static int klb_socket_base_recvfrom(klb_socket_t* p_socket, uint8_t* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
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

void klb_socket_base_construct(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);

    KLB_MEMSET(p_socket, 0, sizeof(klb_socket_t));

    //p_socket->vtable.cb_destroy = klb_socket_base_destroy;
    p_socket->vtable.cb_send = klb_socket_base_send;
    p_socket->vtable.cb_recv = klb_socket_base_recv;
    p_socket->vtable.cb_sendto = klb_socket_base_sendto;
    p_socket->vtable.cb_recvfrom = klb_socket_base_recvfrom;

    p_socket->fd = INVALID_SOCKET;
}

klb_socket_fd klb_socket_base_destruct(klb_socket_t* p_socket)
{
    assert(NULL != p_socket);
    klb_socket_fd fd = p_socket->fd;
    KLB_MEMSET(&p_socket->vtable, 0, sizeof(klb_socket_vtable_t));

    return fd;
}
