#include "socket/klb_go_socket.h"
#include "mem/klb_mem.h"


void klb_go_socket_destroy(klb_go_socket_t* p_socket)
{
    klb_go_socket_destroy_cb cb_destroy = p_socket->cb_destroy;
    cb_destroy(p_socket);
}

int klb_go_socket_send(klb_go_socket_t* p_socket, const char* p_buf, int buf_len)
{
    return p_socket->cb_send(p_socket, p_buf, buf_len);
}

int klb_go_socket_recv(klb_go_socket_t* p_socket, char* p_buf, int buf_len)
{
    return p_socket->cb_recv(p_socket, p_buf, buf_len);
}

int klb_go_socket_sendto(klb_go_socket_t* p_socket, const char* p_buf, int buf_len, const struct sockaddr* p_addr, int addr_len)
{
    return p_socket->cb_sendto(p_socket, p_buf, buf_len, p_addr, addr_len);
}

int klb_go_socket_recvfrom(klb_go_socket_t* p_socket, char* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
    return p_socket->cb_recvfrom(p_socket, p_buf, buf_len, p_addr, p_addr_len);
}


//////////////////////////////////////////////////////////////////////////

static void my_socket_destroy(klb_go_socket_t* p_socket)
{
    KLB_FREE(p_socket);
}

static int my_socket_send(klb_go_socket_t* p_socket, const char* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = send(p_socket->fd, p_buf, buf_len, 0);
#else
#ifndef __APPLE__
    int ret = send(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT | MSG_NOSIGNAL);
#else
    int ret = send(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT);
#endif
#endif

    return ret;
}

static int my_socket_recv(klb_go_socket_t* p_socket, char* p_buf, int buf_len)
{
#ifdef _WIN32
    int ret = recv(p_socket->fd, p_buf, buf_len, 0);
#else
    int ret = recv(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT);
#endif

    return ret;
}

static int my_socket_sendto(klb_go_socket_t* p_socket, const char* p_buf, int buf_len, const struct sockaddr* p_addr, int addr_len)
{
#ifdef _WIN32
    int ret = sendto(p_socket->fd, p_buf, buf_len, 0, p_addr, addr_len);
#else
#ifndef __APPLE__
    int ret = sendto(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT | MSG_NOSIGNAL, p_addr, addr_len);
#else
    int ret = sendto(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT, p_addr, addr_len);
#endif
#endif

    return ret;
}

static int my_socket_recvfrom(klb_go_socket_t* p_socket, char* p_buf, int buf_len, struct sockaddr* p_addr, int* p_addr_len)
{
#ifdef _WIN32
    int ret = recvfrom(p_socket->fd, p_buf, buf_len, 0, p_addr, p_addr_len);
#else
    socklen_t addr_len = *p_addr_len;
    int ret = recvfrom(p_socket->fd, p_buf, buf_len, MSG_DONTWAIT, p_addr, &addr_len);

    *p_addr_len = addr_len;
#endif

    return ret;
}

static klb_go_socket_t* my_socket_create(klb_socket_fd fd)
{
    klb_go_socket_t* ptr = KLB_MALLOC(klb_go_socket_t, 1, 0);
    KLB_MEMSET(ptr, 0, sizeof(klb_go_socket_t));

    ptr->fd = fd;

    ptr->cb_destroy = my_socket_destroy;
    ptr->cb_send = my_socket_send;
    ptr->cb_recv = my_socket_recv;
    ptr->cb_sendto = my_socket_sendto;
    ptr->cb_recvfrom = my_socket_recvfrom;

    return ptr;
}

//////////////////////////////////////////////////////////////////////////

klb_go_socket_t* klb_go_socket_create(klb_socket_fd fd)
{
    return my_socket_create(fd);
}
