// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbbase/klb_mnp.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef struct klb_ncm_ops_mnp_t_
{
    // ncm 参数
    struct
    {
        klb_ncm_t*          p_ncm;              ///< ncm模块
        int                 protocol;           ///< 协议编号
        int                 id;                 ///< id编号

        klb_ncm_ops_recv_cb cb_recv_to_ncm;     ///< 收到数据后, 交给ncm

        klb_buf_malloc_cb   cb_malloc;          ///< 文本(TEXT)/二进制(BINARY)数据类型的内存池分配函数
        void*               p_pool;             ///< cb_malloc函数的操作对象

        klb_buf_malloc_cb   cb_malloc_media;    ///< 媒体(MEDIA)数据类型的内存池分配函数
        void*               p_pool_media;       ///< b_malloc_media函数的操作对象
    };

    // send发送相关
    struct
    {
        klb_nlist_t*     p_w_list;       ///< 待发送列表: klb_buf_t*
        klb_buf_t*      p_w_cur;        ///< 当前正在发送的缓存
        int             w_start;        ///< 当前发送的起始点
    };

    // recv接收相关
    struct
    {
        klb_buf_t*      p_r_buf;        ///< 临时读取缓存
    };

    // 解析
    struct
    {
        klb_mnp_t       mnp;            ///< mnp头
        int             left_len;       ///< 剩下的数据长度


        klb_buffer_t*   p_txt;          ///< 文本/二进制
        klb_buffer_t*   p_media;        ///< 媒体数据
    };
}klb_ncm_ops_mnp_t;

//////////////////////////////////////////////////////////////////////////
static void klb_ncm_ops_mnp_send_heart(klb_ncm_ops_mnp_t* p_ops, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_ops->cb_malloc(p_ops->p_pool, sizeof(klb_mnp_t));

    klb_mnp_t* p_mnp = (klb_mnp_t*)p_buf->p_buf;

    p_mnp->magic = KLB_MNP_MAGIC;
    p_mnp->size = sizeof(klb_mnp_t);
    p_mnp->opt = KLB_MNP_FULL;
    p_mnp->packtype = KLB_MNP_PONG;

    p_buf->end = sizeof(klb_mnp_t);

    klb_nlist_push_tail(p_ops->p_w_list, p_buf);
    klb_socket_set_writing(p_socket, true);   // 有数据可写
}

static int klb_ncm_ops_mnp_parse(klb_ncm_ops_mnp_t* p_ops, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_ops->p_r_buf;

    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    assert(0 <= p_ops->left_len);
    if (p_ops->left_len <= 0)
    {
        // 解析头
        if (data_len < sizeof(klb_mnp_t))
        {
            return 1; // 数据长度不足
        }

        klb_mnp_t mnp = { 0 };
        memcpy(&mnp, p_buf->p_buf + p_buf->start, sizeof(klb_mnp_t));
        p_buf->start += sizeof(klb_mnp_t);

        if (KLB_MNP_PONG == mnp.packtype)
        {
            klb_ncm_ops_mnp_send_heart(p_ops, p_socket);
        }

        p_ops->mnp = mnp;

        p_ops->left_len = mnp.size - sizeof(klb_mnp_t);
        return 0;
    }
    else
    {
        int r_len = MIN(p_ops->left_len, data_len);
        p_ops->left_len -= r_len;

        if (KLB_MNP_TEXT == p_ops->mnp.packtype)
        {
            klb_buffer_write(p_ops->p_txt, p_buf->p_buf + p_buf->start, r_len);

            if (p_ops->left_len <= 0)
            {
                if (KLB_MNP_FULL == p_ops->mnp.opt || KLB_MNP_END == p_ops->mnp.opt)
                {
                    // 完整了
                    klb_buf_t* p_txt = klb_buffer_join(p_ops->p_txt, p_ops->cb_malloc, p_ops->p_pool);

                    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_txt->p_buf + p_txt->start);
                    assert(p_txt->end - p_txt->start == p_com->size);
                    p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_TEXT, p_txt);
                    klb_buf_unref_next(p_txt);

                    klb_buffer_reset(p_ops->p_txt);
                }
            }
        }
        else if (KLB_MNP_BINARY == p_ops->mnp.packtype)
        {
            klb_buffer_write(p_ops->p_txt, p_buf->p_buf + p_buf->start, r_len);

            if (p_ops->left_len <= 0)
            {
                if (KLB_MNP_FULL == p_ops->mnp.opt || KLB_MNP_END == p_ops->mnp.opt)
                {
                    // 完整了
                    klb_buf_t* p_bin = klb_buffer_join(p_ops->p_txt, p_ops->cb_malloc, p_ops->p_pool);
                    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_bin->p_buf + p_bin->start);
                    assert(p_bin->end - p_bin->start == p_com->size);

                    p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_BINARY, p_bin);
                    klb_buf_unref_next(p_bin);

                    klb_buffer_reset(p_ops->p_txt);
                }
            }
        }
        else if (KLB_MNP_MEDIA == p_ops->mnp.packtype)
        {
            klb_buffer_write(p_ops->p_media, p_buf->p_buf + p_buf->start, r_len);

            if (p_ops->left_len <= 0)
            {
                if (KLB_MNP_FULL == p_ops->mnp.opt || KLB_MNP_END == p_ops->mnp.opt)
                {
                    // 数完整了
                    klb_buf_t* p_media = klb_buffer_join(p_ops->p_media, p_ops->cb_malloc_media, p_ops->p_pool_media);

                    p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_MEDIA, p_media);
                    klb_buf_unref_next(p_media);

                    klb_buffer_reset(p_ops->p_media);
                }
            }
        }

        p_buf->start += r_len;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

/// @brief 创建连接
/// @param [in] *p_ncm          ncm模块
/// @return void* 连接的指针
static void* cb_create_klb_ncm_ops_mnp(klb_ncm_ops_lparam_t* p_lparam, klb_ncm_ops_wparam_t* p_wparam)
{
    klb_ncm_ops_mnp_t* p_ops = KLB_MALLOCZ(klb_ncm_ops_mnp_t, 1, 0);

    p_ops->p_ncm = (klb_ncm_t*)p_lparam->p_ncm;
    p_ops->protocol = p_lparam->protocol;
    p_ops->id = p_lparam->id;
    p_ops->cb_recv_to_ncm = p_lparam->cb_recv;
    p_ops->cb_malloc = p_lparam->cb_malloc;
    p_ops->p_pool = p_lparam->p_pool;
    p_ops->cb_malloc_media = p_lparam->cb_malloc_media;
    p_ops->p_pool_media = p_lparam->p_pool_media;

    // send发送相关
    p_ops->p_w_list = klb_nlist_create();

    // recv接收相关
    p_ops->p_r_buf = klb_buf_malloc(KLB_MNP_BLOCK_SIZE_MAX, false);

    // txt/bin缓存
    p_ops->p_txt = klb_buffer_create(KLB_MNP_BLOCK_SIZE_MAX);

    // media缓存
    p_ops->p_media = klb_buffer_create(KLB_MNP_BLOCK_SIZE_MAX);

    return p_ops;
}

/// @brief 销毁连接
/// @param [in] *ptr            连接的指针
/// @return 无
static void cb_destroy_klb_ncm_ops_mnp(void* ptr)
{
    klb_ncm_ops_mnp_t* p_opt = (klb_ncm_ops_mnp_t*)ptr;

    // w
    while (0 < klb_nlist_size(p_opt->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_opt->p_w_list);
        klb_buf_unref_next(p_tmp);
    }

    if (NULL != p_opt->p_w_cur)
    {
        klb_buf_unref_next(p_opt->p_w_cur);
        p_opt->p_w_cur = NULL;
    }

    // 销毁
    KLB_FREE_BY(p_opt->p_media, klb_buffer_destroy);
    KLB_FREE_BY(p_opt->p_txt, klb_buffer_destroy);
    KLB_FREE_BY(p_opt->p_w_list, klb_nlist_destroy);
    KLB_FREE_BY(p_opt->p_r_buf, free);

    KLB_FREE(p_opt);
}


static int cb_init_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    klb_buf_t* p_buf = p_ops->p_r_buf;
    assert(data_len <= p_buf->buf_len);

    memcpy(p_buf->p_buf, p_data, data_len);
    p_buf->end = data_len;


    while (true)
    {
        if (0 != klb_ncm_ops_mnp_parse(p_ops, p_socket))
        {
            break;
        }
    }

    return 0;
}

/// @brief 主动发送文本数据
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_text_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    int head_len = sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t);
    int totol_len = head_len + extra_len + data_len;
    assert(totol_len <= KLB_MNP_BLOCK_SIZE_MAX);

    klb_buf_t* p_buf = p_ops->cb_malloc(p_ops->p_pool, KLB_ALIGNED_4(totol_len));

    klb_mnp_t* p_mnp = (klb_mnp_t*)p_buf->p_buf;
    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + sizeof(klb_mnp_t));

    p_mnp->magic = KLB_MNP_MAGIC;
    p_mnp->size = totol_len;
    p_mnp->opt = KLB_MNP_FULL;
    p_mnp->packtype = KLB_MNP_TEXT;

    p_com->size = totol_len - sizeof(klb_mnp_t);
    p_com->head = extra_len;
    p_com->sequence = sequence;
    p_com->uid = uid;

    if (0 < extra_len)
    {
        memcpy(p_buf->p_buf + head_len, p_extra, extra_len);
    }

    if (0 < data_len)
    {
        memcpy(p_buf->p_buf + head_len + extra_len, p_data, data_len);
    }

    p_buf->end = totol_len;

    klb_nlist_push_tail(p_ops->p_w_list, p_buf);

    klb_socket_set_writing(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 主动发送二进制数据
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_binary_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    int head_len = sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t);
    int totol_len = head_len + extra_len + data_len;
    assert(totol_len <= KLB_MNP_BLOCK_SIZE_MAX);

    klb_buf_t* p_buf = p_ops->cb_malloc(p_ops->p_pool, KLB_ALIGNED_4(totol_len));

    klb_mnp_t* p_mnp = (klb_mnp_t*)p_buf->p_buf;
    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + sizeof(klb_mnp_t));

    p_mnp->magic = KLB_MNP_MAGIC;
    p_mnp->size = totol_len;
    p_mnp->opt = KLB_MNP_FULL;
    p_mnp->packtype = KLB_MNP_BINARY;

    p_com->size = totol_len - sizeof(klb_mnp_t);
    p_com->head = extra_len;
    p_com->sequence = sequence;
    p_com->uid = uid;

    if (0 < extra_len)
    {
        memcpy(p_buf->p_buf + head_len, p_extra, extra_len);
    }

    if (0 < data_len)
    {
        memcpy(p_buf->p_buf + head_len + extra_len, p_data, data_len);
    }

    p_buf->end = totol_len;

    klb_nlist_push_tail(p_ops->p_w_list, p_buf);

    klb_socket_set_writing(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 主动发送媒体数据
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_media_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    klb_buf_ref_next(p_data);

    klb_nlist_push_tail(p_ops->p_w_list, p_data);
    klb_socket_set_writing(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 当网络上可以发送数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_send_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;
    bool first = true;

    while (true)
    {
        if (NULL == p_ops->p_w_cur && 0 < klb_nlist_size(p_ops->p_w_list))
        {
            p_ops->p_w_cur = (klb_buf_t*)klb_nlist_pop_head(p_ops->p_w_list);
            if (NULL != p_ops->p_w_cur)
            {
                p_ops->w_start = p_ops->p_w_cur->start;
            }
        }

        klb_buf_t* p_buf = p_ops->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, (const uint8_t*)(p_buf->p_buf + p_ops->w_start), p_buf->end - p_ops->w_start);
        if (0 < w)
        {
            p_ops->w_start += w;
            send += w;

            if (p_buf->end <= p_ops->w_start)
            {
                p_ops->p_w_cur = p_buf->p_next;
                if (NULL != p_ops->p_w_cur)
                {
                    p_ops->w_start = p_ops->p_w_cur->start;
                }

                klb_buf_unref(p_buf);
            }
        }
        else
        {
            if (first)
            {
                // select成功, 却第一次无法写, 表明对方已经断开
                err = KLB_SOCKET_DISCONNECT;
            }

            break; // 无法再发送
        }

        first = false;
    }

    if (NULL == p_ops->p_w_cur && klb_nlist_size(p_ops->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return send;
}

/// @brief 当网络上可以接收数据时
/// @param [in] *ptr            连接的指针
/// @return int
static int on_recv_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, int64_t now)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    if (KLB_SOCKET_OK != klb_socket_get_status(p_socket))
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int recv = 0;
    bool first = true;

    while (true)
    {
        klb_buf_t* p_buf = p_ops->p_r_buf;

        int r = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            while (true)
            {
                if (0 != klb_ncm_ops_mnp_parse(p_ops, p_socket))
                {
                    break;
                }
            }

            if (p_buf->end <= p_buf->start)
            {
                p_buf->start = 0;
                p_buf->end = 0;
            }
            else if (0 < p_buf->start)
            {
                memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
                p_buf->end = p_buf->end - p_buf->start;
                p_buf->start = 0;
            }
        }
        else
        {
            if (first)
            {
                // select成功, 却第一次读不到数据, 表明对方已断开 
                err = KLB_SOCKET_DISCONNECT;
            }
            break; // 无法再接收
        }

        first = false;
    }

    if (KLB_SOCKET_OK != err)
    {
        klb_socket_set_status(p_socket, err);
        p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, err, 0, NULL);
    }

    return recv;
}

//////////////////////////////////////////////////////////////////////////

int klb_ncm_register_ops_mnp(klb_ncm_t* p_ncm, int protocol)
{
    klb_ncm_ops_t ops = { 0 };

    // 创建/销毁
    ops.cb_create = cb_create_klb_ncm_ops_mnp;
    ops.cb_destroy = cb_destroy_klb_ncm_ops_mnp;

    ops.cb_init = cb_init_klb_ncm_ops_mnp;
    ops.cb_ctrl = NULL;

    //ops.cb_send_text = cb_send_text_klb_ncm_ops_mnp;
    //ops.cb_send_binary = cb_send_binary_klb_ncm_ops_mnp;
    ops.cb_send_media = cb_send_media_klb_ncm_ops_mnp;

    ops.on_send = on_send_klb_ncm_ops_mnp;
    ops.on_recv = on_recv_klb_ncm_ops_mnp;

    int ret = klb_ncm_register(p_ncm, protocol, &ops);
    assert(0 == ret);

    return ret;
}
