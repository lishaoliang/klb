#include "klbnet/klb_ncm_ops/klb_ncm_ops.h"
#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_list.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbbase/klb_mnp.h"
#include "klbutil/klb_log.h"
#include <assert.h>


typedef struct klb_ncm_ops_mnp_t_
{
    klb_ncm_t*          p_ncm;          ///< ncm模块
    int                 protocol;       ///< 协议编号
    int                 id;             ///< id编号

    klb_ncm_ops_recv_cb  cb_recv_to_ncm; ///< 收到数据后, 交给ncm

    // send发送相关
    struct
    {
        klb_list_t*     p_w_list;       ///< 待发送列表: klb_buf_t*
        klb_buf_t*      p_w_cur;        ///< 当前正在发送的缓存
        int             w_len;          ///< 当前等待发送的缓存量
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
    klb_buf_t* p_buf = klb_buf_malloc(sizeof(klb_mnp_t), false);

    klb_mnp_t* p_mnp = (klb_mnp_t*)p_buf->p_buf;

    p_mnp->magic = KLB_MNP_MAGIC;
    p_mnp->size = sizeof(klb_mnp_t);
    p_mnp->opt = KLB_MNP_FULL;
    p_mnp->packtype = KLB_MNP_HEART;

    p_buf->end = sizeof(klb_mnp_t);

    klb_list_push_tail(p_ops->p_w_list, p_buf);
    klb_socket_set_sending(p_socket, true);   // 有数据可写
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

        if (KLB_MNP_HEART == mnp.packtype)
        {
            klb_ncm_ops_mnp_send_heart(p_ops, p_socket);
        }

        p_ops->mnp = mnp;
        //KLB_LOG("packtype:[%d],opt:[%d],size:[%d]\n", mnp.packtype, mnp.opt, mnp.size);

        p_ops->left_len = mnp.size - sizeof(klb_mnp_t);
        return 0;
    }
    else
    {
        int r_len = MIN(p_ops->left_len, data_len);
        p_ops->left_len -= r_len;

        if (KLB_MNP_TXT == p_ops->mnp.packtype)
        {
            klb_buffer_write(p_ops->p_txt, p_buf->p_buf + p_buf->start, r_len);

            if (p_ops->left_len <= 0)
            {
                if (KLB_MNP_FULL == p_ops->mnp.opt || KLB_MNP_END == p_ops->mnp.opt)
                {
                    // 数完整了
                    klb_buf_t* p_txt = klb_buffer_join(p_ops->p_txt);
                    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_txt->p_buf + p_txt->start);
                    assert(p_txt->end - p_txt->start == p_com->size);

                    //KLB_LOG("extra=[%d],data len=[%d]\n", p_com->extra, p_com->size - p_com->extra - sizeof(klb_mnp_common_t));
                    //KLB_FREE(p_txt);
                    p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_TEXT, p_txt);

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
                    klb_buf_t* p_media = klb_buffer_join(p_ops->p_media);

                    p_ops->cb_recv_to_ncm(p_ops->p_ncm, p_ops->protocol, p_ops->id, 0, KLB_NCM_PACK_MEDIA, p_media);

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
static void* cb_create_klb_ncm_ops_mnp(klb_ncm_t* p_ncm, klb_ncm_ops_recv_cb cb_recv, int protocol, int id)
{
    klb_ncm_ops_mnp_t* p_ops = KLB_MALLOC(klb_ncm_ops_mnp_t, 1, 0);
    KLB_MEMSET(p_ops, 0, sizeof(klb_ncm_ops_mnp_t));

    p_ops->p_ncm = p_ncm;
    p_ops->protocol = protocol;
    p_ops->id = id;
    p_ops->cb_recv_to_ncm = cb_recv;

    // send发送相关
    p_ops->p_w_list = klb_list_create();

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
    while (0 < klb_list_size(p_opt->p_w_list))
    {
        klb_buf_t* p_tmp = klb_list_pop_head(p_opt->p_w_list);
        KLB_FREE(p_tmp);
    }

    // 销毁
    KLB_FREE_BY(p_opt->p_media, klb_buffer_destroy);
    KLB_FREE_BY(p_opt->p_txt, klb_buffer_destroy);
    KLB_FREE_BY(p_opt->p_w_cur, free);
    KLB_FREE_BY(p_opt->p_w_list, klb_list_destroy);
    KLB_FREE_BY(p_opt->p_r_buf, free);

    KLB_FREE(p_opt);
}

/// @brief 主动发送数据(非媒体数据)
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

    int head_len = sizeof(klb_mnp_t) + sizeof(klb_mnp_common_t);
    int totol_len = head_len + extra_len + data_len;
    assert(totol_len <= KLB_MNP_BLOCK_SIZE_MAX);

    klb_buf_t* p_buf = klb_buf_malloc(KLB_ALIGNED_4(totol_len), false);

    klb_mnp_t* p_mnp = (klb_mnp_t*)p_buf->p_buf;
    klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + sizeof(klb_mnp_t));

    p_mnp->magic = KLB_MNP_MAGIC;
    p_mnp->size = totol_len;
    p_mnp->opt = KLB_MNP_FULL;
    p_mnp->packtype = KLB_MNP_TXT;

    p_com->size = totol_len - sizeof(klb_mnp_t);
    p_com->extra = extra_len;
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

    klb_list_push_tail(p_ops->p_w_list, p_buf);

    klb_socket_set_sending(p_socket, true);   // 有数据可写

    return 0;
}

/// @brief 主动发送媒体数据
/// @param [in] *ptr            连接的指针
/// @return int
static int cb_send_media_klb_ncm_ops_mnp(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data)
{
    klb_ncm_ops_mnp_t* p_ops = (klb_ncm_ops_mnp_t*)ptr;

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

    while (true)
    {
        if (NULL == p_ops->p_w_cur && 0 < klb_list_size(p_ops->p_w_list))
        {
            p_ops->p_w_cur = (klb_buf_t*)klb_list_pop_head(p_ops->p_w_list);
        }

        klb_buf_t* p_buf = p_ops->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        if (0 < w)
        {
            send += w;
            p_buf->start += w;

            if (p_buf->end <= p_buf->start)
            {
                p_ops->p_w_cur = NULL;
                KLB_FREE(p_buf);
            }
        }
        else if (0 == w)
        {
            break; // 无法再发送
        }
        else
        {
            err = KLB_SOCKET_ERR;
            break; // 出现错误
        }
    }

    if (NULL == p_ops->p_w_cur && klb_list_size(p_ops->p_w_list) <= 0)
    {
        klb_socket_set_sending(p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        //klua_khttp_error(p_khttp, err);
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

    while (true)
    {
        klb_buf_t* p_buf = p_ops->p_r_buf;

        int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;

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
        else if (0 == r)
        {
            break; // 无法再接收
        }
        else
        {
            err = KLB_SOCKET_ERR;
            break; // 出现错误
        }
    }

    if (KLB_SOCKET_OK != err)
    {
        //klua_khttp_error(p_khttp, err);
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

    ops.cb_send = cb_send_klb_ncm_ops_mnp;
    ops.cb_send_media = cb_send_media_klb_ncm_ops_mnp;

    ops.on_send = on_send_klb_ncm_ops_mnp;
    ops.on_recv = on_recv_klb_ncm_ops_mnp;

    int ret = klb_ncm_register(p_ncm, protocol, &ops);
    assert(0 == ret);

    return ret;
}
