// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbonvif/core/klb_onvif_discovery.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_mnp.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_udp.h"
#include "klbnet/klb_netcode.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


#define KLB_ONVIF_DISCOVERY_RECV_MAX    8192
#define KLB_ONVIF_DISCOVERY_TTL         4


/// @struct klb_onvif_discovery_t
/// @brief  Discovery 私有状态
typedef struct klb_onvif_discovery_t_
{
    klb_netmulti_t*     p_netmulti;     ///< 复用
    int                 role;           ///< klb_onvif_discovery_role_e
    bool                joined;         ///< 是否已加入组播
    uint8_t             recv_buf[KLB_ONVIF_DISCOVERY_RECV_MAX]; ///< 收包缓存
}klb_onvif_discovery_t;


//////////////////////////////////////////////////////////////////////////
// 前置


static void quit_klb_onvif_discovery(klb_netconn_t* p_conn);
static void format_peer_klb_onvif_discovery(char* p_dst, int cap, const struct sockaddr_in* p_addr);
static void push_datagram_klb_onvif_discovery(klb_netconn_t* p_conn, const struct sockaddr_in* p_addr,
                                              const uint8_t* p_data, int len);
static void push_netcode_klb_onvif_discovery(klb_netconn_t* p_conn, int code);
static klb_socket_t* open_udp_socket_klb_onvif_discovery(int role, uint16_t port, int* p_code);


static void format_peer_klb_onvif_discovery(char* p_dst, int cap, const struct sockaddr_in* p_addr)
{
    unsigned int ip = ntohl(p_addr->sin_addr.s_addr);
    int port = (int)ntohs(p_addr->sin_port);
    int n = snprintf(p_dst, (size_t)cap, "%u.%u.%u.%u:%d",
        (ip >> 24) & 0xFFu,
        (ip >> 16) & 0xFFu,
        (ip >> 8) & 0xFFu,
        ip & 0xFFu,
        port);

    if (0 > n || n >= cap)
    {
        p_dst[0] = '\0';
    }

    p_dst[cap - 1] = '\0';
}


//////////////////////////////////////////////////////////////////////////
// 上送


static void push_datagram_klb_onvif_discovery(klb_netconn_t* p_conn, const struct sockaddr_in* p_addr,
                                              const uint8_t* p_data, int len)
{
    char peer[64];
    format_peer_klb_onvif_discovery(peer, (int)sizeof(peer), p_addr);

    int head_len = (int)strlen(peer);
    int total = (int)sizeof(klb_mnp_text_t) + head_len + len;
    klb_buf_t* p_buf = klb_buf_malloc(total, false);

    klb_mnp_text_t* p_txt = (klb_mnp_text_t*)p_buf->p_buf;
    memset(p_txt, 0, sizeof(klb_mnp_text_t));
    p_txt->size = (uint32_t)total;
    p_txt->head_size = (uint32_t)head_len;

    if (0 < head_len)
    {
        memcpy(p_buf->p_buf + sizeof(klb_mnp_text_t), peer, (size_t)head_len);
    }

    if (0 < len && NULL != p_data)
    {
        memcpy(p_buf->p_buf + sizeof(klb_mnp_text_t) + head_len, p_data, (size_t)len);
    }

    p_buf->start = 0;
    p_buf->end = total;

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, KLB_NETCODE_OK, KLB_MNP_TEXT, p_buf);
    }
    else
    {
        KLB_FREE_BY(p_buf, klb_buf_unref);
    }
}


static void push_netcode_klb_onvif_discovery(klb_netconn_t* p_conn, int code)
{
    assert(KLB_NETCODE_OK != code);

    if (NULL != p_conn->vtable.recv_data)
    {
        p_conn->vtable.recv_data(p_conn, code, 0, NULL);
    }
}


//////////////////////////////////////////////////////////////////////////
// netconn vtable


static void destroy_klb_onvif_discovery(klb_netconn_t* p_conn)
{
    klb_onvif_discovery_t* p_disc = (klb_onvif_discovery_t*)p_conn->extra;

    assert(NULL == p_disc->p_netmulti);

    if (p_disc->joined && NULL != p_conn->p_socket)
    {
        (void)klb_socket_udp_leave_v4(p_conn->p_socket, KLB_ONVIF_DISCOVERY_MCAST_IP, NULL);
        p_disc->joined = false;
    }

    KLB_FREE_BY(p_conn->p_socket, klb_socket_destroy);
    quit_klb_onvif_discovery(p_conn);
    KLB_FREE(p_conn);
}


static int ioctrl_klb_onvif_discovery(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out)
{
    (void)p_conn;
    (void)p_in;
    (void)p_out;
    return 0;
}


static int on_send_klb_onvif_discovery(klb_netconn_t* p_conn, int64_t now)
{
    (void)p_conn;
    (void)now;
    return 0;
}


static int on_recv_klb_onvif_discovery(klb_netconn_t* p_conn, int64_t now)
{
    klb_onvif_discovery_t* p_disc = (klb_onvif_discovery_t*)p_conn->extra;
    klb_socket_t* p_socket = p_conn->p_socket;
    int num = 0;

    (void)now;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    while (true)
    {
        struct sockaddr_in addr;
        int addr_len = (int)sizeof(addr);
        int n = 0;
        int judge = 0;

        memset(&addr, 0, sizeof(addr));
        n = klb_socket_udp_recvfrom(p_socket, p_disc->recv_buf, KLB_ONVIF_DISCOVERY_RECV_MAX,
            (struct sockaddr*)&addr, &addr_len);
        judge = klb_socket_udp_recv_judge(p_socket, n, KLB_ONVIF_DISCOVERY_RECV_MAX, NULL);

        if (KLB_SOCKET_UDP_RECV_DATA == judge)
        {
            if (0 < n)
            {
                push_datagram_klb_onvif_discovery(p_conn, &addr, p_disc->recv_buf, n);
                num++;
            }
        }
        else if (KLB_SOCKET_UDP_RECV_AGAIN == judge)
        {
            break;
        }
        else
        {
            push_netcode_klb_onvif_discovery(p_conn, KLB_SOCKET_DISCONNECT);
            break;
        }
    }

    return num;
}


static int on_msg_klb_onvif_discovery(klb_netconn_t* p_conn, int msg, int64_t now)
{
    (void)p_conn;
    (void)msg;
    (void)now;
    return 0;
}


static void quit_klb_onvif_discovery(klb_netconn_t* p_conn)
{
    (void)p_conn;
}


static void init_vtable_klb_onvif_discovery(klb_netconn_t* p_conn)
{
    p_conn->vtable.destroy = destroy_klb_onvif_discovery;
    p_conn->vtable.recv_data = NULL;
    p_conn->vtable.ioctrl = ioctrl_klb_onvif_discovery;
    p_conn->vtable.send_normal = NULL;
    p_conn->vtable.send_media = NULL;
    p_conn->vtable.on_send = on_send_klb_onvif_discovery;
    p_conn->vtable.on_recv = on_recv_klb_onvif_discovery;
    p_conn->vtable.on_msg = on_msg_klb_onvif_discovery;
}


//////////////////////////////////////////////////////////////////////////
// 打开 / 释放 / 发送


static klb_socket_t* open_udp_socket_klb_onvif_discovery(int role, uint16_t port, int* p_code)
{
    uint32_t flags = KLB_SOCKET_F_REUSEADDR;
    int bind_port = 0;
    klb_socket_t* p_socket = NULL;
    int code = KLB_NETCODE_BIND_FAIL;

    if (KLB_ONVIF_DISCOVERY_SERVE == role)
    {
        bind_port = (0 != port) ? (int)port : KLB_ONVIF_DISCOVERY_PORT;
    }
    else
    {
        bind_port = 0;
    }

    // step1. 统一 UDP open(bind) + TTL; serve 再 join 组播
    p_socket = klb_socket_udp_open(KLB_SOCKET_FAMILY_V4, "*", bind_port, flags);

    if (NULL == p_socket)
    {
        code = klb_netcode_last_syserr();

        if (KLB_NETCODE_IO == code || KLB_NETCODE_ERR == code)
        {
            code = KLB_NETCODE_BIND_FAIL;
        }

        if (NULL != p_code)
        {
            *p_code = code;
        }

        return NULL;
    }

    if (0 != klb_socket_udp_set_mcast_ttl(p_socket, KLB_ONVIF_DISCOVERY_TTL))
    {
        KLB_FREE_BY(p_socket, klb_socket_destroy);

        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_IO;
        }

        return NULL;
    }

    if (KLB_ONVIF_DISCOVERY_SERVE == role)
    {
        if (0 != klb_socket_udp_join_v4(p_socket, KLB_ONVIF_DISCOVERY_MCAST_IP, NULL))
        {
            KLB_FREE_BY(p_socket, klb_socket_destroy);

            if (NULL != p_code)
            {
                *p_code = KLB_NETCODE_IO;
            }

            return NULL;
        }
    }

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_socket;
}


klb_netconn_t* klb_onvif_discovery_open(klb_netmulti_t* p_netmulti, int role, uint16_t port, int* p_code)
{
    klb_socket_t* p_socket = NULL;
    klb_netconn_t* p_conn = NULL;
    klb_onvif_discovery_t* p_disc = NULL;
    int code = 0;

    if (NULL == p_netmulti)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    if (KLB_ONVIF_DISCOVERY_SEARCH != role && KLB_ONVIF_DISCOVERY_SERVE != role)
    {
        if (NULL != p_code)
        {
            *p_code = KLB_NETCODE_INVAL;
        }

        return NULL;
    }

    // step1. UDP + bind (+ join)
    p_socket = open_udp_socket_klb_onvif_discovery(role, port, &code);

    if (NULL == p_socket)
    {
        if (NULL != p_code)
        {
            *p_code = code;
        }

        return NULL;
    }

    // step2. netconn
    p_conn = KLB_MALLOCZ(klb_netconn_t, 1, sizeof(klb_onvif_discovery_t));
    p_disc = (klb_onvif_discovery_t*)p_conn->extra;

    init_vtable_klb_onvif_discovery(p_conn);
    p_conn->protocol = KLB_PROTOCOL_user;
    p_conn->p_socket = p_socket;
    p_disc->p_netmulti = p_netmulti;
    p_disc->role = role;
    p_disc->joined = (KLB_ONVIF_DISCOVERY_SERVE == role) ? true : false;

    klb_socket_set_connected(p_socket, true);
    klb_socket_set_reading(p_socket, true);

    // step3. 挂入复用
    klb_netmulti_push(p_netmulti, p_conn);

    if (NULL != p_code)
    {
        *p_code = 0;
    }

    return p_conn;
}


void klb_onvif_discovery_free(klb_netconn_t* p_conn)
{
    if (NULL == p_conn)
    {
        return;
    }

    klb_onvif_discovery_t* p_disc = (klb_onvif_discovery_t*)p_conn->extra;

    if (NULL != p_disc->p_netmulti)
    {
        klb_netmulti_closing(p_disc->p_netmulti, p_conn);
        p_disc->p_netmulti = NULL;
    }
}


int klb_onvif_discovery_send(klb_netconn_t* p_conn, const uint8_t* p_data, int len,
                             const char* p_ip, uint16_t port)
{
    if (NULL == p_conn || NULL == p_data || 0 >= len)
    {
        return -1;
    }

    klb_socket_t* p_socket = p_conn->p_socket;

    if (NULL == p_socket || KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return -1;
    }

    if (NULL == p_ip || '\0' == p_ip[0])
    {
        int rc = klb_socket_udp_sendto_host(p_socket, p_data, len,
            KLB_ONVIF_DISCOVERY_MCAST_IP, KLB_ONVIF_DISCOVERY_PORT);

        if (0 != rc)
        {
            return -1;
        }

        return 0;
    }
    else
    {
        int dst_port = (0 != port) ? (int)port : KLB_ONVIF_DISCOVERY_PORT;
        int rc = klb_socket_udp_sendto_host(p_socket, p_data, len, p_ip, dst_port);

        if (0 != rc)
        {
            return -1;
        }

        return 0;
    }
}


// end
