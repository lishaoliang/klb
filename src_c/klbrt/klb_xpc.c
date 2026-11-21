// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbrt/klb_xpc.h"
#include "klbplatform/klb_msgq.h"
#include "klbplatform/klb_npipe.h"
#include "klbplatform/klb_sysmq.h"
#include "klbnet/klb_socket.h"
#include "klbutil/klb_rand.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#endif


#define KLB_XPC_MAGIC           0x4B585043u     ///< 'KXPC'
#define KLB_XPC_VERSION         2u              ///< v2: name/path buf 108 (was name 16)
#define KLB_XPC_MSGQ_CAPACITY   (64u * 1024u)
#define KLB_XPC_MSGQ_MAX_MSG    (8u * 1024u)
#define KLB_XPC_SYSMQ_MAX_COUNT 8u      ///< keep <= /proc/sys/fs/mqueue/msg_max
#define KLB_XPC_SYSMQ_MAX_MSG   KLB_XPC_MSGQ_MAX_MSG
#define KLB_XPC_QNAME_MAX       (KLB_XPC_NAME_BUF + 16) ///< "xpc.in." + name
#define KLB_XPC_EPHEMERAL_RAND  14u     ///< client temp: 'c' + 14 = 15 chars (default short)
#define KLB_XPC_SEND_WAIT_MS    3000u
#define KLB_XPC_UNIX_BACKLOG    16


#pragma pack(4)

/// @struct klb_xpc_hdr_t
/// @brief  信封头(msgq 整消息前部; unix 流式帧前部, 同布局)
typedef struct klb_xpc_hdr_t_
{
    uint32_t        magic;          ///< KLB_XPC_MAGIC
    uint16_t        version;        ///< KLB_XPC_VERSION
    uint16_t        type;           ///< KLB_XPC_*
    uint32_t        corr_id;        ///< REQUEST/RESPONSE 配对
    uint32_t        payload_len;    ///< 载荷字节
    char            src[KLB_XPC_NAME_BUF];
    char            dst[KLB_XPC_NAME_BUF];
}klb_xpc_hdr_t;

#pragma pack()


/// @struct klb_xpc_pending_t
/// @brief  本端暂存(request 等待期间收到的非目标消息)
typedef struct klb_xpc_pending_t_
{
    struct klb_xpc_pending_t_*  p_next;
    int                         type;
    uint32_t                    corr_id;
    uint32_t                    payload_len;
    char                        src[KLB_XPC_NAME_BUF];
    uint8_t                     payload[1];
}klb_xpc_pending_t;


/// @struct klb_xpc_t_
struct klb_xpc_t_
{
    char                local_name[KLB_XPC_NAME_BUF];
    char                path[KLB_XPC_PATH_MAX];
    char                peer_name[KLB_XPC_NAME_BUF];
    char                peer_path[KLB_XPC_PATH_MAX];
    int                 transport;
    bool                is_owner;
    bool                unlink_path;
    klb_msgq_t*         p_inbox;
    klb_npipe_t*        p_npipe;
    klb_sysmq_t*        p_sysmq;
    klb_socket_fd       listen_fd;
    uint32_t            next_corr;
    klb_xpc_pending_t*  p_pending_head;
    klb_xpc_pending_t*  p_pending_tail;
};


//////////////////////////////////////////////////////////////////////////


static uint32_t now_ms_klb_xpc(void)
{
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
}


static bool timed_out_klb_xpc(uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t now = 0;
    uint32_t elapsed = 0;

    if ((uint32_t)-1 == timeout_ms)
    {
        return false;
    }

    if (0 == timeout_ms)
    {
        return true;
    }

    now = now_ms_klb_xpc();
    elapsed = now - start_ms;
    return (elapsed >= timeout_ms);
}


static uint32_t remain_ms_klb_xpc(uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t now = 0;
    uint32_t elapsed = 0;

    if ((uint32_t)-1 == timeout_ms)
    {
        return (uint32_t)-1;
    }

    if (0 == timeout_ms)
    {
        return 0;
    }

    now = now_ms_klb_xpc();
    elapsed = now - start_ms;
    if (elapsed >= timeout_ms)
    {
        return 0;
    }

    return timeout_ms - elapsed;
}


static bool valid_name_klb_xpc(const char* p_name)
{
    size_t n = 0;

    if (NULL == p_name)
    {
        return false;
    }

    n = strlen(p_name);
    if (0 == n || KLB_XPC_NAME_LEN < n)
    {
        return false;
    }

    return true;
}


static void make_inbox_name_klb_xpc(char* p_out, size_t out_len, const char* p_mod)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

    snprintf(p_out, out_len, "xpc.in.%s", p_mod);
    p_out[out_len - 1] = '\0';
}


static void make_unix_path_klb_xpc(char* p_out, size_t out_len, const char* p_mod)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

    snprintf(p_out, out_len, "/tmp/klb.xpc.%s", p_mod);
    p_out[out_len - 1] = '\0';
}


static void make_npipe_path_klb_xpc(char* p_out, size_t out_len, const char* p_mod)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

#ifdef _WIN32
    snprintf(p_out, out_len, "\\\\.\\pipe\\klb.xpc.n.%s", p_mod);
#else
    snprintf(p_out, out_len, "/tmp/klb.xpc.n.%s", p_mod);
#endif
    p_out[out_len - 1] = '\0';
}


static void resolve_sysmq_name_klb_xpc(char* p_out, size_t out_len, const char* p_mod, const char* p_path)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

    if (NULL != p_path && '\0' != p_path[0])
    {
        snprintf(p_out, out_len, "%s", p_path);
        p_out[out_len - 1] = '\0';
        return;
    }

    make_inbox_name_klb_xpc(p_out, out_len, p_mod);
}


static void dst_sysmq_name_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, char* p_out, size_t out_len)
{
    assert(NULL != p_xpc);
    assert(NULL != p_dst);
    assert(NULL != p_out);

    if ('\0' != p_xpc->peer_name[0] && 0 == strcmp(p_dst, p_xpc->peer_name)
        && '\0' != p_xpc->peer_path[0])
    {
        strncpy(p_out, p_xpc->peer_path, out_len - 1u);
        p_out[out_len - 1u] = '\0';
        return;
    }

    make_inbox_name_klb_xpc(p_out, out_len, p_dst);
}


static void resolve_unix_path_klb_xpc(char* p_out, size_t out_len, const char* p_mod, const char* p_path)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

    if (NULL != p_path && '\0' != p_path[0])
    {
        strncpy(p_out, p_path, out_len - 1u);
        p_out[out_len - 1u] = '\0';
        return;
    }

    make_unix_path_klb_xpc(p_out, out_len, p_mod);
}


static void resolve_npipe_path_klb_xpc(char* p_out, size_t out_len, const char* p_mod, const char* p_path)
{
    assert(NULL != p_out);
    assert(NULL != p_mod);

    if (NULL != p_path && '\0' != p_path[0])
    {
        strncpy(p_out, p_path, out_len - 1u);
        p_out[out_len - 1u] = '\0';
        return;
    }

    make_npipe_path_klb_xpc(p_out, out_len, p_mod);
}


static void sleep_ms_klb_xpc(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000u);
#endif
}


static uint32_t max_payload_klb_xpc(void)
{
    return KLB_XPC_MSGQ_MAX_MSG - (uint32_t)sizeof(klb_xpc_hdr_t);
}


static void pending_push_klb_xpc(klb_xpc_t* p_xpc, int type, uint32_t corr_id,
                                 const char* p_src, const void* p_data, uint32_t len)
{
    klb_xpc_pending_t* p_node = NULL;
    size_t bytes = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_src);

    bytes = sizeof(klb_xpc_pending_t) + (0 < len ? (size_t)len - 1u : 0u);
    p_node = (klb_xpc_pending_t*)KLB_MALLOCZ(uint8_t, bytes, 0);
    p_node->type = type;
    p_node->corr_id = corr_id;
    p_node->payload_len = len;
    strncpy(p_node->src, p_src, KLB_XPC_NAME_LEN);
    p_node->src[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < len && NULL != p_data)
    {
        memcpy(p_node->payload, p_data, len);
    }

    p_node->p_next = NULL;
    if (NULL == p_xpc->p_pending_tail)
    {
        p_xpc->p_pending_head = p_node;
        p_xpc->p_pending_tail = p_node;
    }
    else
    {
        p_xpc->p_pending_tail->p_next = p_node;
        p_xpc->p_pending_tail = p_node;
    }
}


static klb_xpc_pending_t* pending_pop_klb_xpc(klb_xpc_t* p_xpc)
{
    klb_xpc_pending_t* p_node = NULL;

    assert(NULL != p_xpc);

    p_node = p_xpc->p_pending_head;
    if (NULL == p_node)
    {
        return NULL;
    }

    p_xpc->p_pending_head = p_node->p_next;
    if (NULL == p_xpc->p_pending_head)
    {
        p_xpc->p_pending_tail = NULL;
    }

    p_node->p_next = NULL;
    return p_node;
}


static void pending_free_all_klb_xpc(klb_xpc_t* p_xpc)
{
    klb_xpc_pending_t* p_node = NULL;

    assert(NULL != p_xpc);

    while (NULL != (p_node = pending_pop_klb_xpc(p_xpc)))
    {
        KLB_FREE(p_node);
    }
}


static int parse_frame_klb_xpc(const uint8_t* p_raw, uint32_t raw_len,
                               klb_xpc_hdr_t* p_hdr_out,
                               const uint8_t** pp_payload)
{
    const klb_xpc_hdr_t* p_hdr = NULL;

    assert(NULL != p_raw);
    assert(NULL != p_hdr_out);
    assert(NULL != pp_payload);

    if (raw_len < (uint32_t)sizeof(klb_xpc_hdr_t))
    {
        return -1;
    }

    p_hdr = (const klb_xpc_hdr_t*)p_raw;
    if (KLB_XPC_MAGIC != p_hdr->magic)
    {
        return -1;
    }

    if (KLB_XPC_VERSION != p_hdr->version)
    {
        return -1;
    }

    if (raw_len < (uint32_t)sizeof(klb_xpc_hdr_t) + p_hdr->payload_len)
    {
        return -1;
    }

    *p_hdr_out = *p_hdr;
    *pp_payload = p_raw + sizeof(klb_xpc_hdr_t);
    return 0;
}


#ifndef _WIN32
static int wait_fd_klb_xpc(klb_socket_fd fd, bool want_read, uint32_t timeout_ms)
{
    fd_set fds;
    struct timeval tv;
    struct timeval* p_tv = NULL;
    int ret = 0;

    if (INVALID_SOCKET == fd)
    {
        return -1;
    }

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if ((uint32_t)-1 != timeout_ms)
    {
        tv.tv_sec = (time_t)(timeout_ms / 1000u);
        tv.tv_usec = (suseconds_t)((timeout_ms % 1000u) * 1000u);
        p_tv = &tv;
    }

    if (want_read)
    {
        ret = select((int)fd + 1, &fds, NULL, NULL, p_tv);
    }
    else
    {
        ret = select((int)fd + 1, NULL, &fds, NULL, p_tv);
    }

    if (ret <= 0)
    {
        return -1;
    }

    return 0;
}


static int io_full_klb_xpc(klb_socket_fd fd, void* p_buf, uint32_t len, bool is_read, uint32_t timeout_ms)
{
    uint8_t* p = (uint8_t*)p_buf;
    uint32_t done = 0;
    uint32_t start = 0;
    uint32_t wait_ms = 0;
    int n = 0;

    assert(NULL != p_buf || 0 == len);

    if (0 == len)
    {
        return 0;
    }

    start = now_ms_klb_xpc();
    while (done < len)
    {
        if (timed_out_klb_xpc(start, timeout_ms))
        {
            return -1;
        }

        wait_ms = remain_ms_klb_xpc(start, timeout_ms);
        if (0 != wait_fd_klb_xpc(fd, is_read, wait_ms))
        {
            return -1;
        }

        if (is_read)
        {
            n = (int)recv(fd, (char*)(p + done), (size_t)(len - done), 0);
        }
        else
        {
            n = (int)send(fd, (const char*)(p + done), (size_t)(len - done), 0);
        }

        if (0 == n)
        {
            return -1;
        }

        if (n < 0)
        {
            if (EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
            {
                continue;
            }

            return -1;
        }

        done += (uint32_t)n;
    }

    return 0;
}
#endif


static void dst_path_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, char* p_out, size_t out_len)
{
    assert(NULL != p_xpc);
    assert(NULL != p_dst);
    assert(NULL != p_out);

    if ('\0' != p_xpc->peer_name[0] && 0 == strcmp(p_dst, p_xpc->peer_name)
        && '\0' != p_xpc->peer_path[0])
    {
        strncpy(p_out, p_xpc->peer_path, out_len - 1u);
        p_out[out_len - 1u] = '\0';
        return;
    }

    make_unix_path_klb_xpc(p_out, out_len, p_dst);
}


static int send_frame_unix_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, uint16_t type,
                                   uint32_t corr_id, const void* p_data, uint32_t len,
                                   uint32_t timeout_ms)
{
#ifdef _WIN32
    (void)p_xpc;
    (void)p_dst;
    (void)type;
    (void)corr_id;
    (void)p_data;
    (void)len;
    (void)timeout_ms;
    return -1;
#else
    char path[KLB_XPC_PATH_MAX];
    uint8_t frame[KLB_XPC_MSGQ_MAX_MSG];
    klb_xpc_hdr_t* p_hdr = NULL;
    klb_socket_fd fd = INVALID_SOCKET;
    uint32_t frame_len = 0;
    uint32_t wait_ms = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_dst);

    if (!valid_name_klb_xpc(p_dst))
    {
        return -1;
    }

    if (len > max_payload_klb_xpc())
    {
        return -1;
    }

    if (0 < len && NULL == p_data)
    {
        return -1;
    }

    frame_len = (uint32_t)sizeof(klb_xpc_hdr_t) + len;
    p_hdr = (klb_xpc_hdr_t*)frame;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = KLB_XPC_MAGIC;
    p_hdr->version = (uint16_t)KLB_XPC_VERSION;
    p_hdr->type = type;
    p_hdr->corr_id = corr_id;
    p_hdr->payload_len = len;
    strncpy(p_hdr->src, p_xpc->local_name, KLB_XPC_NAME_LEN);
    p_hdr->src[KLB_XPC_NAME_BUF - 1] = '\0';
    strncpy(p_hdr->dst, p_dst, KLB_XPC_NAME_LEN);
    p_hdr->dst[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < len)
    {
        memcpy(frame + sizeof(klb_xpc_hdr_t), p_data, len);
    }

    dst_path_klb_xpc(p_xpc, p_dst, path, sizeof(path));
    wait_ms = timeout_ms;
    if (0 == wait_ms)
    {
        wait_ms = 1;
    }
    else if ((uint32_t)-1 == wait_ms)
    {
        wait_ms = KLB_XPC_SEND_WAIT_MS;
    }

    fd = klb_socket_connect_unix(path, (int)wait_ms);
    if (INVALID_SOCKET == fd)
    {
        return -1;
    }

    klb_socket_set_block(fd, false);
    rc = io_full_klb_xpc(fd, frame, frame_len, false, wait_ms);
    KLB_SOCKET_CLOSE(fd);
    return rc;
#endif
}


static int send_frame_msgq_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, uint16_t type,
                                   uint32_t corr_id, const void* p_data, uint32_t len,
                                   uint32_t timeout_ms)
{
    char qname[KLB_XPC_QNAME_MAX];
    uint8_t frame[KLB_XPC_MSGQ_MAX_MSG];
    klb_xpc_hdr_t* p_hdr = NULL;
    klb_msgq_t* p_out = NULL;
    uint32_t frame_len = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_dst);

    if (!valid_name_klb_xpc(p_dst))
    {
        return -1;
    }

    if (len > max_payload_klb_xpc())
    {
        return -1;
    }

    if (0 < len && NULL == p_data)
    {
        return -1;
    }

    frame_len = (uint32_t)sizeof(klb_xpc_hdr_t) + len;
    p_hdr = (klb_xpc_hdr_t*)frame;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = KLB_XPC_MAGIC;
    p_hdr->version = (uint16_t)KLB_XPC_VERSION;
    p_hdr->type = type;
    p_hdr->corr_id = corr_id;
    p_hdr->payload_len = len;
    strncpy(p_hdr->src, p_xpc->local_name, KLB_XPC_NAME_LEN);
    p_hdr->src[KLB_XPC_NAME_BUF - 1] = '\0';
    strncpy(p_hdr->dst, p_dst, KLB_XPC_NAME_LEN);
    p_hdr->dst[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < len)
    {
        memcpy(frame + sizeof(klb_xpc_hdr_t), p_data, len);
    }

    make_inbox_name_klb_xpc(qname, sizeof(qname), p_dst);
    p_out = klb_msgq_open(qname, KLB_XPC_MSGQ_CAPACITY, KLB_XPC_MSGQ_MAX_MSG);
    if (NULL == p_out)
    {
        return -1;
    }

    rc = klb_msgq_send(p_out, frame, frame_len, timeout_ms);
    klb_msgq_destroy(p_out);
    return rc;
}


static int send_frame_sysmq_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, uint16_t type,
                                    uint32_t corr_id, const void* p_data, uint32_t len,
                                    uint32_t timeout_ms)
{
    char qname[KLB_XPC_PATH_MAX];
    uint8_t frame[KLB_XPC_SYSMQ_MAX_MSG];
    klb_xpc_hdr_t* p_hdr = NULL;
    klb_sysmq_t* p_out = NULL;
    uint32_t frame_len = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_dst);

    if (!valid_name_klb_xpc(p_dst))
    {
        return -1;
    }

    if (len > max_payload_klb_xpc())
    {
        return -1;
    }

    if (0 < len && NULL == p_data)
    {
        return -1;
    }

    frame_len = (uint32_t)sizeof(klb_xpc_hdr_t) + len;
    p_hdr = (klb_xpc_hdr_t*)frame;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = KLB_XPC_MAGIC;
    p_hdr->version = (uint16_t)KLB_XPC_VERSION;
    p_hdr->type = type;
    p_hdr->corr_id = corr_id;
    p_hdr->payload_len = len;
    strncpy(p_hdr->src, p_xpc->local_name, KLB_XPC_NAME_LEN);
    p_hdr->src[KLB_XPC_NAME_BUF - 1] = '\0';
    strncpy(p_hdr->dst, p_dst, KLB_XPC_NAME_LEN);
    p_hdr->dst[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < len)
    {
        memcpy(frame + sizeof(klb_xpc_hdr_t), p_data, len);
    }

    dst_sysmq_name_klb_xpc(p_xpc, p_dst, qname, sizeof(qname));
    p_out = klb_sysmq_open(qname, KLB_XPC_SYSMQ_MAX_COUNT, KLB_XPC_SYSMQ_MAX_MSG);
    if (NULL == p_out)
    {
        return -1;
    }

    rc = klb_sysmq_send(p_out, frame, frame_len, timeout_ms);
    klb_sysmq_destroy(p_out);
    return rc;
}


static int npipe_io_full_klb_xpc(klb_npipe_t* p_npipe, void* p_buf, uint32_t len,
                                 bool is_read, uint32_t timeout_ms)
{
    uint8_t* p = (uint8_t*)p_buf;
    uint32_t done = 0;
    uint32_t start = 0;
    int n = 0;

    assert(NULL != p_npipe);
    assert(NULL != p_buf || 0 == len);

    if (0 == len)
    {
        return 0;
    }

    start = now_ms_klb_xpc();
    (void)klb_npipe_set_block(p_npipe, false);

    while (done < len)
    {
        if (timed_out_klb_xpc(start, timeout_ms))
        {
            return -1;
        }

        if (is_read)
        {
            n = klb_npipe_read(p_npipe, p + done, (int)(len - done));
        }
        else
        {
            n = klb_npipe_write(p_npipe, p + done, (int)(len - done));
        }

        if (0 < n)
        {
            done += (uint32_t)n;
            continue;
        }

        if (0 == n && is_read)
        {
            return -1;
        }

        sleep_ms_klb_xpc(1);
    }

    return 0;
}


static void dst_npipe_path_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, char* p_out, size_t out_len)
{
    assert(NULL != p_xpc);
    assert(NULL != p_dst);
    assert(NULL != p_out);

    if ('\0' != p_xpc->peer_name[0] && 0 == strcmp(p_dst, p_xpc->peer_name)
        && '\0' != p_xpc->peer_path[0])
    {
        strncpy(p_out, p_xpc->peer_path, out_len - 1u);
        p_out[out_len - 1u] = '\0';
        return;
    }

    make_npipe_path_klb_xpc(p_out, out_len, p_dst);
}


static int send_frame_npipe_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, uint16_t type,
                                    uint32_t corr_id, const void* p_data, uint32_t len,
                                    uint32_t timeout_ms)
{
    char path[KLB_XPC_PATH_MAX];
    uint8_t frame[KLB_XPC_MSGQ_MAX_MSG];
    klb_xpc_hdr_t* p_hdr = NULL;
    klb_npipe_t* p_out = NULL;
    uint32_t frame_len = 0;
    uint32_t wait_ms = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_dst);

    if (!valid_name_klb_xpc(p_dst))
    {
        return -1;
    }

    if (len > max_payload_klb_xpc())
    {
        return -1;
    }

    if (0 < len && NULL == p_data)
    {
        return -1;
    }

    frame_len = (uint32_t)sizeof(klb_xpc_hdr_t) + len;
    p_hdr = (klb_xpc_hdr_t*)frame;
    memset(p_hdr, 0, sizeof(*p_hdr));
    p_hdr->magic = KLB_XPC_MAGIC;
    p_hdr->version = (uint16_t)KLB_XPC_VERSION;
    p_hdr->type = type;
    p_hdr->corr_id = corr_id;
    p_hdr->payload_len = len;
    strncpy(p_hdr->src, p_xpc->local_name, KLB_XPC_NAME_LEN);
    p_hdr->src[KLB_XPC_NAME_BUF - 1] = '\0';
    strncpy(p_hdr->dst, p_dst, KLB_XPC_NAME_LEN);
    p_hdr->dst[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < len)
    {
        memcpy(frame + sizeof(klb_xpc_hdr_t), p_data, len);
    }

    dst_npipe_path_klb_xpc(p_xpc, p_dst, path, sizeof(path));
    wait_ms = timeout_ms;
    if (0 == wait_ms)
    {
        wait_ms = 1;
    }
    else if ((uint32_t)-1 == wait_ms)
    {
        wait_ms = KLB_XPC_SEND_WAIT_MS;
    }

    p_out = klb_npipe_open(path);
    if (NULL == p_out)
    {
        return -1;
    }

    rc = npipe_io_full_klb_xpc(p_out, frame, frame_len, false, wait_ms);
    klb_npipe_destroy(p_out);
    return rc;
}


static int send_frame_klb_xpc(klb_xpc_t* p_xpc, const char* p_dst, uint16_t type,
                              uint32_t corr_id, const void* p_data, uint32_t len,
                              uint32_t timeout_ms)
{
    assert(NULL != p_xpc);

    if (KLB_XPC_TRANSPORT_UNIX == p_xpc->transport)
    {
        return send_frame_unix_klb_xpc(p_xpc, p_dst, type, corr_id, p_data, len, timeout_ms);
    }

    if (KLB_XPC_TRANSPORT_NPIPE == p_xpc->transport)
    {
        return send_frame_npipe_klb_xpc(p_xpc, p_dst, type, corr_id, p_data, len, timeout_ms);
    }

    if (KLB_XPC_TRANSPORT_SYSMQ == p_xpc->transport)
    {
        return send_frame_sysmq_klb_xpc(p_xpc, p_dst, type, corr_id, p_data, len, timeout_ms);
    }

    return send_frame_msgq_klb_xpc(p_xpc, p_dst, type, corr_id, p_data, len, timeout_ms);
}


static int recv_raw_unix_klb_xpc(klb_xpc_t* p_xpc, uint8_t* p_raw, uint32_t raw_cap,
                                 uint32_t* p_raw_len, uint32_t timeout_ms)
{
#ifdef _WIN32
    (void)p_xpc;
    (void)p_raw;
    (void)raw_cap;
    (void)p_raw_len;
    (void)timeout_ms;
    return -1;
#else
    klb_socket_fd fd = INVALID_SOCKET;
    klb_xpc_hdr_t hdr;
    uint32_t start = 0;
    uint32_t wait_ms = 0;
    uint32_t frame_len = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_raw);
    assert(NULL != p_raw_len);
    assert(INVALID_SOCKET != p_xpc->listen_fd);

    start = now_ms_klb_xpc();
    wait_ms = remain_ms_klb_xpc(start, timeout_ms);
    if (0 != wait_fd_klb_xpc(p_xpc->listen_fd, true, wait_ms))
    {
        return -1;
    }

    fd = klb_socket_accept_sa(p_xpc->listen_fd, NULL, NULL);
    if (INVALID_SOCKET == fd)
    {
        return -1;
    }

    klb_socket_set_block(fd, false);

    wait_ms = remain_ms_klb_xpc(start, timeout_ms);
    rc = io_full_klb_xpc(fd, &hdr, (uint32_t)sizeof(hdr), true, wait_ms);
    if (0 != rc)
    {
        KLB_SOCKET_CLOSE(fd);
        return -1;
    }

    if (KLB_XPC_MAGIC != hdr.magic || KLB_XPC_VERSION != hdr.version)
    {
        KLB_SOCKET_CLOSE(fd);
        return -1;
    }

    if (hdr.payload_len > max_payload_klb_xpc())
    {
        KLB_SOCKET_CLOSE(fd);
        return -1;
    }

    frame_len = (uint32_t)sizeof(hdr) + hdr.payload_len;
    if (raw_cap < frame_len)
    {
        KLB_SOCKET_CLOSE(fd);
        return -1;
    }

    memcpy(p_raw, &hdr, sizeof(hdr));
    if (0 < hdr.payload_len)
    {
        wait_ms = remain_ms_klb_xpc(start, timeout_ms);
        rc = io_full_klb_xpc(fd, p_raw + sizeof(hdr), hdr.payload_len, true, wait_ms);
        if (0 != rc)
        {
            KLB_SOCKET_CLOSE(fd);
            return -1;
        }
    }

    KLB_SOCKET_CLOSE(fd);
    *p_raw_len = frame_len;
    return 0;
#endif
}


static int recv_raw_msgq_klb_xpc(klb_xpc_t* p_xpc, uint8_t* p_raw, uint32_t raw_cap,
                                 uint32_t* p_raw_len, uint32_t timeout_ms)
{
    assert(NULL != p_xpc);
    assert(NULL != p_xpc->p_inbox);
    return klb_msgq_recv(p_xpc->p_inbox, p_raw, raw_cap, p_raw_len, timeout_ms);
}


static int recv_raw_sysmq_klb_xpc(klb_xpc_t* p_xpc, uint8_t* p_raw, uint32_t raw_cap,
                                  uint32_t* p_raw_len, uint32_t timeout_ms)
{
    assert(NULL != p_xpc);
    assert(NULL != p_xpc->p_sysmq);
    return klb_sysmq_recv(p_xpc->p_sysmq, p_raw, raw_cap, p_raw_len, timeout_ms);
}


static int recv_raw_npipe_klb_xpc(klb_xpc_t* p_xpc, uint8_t* p_raw, uint32_t raw_cap,
                                  uint32_t* p_raw_len, uint32_t timeout_ms)
{
    klb_xpc_hdr_t hdr;
    uint32_t start = 0;
    uint32_t wait_ms = 0;
    uint32_t frame_len = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_xpc->p_npipe);
    assert(NULL != p_raw);
    assert(NULL != p_raw_len);

    start = now_ms_klb_xpc();
    wait_ms = remain_ms_klb_xpc(start, timeout_ms);
    rc = npipe_io_full_klb_xpc(p_xpc->p_npipe, &hdr, (uint32_t)sizeof(hdr), true, wait_ms);
    if (0 != rc)
    {
        return -1;
    }

    if (KLB_XPC_MAGIC != hdr.magic || KLB_XPC_VERSION != hdr.version)
    {
        return -1;
    }

    if (hdr.payload_len > max_payload_klb_xpc())
    {
        return -1;
    }

    frame_len = (uint32_t)sizeof(hdr) + hdr.payload_len;
    if (raw_cap < frame_len)
    {
        return -1;
    }

    memcpy(p_raw, &hdr, sizeof(hdr));
    if (0 < hdr.payload_len)
    {
        wait_ms = remain_ms_klb_xpc(start, timeout_ms);
        rc = npipe_io_full_klb_xpc(p_xpc->p_npipe, p_raw + sizeof(hdr), hdr.payload_len, true, wait_ms);
        if (0 != rc)
        {
            return -1;
        }
    }

    *p_raw_len = frame_len;
    return 0;
}


static int recv_raw_klb_xpc(klb_xpc_t* p_xpc, uint8_t* p_raw, uint32_t raw_cap,
                            uint32_t* p_raw_len, uint32_t timeout_ms)
{
    assert(NULL != p_xpc);

    if (KLB_XPC_TRANSPORT_UNIX == p_xpc->transport)
    {
        return recv_raw_unix_klb_xpc(p_xpc, p_raw, raw_cap, p_raw_len, timeout_ms);
    }

    if (KLB_XPC_TRANSPORT_NPIPE == p_xpc->transport)
    {
        return recv_raw_npipe_klb_xpc(p_xpc, p_raw, raw_cap, p_raw_len, timeout_ms);
    }

    if (KLB_XPC_TRANSPORT_SYSMQ == p_xpc->transport)
    {
        return recv_raw_sysmq_klb_xpc(p_xpc, p_raw, raw_cap, p_raw_len, timeout_ms);
    }

    return recv_raw_msgq_klb_xpc(p_xpc, p_raw, raw_cap, p_raw_len, timeout_ms);
}


static klb_xpc_t* new_endpoint_msgq_klb_xpc(const char* p_local, bool is_owner)
{
    klb_xpc_t* p_xpc = NULL;
    char qname[KLB_XPC_QNAME_MAX];

    assert(NULL != p_local);

    p_xpc = KLB_MALLOCZ(klb_xpc_t, 1, 0);
    strncpy(p_xpc->local_name, p_local, KLB_XPC_NAME_LEN);
    p_xpc->local_name[KLB_XPC_NAME_BUF - 1] = '\0';
    p_xpc->transport = KLB_XPC_TRANSPORT_MSGQ;
    p_xpc->is_owner = is_owner;
    p_xpc->unlink_path = false;
    p_xpc->listen_fd = INVALID_SOCKET;
    p_xpc->p_npipe = NULL;
    p_xpc->p_sysmq = NULL;
    p_xpc->next_corr = 1;

    make_inbox_name_klb_xpc(qname, sizeof(qname), p_xpc->local_name);
    p_xpc->p_inbox = klb_msgq_create(qname, KLB_XPC_MSGQ_CAPACITY, KLB_XPC_MSGQ_MAX_MSG);
    if (NULL == p_xpc->p_inbox)
    {
        KLB_FREE(p_xpc);
        return NULL;
    }

    return p_xpc;
}


static bool probe_unix_peer_klb_xpc(const char* p_path);


static klb_xpc_t* new_endpoint_unix_klb_xpc(const char* p_local, bool is_owner, const char* p_path)
{
#ifdef _WIN32
    (void)p_local;
    (void)is_owner;
    (void)p_path;
    return NULL;
#else
    klb_xpc_t* p_xpc = NULL;

    assert(NULL != p_local);

    p_xpc = KLB_MALLOCZ(klb_xpc_t, 1, 0);
    strncpy(p_xpc->local_name, p_local, KLB_XPC_NAME_LEN);
    p_xpc->local_name[KLB_XPC_NAME_BUF - 1] = '\0';
    p_xpc->transport = KLB_XPC_TRANSPORT_UNIX;
    p_xpc->is_owner = is_owner;
    p_xpc->unlink_path = true;
    p_xpc->listen_fd = INVALID_SOCKET;
    p_xpc->p_inbox = NULL;
    p_xpc->p_npipe = NULL;
    p_xpc->p_sysmq = NULL;
    p_xpc->next_corr = 1;

    resolve_unix_path_klb_xpc(p_xpc->path, sizeof(p_xpc->path), p_local, p_path);

    // step1. owner only: live peer already owns path -> refuse (do not unlink)
    if (is_owner && probe_unix_peer_klb_xpc(p_xpc->path))
    {
        KLB_FREE(p_xpc);
        return NULL;
    }

    // step2. listen_unix may unlink stale path then bind
    p_xpc->listen_fd = klb_socket_listen_unix(p_xpc->path, KLB_XPC_UNIX_BACKLOG);
    if (INVALID_SOCKET == p_xpc->listen_fd)
    {
        KLB_FREE(p_xpc);
        return NULL;
    }

    return p_xpc;
#endif
}


static bool probe_unix_peer_klb_xpc(const char* p_path)
{
#ifdef _WIN32
    (void)p_path;
    return false;
#else
    klb_socket_fd fd = INVALID_SOCKET;

    assert(NULL != p_path);
    if ('\0' == p_path[0])
    {
        return false;
    }

    // step1. connect (not mere stat): stale sock file / orphan listen without path -> false
    fd = klb_socket_connect_unix(p_path, 200);
    if (INVALID_SOCKET == fd)
    {
        return false;
    }

    KLB_SOCKET_CLOSE(fd);
    return true;
#endif
}


static klb_xpc_t* new_endpoint_npipe_klb_xpc(const char* p_local, bool is_owner, const char* p_path)
{
    klb_xpc_t* p_xpc = NULL;

    assert(NULL != p_local);

    p_xpc = KLB_MALLOCZ(klb_xpc_t, 1, 0);
    strncpy(p_xpc->local_name, p_local, KLB_XPC_NAME_LEN);
    p_xpc->local_name[KLB_XPC_NAME_BUF - 1] = '\0';
    p_xpc->transport = KLB_XPC_TRANSPORT_NPIPE;
    p_xpc->is_owner = is_owner;
    p_xpc->unlink_path = false;
    p_xpc->listen_fd = INVALID_SOCKET;
    p_xpc->p_inbox = NULL;
    p_xpc->p_sysmq = NULL;
    p_xpc->next_corr = 1;

    resolve_npipe_path_klb_xpc(p_xpc->path, sizeof(p_xpc->path), p_local, p_path);
    p_xpc->p_npipe = klb_npipe_create(p_xpc->path);
    if (NULL == p_xpc->p_npipe)
    {
        KLB_FREE(p_xpc);
        return NULL;
    }

    return p_xpc;
}


static bool probe_npipe_peer_klb_xpc(const char* p_path)
{
    klb_npipe_t* p_probe = NULL;

    assert(NULL != p_path);
    p_probe = klb_npipe_open(p_path);
    if (NULL == p_probe)
    {
        return false;
    }

    klb_npipe_destroy(p_probe);
    return true;
}


static klb_xpc_t* new_endpoint_sysmq_klb_xpc(const char* p_local, bool is_owner, const char* p_path)
{
    klb_xpc_t* p_xpc = NULL;
    char qname[KLB_XPC_PATH_MAX];

    assert(NULL != p_local);

    p_xpc = KLB_MALLOCZ(klb_xpc_t, 1, 0);
    strncpy(p_xpc->local_name, p_local, KLB_XPC_NAME_LEN);
    p_xpc->local_name[KLB_XPC_NAME_BUF - 1] = '\0';
    p_xpc->transport = KLB_XPC_TRANSPORT_SYSMQ;
    p_xpc->is_owner = is_owner;
    p_xpc->unlink_path = false;
    p_xpc->listen_fd = INVALID_SOCKET;
    p_xpc->p_inbox = NULL;
    p_xpc->p_npipe = NULL;
    p_xpc->next_corr = 1;

    resolve_sysmq_name_klb_xpc(qname, sizeof(qname), p_local, p_path);
    strncpy(p_xpc->path, qname, KLB_XPC_PATH_MAX - 1);
    p_xpc->path[KLB_XPC_PATH_MAX - 1] = '\0';

    p_xpc->p_sysmq = klb_sysmq_create(qname, KLB_XPC_SYSMQ_MAX_COUNT, KLB_XPC_SYSMQ_MAX_MSG);
    if (NULL == p_xpc->p_sysmq)
    {
        KLB_FREE(p_xpc);
        return NULL;
    }

    return p_xpc;
}


static bool probe_sysmq_peer_klb_xpc(const char* p_qname)
{
    klb_sysmq_t* p_probe = NULL;

    assert(NULL != p_qname);
    p_probe = klb_sysmq_open(p_qname, KLB_XPC_SYSMQ_MAX_COUNT, KLB_XPC_SYSMQ_MAX_MSG);
    if (NULL == p_probe)
    {
        return false;
    }

    klb_sysmq_destroy(p_probe);
    return true;
}


//////////////////////////////////////////////////////////////////////////


klb_xpc_t* klb_xpc_create(const char* p_name)
{
    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    // step1. 创建具名 inbox(owner)
    return new_endpoint_msgq_klb_xpc(p_name, true);
}


klb_xpc_t* klb_xpc_create_unix(const char* p_name, const char* p_path)
{
    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    return new_endpoint_unix_klb_xpc(p_name, true, p_path);
}


klb_xpc_t* klb_xpc_create_npipe(const char* p_name, const char* p_path)
{
    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    return new_endpoint_npipe_klb_xpc(p_name, true, p_path);
}


klb_xpc_t* klb_xpc_create_sysmq(const char* p_name, const char* p_path)
{
    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    return new_endpoint_sysmq_klb_xpc(p_name, true, p_path);
}


klb_xpc_t* klb_xpc_open(const char* p_name)
{
    char qname[KLB_XPC_QNAME_MAX];
    char local[KLB_XPC_NAME_BUF];
    klb_msgq_t* p_probe = NULL;
    klb_xpc_t* p_xpc = NULL;
    int i = 0;

    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    // step1. 探测对端 inbox 是否存在
    make_inbox_name_klb_xpc(qname, sizeof(qname), p_name);
    p_probe = klb_msgq_open(qname, KLB_XPC_MSGQ_CAPACITY, KLB_XPC_MSGQ_MAX_MSG);
    if (NULL == p_probe)
    {
        return NULL;
    }

    klb_msgq_destroy(p_probe);

    // step2. 生成本地临时名并建 inbox(非服务 owner)
    for (i = 0; i < 8; ++i)
    {
        local[0] = 'c';
        klb_rand_string(local + 1, (int)KLB_XPC_EPHEMERAL_RAND, true);
        local[1 + KLB_XPC_EPHEMERAL_RAND] = '\0';

        if (0 == strcmp(local, p_name))
        {
            continue;
        }

        p_xpc = new_endpoint_msgq_klb_xpc(local, false);
        if (NULL != p_xpc)
        {
            return p_xpc;
        }
    }

    return NULL;
}


klb_xpc_t* klb_xpc_open_unix(const char* p_name, const char* p_path)
{
    char peer_path[KLB_XPC_PATH_MAX];
    char local[KLB_XPC_NAME_BUF];
    klb_xpc_t* p_xpc = NULL;
    int i = 0;

    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    resolve_unix_path_klb_xpc(peer_path, sizeof(peer_path), p_name, p_path);
    if (!probe_unix_peer_klb_xpc(peer_path))
    {
        return NULL;
    }

    for (i = 0; i < 8; ++i)
    {
        local[0] = 'c';
        klb_rand_string(local + 1, (int)KLB_XPC_EPHEMERAL_RAND, true);
        local[1 + KLB_XPC_EPHEMERAL_RAND] = '\0';

        if (0 == strcmp(local, p_name))
        {
            continue;
        }

        p_xpc = new_endpoint_unix_klb_xpc(local, false, NULL);
        if (NULL != p_xpc)
        {
            strncpy(p_xpc->peer_name, p_name, KLB_XPC_NAME_LEN);
            p_xpc->peer_name[KLB_XPC_NAME_BUF - 1] = '\0';
            strncpy(p_xpc->peer_path, peer_path, KLB_XPC_PATH_MAX - 1);
            p_xpc->peer_path[KLB_XPC_PATH_MAX - 1] = '\0';
            return p_xpc;
        }
    }

    return NULL;
}


klb_xpc_t* klb_xpc_open_npipe(const char* p_name, const char* p_path)
{
    char peer_path[KLB_XPC_PATH_MAX];
    char local[KLB_XPC_NAME_BUF];
    klb_xpc_t* p_xpc = NULL;
    int i = 0;

    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    resolve_npipe_path_klb_xpc(peer_path, sizeof(peer_path), p_name, p_path);
    if (!probe_npipe_peer_klb_xpc(peer_path))
    {
        return NULL;
    }

    for (i = 0; i < 8; ++i)
    {
        local[0] = 'c';
        klb_rand_string(local + 1, (int)KLB_XPC_EPHEMERAL_RAND, true);
        local[1 + KLB_XPC_EPHEMERAL_RAND] = '\0';

        if (0 == strcmp(local, p_name))
        {
            continue;
        }

        p_xpc = new_endpoint_npipe_klb_xpc(local, false, NULL);
        if (NULL != p_xpc)
        {
            strncpy(p_xpc->peer_name, p_name, KLB_XPC_NAME_LEN);
            p_xpc->peer_name[KLB_XPC_NAME_BUF - 1] = '\0';
            strncpy(p_xpc->peer_path, peer_path, KLB_XPC_PATH_MAX - 1);
            p_xpc->peer_path[KLB_XPC_PATH_MAX - 1] = '\0';
            return p_xpc;
        }
    }

    return NULL;
}


klb_xpc_t* klb_xpc_open_sysmq(const char* p_name, const char* p_path)
{
    char peer_qname[KLB_XPC_PATH_MAX];
    char local[KLB_XPC_NAME_BUF];
    klb_xpc_t* p_xpc = NULL;
    int i = 0;

    if (!valid_name_klb_xpc(p_name))
    {
        return NULL;
    }

    resolve_sysmq_name_klb_xpc(peer_qname, sizeof(peer_qname), p_name, p_path);
    if (!probe_sysmq_peer_klb_xpc(peer_qname))
    {
        return NULL;
    }

    for (i = 0; i < 8; ++i)
    {
        local[0] = 'c';
        klb_rand_string(local + 1, (int)KLB_XPC_EPHEMERAL_RAND, true);
        local[1 + KLB_XPC_EPHEMERAL_RAND] = '\0';

        if (0 == strcmp(local, p_name))
        {
            continue;
        }

        p_xpc = new_endpoint_sysmq_klb_xpc(local, false, NULL);
        if (NULL != p_xpc)
        {
            strncpy(p_xpc->peer_name, p_name, KLB_XPC_NAME_LEN);
            p_xpc->peer_name[KLB_XPC_NAME_BUF - 1] = '\0';
            strncpy(p_xpc->peer_path, peer_qname, KLB_XPC_PATH_MAX - 1);
            p_xpc->peer_path[KLB_XPC_PATH_MAX - 1] = '\0';
            return p_xpc;
        }
    }

    return NULL;
}


void klb_xpc_destroy(klb_xpc_t* p_xpc)
{
    assert(NULL != p_xpc);

    pending_free_all_klb_xpc(p_xpc);
    KLB_FREE_BY(p_xpc->p_inbox, klb_msgq_destroy);
    KLB_FREE_BY(p_xpc->p_npipe, klb_npipe_destroy);
    KLB_FREE_BY(p_xpc->p_sysmq, klb_sysmq_destroy);

    if (INVALID_SOCKET != p_xpc->listen_fd)
    {
        KLB_SOCKET_CLOSE(p_xpc->listen_fd);
    }

#ifndef _WIN32
    if (p_xpc->unlink_path && '\0' != p_xpc->path[0])
    {
        unlink(p_xpc->path);
    }
#endif

    KLB_FREE(p_xpc);
}


int klb_xpc_post(klb_xpc_t* p_xpc, const char* p_dst, const void* p_data, uint32_t len)
{
    assert(NULL != p_xpc);
    return send_frame_klb_xpc(p_xpc, p_dst, (uint16_t)KLB_XPC_POST, 0, p_data, len, KLB_XPC_SEND_WAIT_MS);
}


int klb_xpc_response(klb_xpc_t* p_xpc, const char* p_dst, uint32_t corr_id,
                     const void* p_data, uint32_t len)
{
    assert(NULL != p_xpc);
    return send_frame_klb_xpc(p_xpc, p_dst, (uint16_t)KLB_XPC_RESPONSE, corr_id,
                              p_data, len, KLB_XPC_SEND_WAIT_MS);
}


int klb_xpc_notify(klb_xpc_t* p_xpc, const char* p_dst, const void* p_data, uint32_t len)
{
    assert(NULL != p_xpc);
    return send_frame_klb_xpc(p_xpc, p_dst, (uint16_t)KLB_XPC_NOTIFY, 0, p_data, len, KLB_XPC_SEND_WAIT_MS);
}


bool klb_xpc_is_owner(klb_xpc_t* p_xpc)
{
    assert(NULL != p_xpc);
    return p_xpc->is_owner;
}


const char* klb_xpc_name(klb_xpc_t* p_xpc)
{
    assert(NULL != p_xpc);
    return p_xpc->local_name;
}


static int deliver_pending_klb_xpc(klb_xpc_pending_t* p_node, int* p_type, char* p_src,
                                   void* p_buf, uint32_t buf_size, uint32_t* p_out_len,
                                   uint32_t* p_corr_id)
{
    assert(NULL != p_node);
    assert(NULL != p_type);
    assert(NULL != p_src);
    assert(NULL != p_buf);

    if (buf_size < p_node->payload_len)
    {
        KLB_FREE(p_node);
        return -1;
    }

    *p_type = p_node->type;
    strncpy(p_src, p_node->src, KLB_XPC_NAME_LEN);
    p_src[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < p_node->payload_len)
    {
        memcpy(p_buf, p_node->payload, p_node->payload_len);
    }

    if (NULL != p_out_len)
    {
        *p_out_len = p_node->payload_len;
    }

    if (NULL != p_corr_id)
    {
        *p_corr_id = p_node->corr_id;
    }

    KLB_FREE(p_node);
    return 0;
}


int klb_xpc_recv(klb_xpc_t* p_xpc, int* p_type, char* p_src,
                 void* p_buf, uint32_t buf_size, uint32_t* p_out_len,
                 uint32_t* p_corr_id, uint32_t timeout_ms)
{
    klb_xpc_pending_t* p_pend = NULL;
    uint8_t raw[KLB_XPC_MSGQ_MAX_MSG];
    klb_xpc_hdr_t hdr;
    const uint8_t* p_payload = NULL;
    uint32_t raw_len = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_type);
    assert(NULL != p_src);
    assert(NULL != p_buf);
    assert(0 < buf_size);

    // step1. 先取 pending
    p_pend = pending_pop_klb_xpc(p_xpc);
    if (NULL != p_pend)
    {
        return deliver_pending_klb_xpc(p_pend, p_type, p_src, p_buf, buf_size, p_out_len, p_corr_id);
    }

    // step2. 从传输收一帧
    rc = recv_raw_klb_xpc(p_xpc, raw, sizeof(raw), &raw_len, timeout_ms);
    if (0 != rc)
    {
        return -1;
    }

    rc = parse_frame_klb_xpc(raw, raw_len, &hdr, &p_payload);
    if (0 != rc)
    {
        return -1;
    }

    if (buf_size < hdr.payload_len)
    {
        return -1;
    }

    *p_type = (int)hdr.type;
    strncpy(p_src, hdr.src, KLB_XPC_NAME_LEN);
    p_src[KLB_XPC_NAME_BUF - 1] = '\0';

    if (0 < hdr.payload_len)
    {
        memcpy(p_buf, p_payload, hdr.payload_len);
    }

    if (NULL != p_out_len)
    {
        *p_out_len = hdr.payload_len;
    }

    if (NULL != p_corr_id)
    {
        *p_corr_id = hdr.corr_id;
    }

    return 0;
}


int klb_xpc_request(klb_xpc_t* p_xpc, const char* p_dst,
                    const void* p_data, uint32_t len,
                    void* p_buf, uint32_t buf_size, uint32_t* p_out_len,
                    uint32_t timeout_ms)
{
    uint8_t raw[KLB_XPC_MSGQ_MAX_MSG];
    klb_xpc_hdr_t hdr;
    const uint8_t* p_payload = NULL;
    uint32_t raw_len = 0;
    uint32_t corr = 0;
    uint32_t start = 0;
    uint32_t wait_ms = 0;
    int rc = 0;

    assert(NULL != p_xpc);
    assert(NULL != p_buf);
    assert(0 < buf_size);

    // step1. 分配 corr 并发送 REQUEST
    corr = p_xpc->next_corr++;
    if (0 == corr)
    {
        corr = p_xpc->next_corr++;
    }

    start = now_ms_klb_xpc();
    wait_ms = remain_ms_klb_xpc(start, timeout_ms);
    if (0 == timeout_ms)
    {
        wait_ms = 0;
    }
    else if ((uint32_t)-1 != timeout_ms && 0 == wait_ms)
    {
        return -1;
    }

    if ((uint32_t)-1 == timeout_ms)
    {
        wait_ms = KLB_XPC_SEND_WAIT_MS;
    }
    else if (wait_ms > KLB_XPC_SEND_WAIT_MS)
    {
        wait_ms = KLB_XPC_SEND_WAIT_MS;
    }

    rc = send_frame_klb_xpc(p_xpc, p_dst, (uint16_t)KLB_XPC_REQUEST, corr,
                            p_data, len, wait_ms);
    if (0 != rc)
    {
        return -1;
    }

    // step2. 等待匹配 RESPONSE; 其它消息进 pending
    for (;;)
    {
        if (timed_out_klb_xpc(start, timeout_ms))
        {
            return -1;
        }

        wait_ms = remain_ms_klb_xpc(start, timeout_ms);
        rc = recv_raw_klb_xpc(p_xpc, raw, sizeof(raw), &raw_len, wait_ms);
        if (0 != rc)
        {
            return -1;
        }

        rc = parse_frame_klb_xpc(raw, raw_len, &hdr, &p_payload);
        if (0 != rc)
        {
            continue;
        }

        if (KLB_XPC_RESPONSE == hdr.type && corr == hdr.corr_id)
        {
            if (buf_size < hdr.payload_len)
            {
                return -1;
            }

            if (0 < hdr.payload_len)
            {
                memcpy(p_buf, p_payload, hdr.payload_len);
            }

            if (NULL != p_out_len)
            {
                *p_out_len = hdr.payload_len;
            }

            return 0;
        }

        pending_push_klb_xpc(p_xpc, (int)hdr.type, hdr.corr_id, hdr.src,
                             p_payload, hdr.payload_len);
    }
}

// end
