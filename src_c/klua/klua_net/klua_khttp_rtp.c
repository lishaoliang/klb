// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbutil/klb_list.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_help.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_log.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbthird/http_parser.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// http-rtp
#define KLUA_KHTTP_RTP_HANDLE            "KLUA_KHTTP_RTP_HANDLE*"     ///< Lua meta标示


typedef struct klua_khttp_rtp_t_ klua_khttp_rtp_t;


// 参数
typedef struct klua_khttp_rtp_param_t_
{
    enum http_parser_type   http_type;  ///< http类型

    int     rbuf_max;       ///< 临时读取缓存最大值
    int     read_max;       ///< 缓存数据最大值后,暂停读取
    bool    tls;            ///< TLS
}klua_khttp_rtp_param_t;


// C
typedef struct klua_khttp_rtp_inter_t_
{
    klua_khttp_rtp_param_t           param;          ///< 参数

    klua_khttp_rtp_t*                p_ktcp;         ///< ktcp
    bool                        close;          ///< 是否关闭: true.关闭; false.未关闭

                                                // socket相关
    struct
    {
        klb_socket_t*           p_socket;       ///< socket
        int                     id;             ///< 被分配的ID, 大于0的正整数
    };

    // send发送相关
    struct
    {
        klb_list_t*             p_w_list;       ///< 待发送数据列表: klb_buf_t*
        klb_buf_t*              p_w_cur;        ///< 当前正在发送的缓存
        int                     w_start;        ///< 当前发送的起始点
    };

    // recv接收相关
    struct
    {
        klb_buf_t*              p_r_buf;        ///< 临时读取缓存

        klb_list_t*             p_r_list;       ///< 
    };

    // http解析
    struct
    {
        int                     parser_status;  ///< klua_khttp_parser_status_e

        http_parser             parser;         ///< HTTP解析器
        http_parser_settings    settings;       ///< 解析器设置

        klb_buffer_t*           p_body;         ///< 
        int                     body_len;       ///< 

        sds                     header_field;   ///< 
        uint64_t                content_length; ///< 
    };

    // mnp解析
    struct
    {
        klb_mnp_t               mnp;            ///< mnp头
        int                     remain_len;     ///< 剩下的数据长度

        klb_buffer_t*           p_txt;          ///< 文本/二进制
        klb_buffer_t*           p_media;        ///< 媒体数据
    };
}klua_khttp_rtp_inter_t;


typedef struct klua_khttp_rtp_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_recv;        ///< co_recv协程
    };

    struct
    {
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    klua_khttp_rtp_inter_t*          p_inter;        ///< C内部实现
}klua_khttp_rtp_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_co_recv_klua_khttp_rtp(klua_khttp_rtp_t* p_kmnp, const char* p_type, klb_buf_t* p_buf)
{
    assert(NULL != p_kmnp);

    if (NULL != p_kmnp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kmnp->p_env), p_kmnp->co_recv);
        if (NULL == L) return -1; // 未处理

        p_kmnp->co_recv = NULL; // 清空

        klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + p_buf->start);
        assert(p_buf->end - p_buf->start == p_com->size);

        lua_pushstring(L, p_type);                                                  // 类型
        lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t), p_com->head); // head
        lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t) + p_com->head, p_com->size - p_com->head - sizeof(klb_mnp_common_t)); // body

        int status = lua_pcall(L, 3, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static void free_klua_khttp_rtp_inter(klua_khttp_rtp_inter_t* p_inter)
{
    // 清空
    while (0 < klb_list_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_list_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    while (0 < klb_list_size(p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_list_pop_head(p_inter->p_r_list);
        KLB_FREE(p_tmp);
    }

    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);
    KLB_FREE_BY(p_inter->p_w_list, klb_list_destroy);
    KLB_FREE_BY(p_inter->p_r_list, klb_list_destroy);
    KLB_FREE_BY(p_inter->p_txt, klb_buffer_destroy);
    KLB_FREE_BY(p_inter->p_media, klb_buffer_destroy);

    KLB_FREE(p_inter->p_w_cur);
    KLB_FREE(p_inter->p_r_buf);
    KLB_FREE(p_inter);
}

////////////////////////////////////////
// mnp 解析

static int klua_khttp_rtp_parse(klua_khttp_rtp_t* p_kmnp, klua_khttp_rtp_inter_t* p_inter, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_inter->p_r_buf;

    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    assert(0 <= p_inter->remain_len);
    if (p_inter->remain_len <= 0)
    {
        assert(0 == p_inter->remain_len);

        // 解析头
        if (data_len < sizeof(klb_mnp_t))
        {
            return 1; // 数据长度不足
        }

        klb_mnp_t mnp = { 0 };
        memcpy(&mnp, p_buf->p_buf + p_buf->start, sizeof(klb_mnp_t));

        assert(KLB_MNP_MAGIC == mnp.magic);
        assert(mnp.packtype <= KLB_MNP_MEDIA);

        p_buf->start += sizeof(klb_mnp_t);

        if (KLB_MNP_PONG == mnp.packtype)
        {
            //klb_ncm_ops_mnp_send_heart(p_inter, p_socket);
        }

        p_inter->mnp = mnp;

        p_inter->remain_len = mnp.size - sizeof(klb_mnp_t);
        return 0;
    }
    else
    {
        int r_len = MIN(p_inter->remain_len, data_len);
        p_inter->remain_len -= r_len;

        if (KLB_MNP_TEXT == p_inter->mnp.packtype)
        {
            klb_buffer_write(p_inter->p_txt, p_buf->p_buf + p_buf->start, r_len);

            if (p_inter->remain_len <= 0)
            {
                if (KLB_MNP_FULL == p_inter->mnp.opt || KLB_MNP_END == p_inter->mnp.opt)
                {
                    // 完整了
                    klb_buf_t* p_txt = klb_buffer_join_offset(p_inter->p_txt, sizeof(int), 0, NULL, NULL);
                    *((int*)p_txt->p_buf) = KLB_MNP_TEXT;

                    if (call_lua_co_recv_klua_khttp_rtp(p_kmnp, "text", p_txt) < 0)
                    {
                        klb_list_push_tail(p_inter->p_r_list, p_txt);
                    }
                    else
                    {
                        KLB_FREE(p_txt);
                    }

                    klb_buffer_reset(p_inter->p_txt);
                }
            }
        }
        else if (KLB_MNP_BINARY == p_inter->mnp.packtype)
        {
            klb_buffer_write(p_inter->p_txt, p_buf->p_buf + p_buf->start, r_len);

            if (p_inter->remain_len <= 0)
            {
                if (KLB_MNP_FULL == p_inter->mnp.opt || KLB_MNP_END == p_inter->mnp.opt)
                {
                    // 完整了
                    klb_buf_t* p_bin = klb_buffer_join_offset(p_inter->p_txt, sizeof(int), 0, NULL, NULL);
                    *((int*)p_bin->p_buf) = KLB_MNP_BINARY;

                    if (call_lua_co_recv_klua_khttp_rtp(p_kmnp, "binary", p_bin) < 0)
                    {
                        klb_list_push_tail(p_inter->p_r_list, p_bin);
                    }
                    else
                    {
                        KLB_FREE(p_bin);
                    }

                    klb_buffer_reset(p_inter->p_txt);
                }
            }
        }
        else if (KLB_MNP_MEDIA == p_inter->mnp.packtype)
        {
            assert(false);

            klb_buffer_write(p_inter->p_media, p_buf->p_buf + p_buf->start, r_len);

            if (p_inter->remain_len <= 0)
            {
                if (KLB_MNP_FULL == p_inter->mnp.opt || KLB_MNP_END == p_inter->mnp.opt)
                {
                    // 数完整了
                    //klb_buf_t* p_media = klb_buffer_join(p_inter->p_media, p_inter->cb_malloc_media, p_inter->p_pool_media);

                    //p_inter->cb_recv_to_ncm(p_inter->p_ncm, p_inter->protocol, p_inter->id, 0, KLB_NCM_PACK_MEDIA, p_media);
                    //klb_buf_unref_next(p_media);

                    klb_buffer_reset(p_inter->p_media);
                }
            }
        }
        else
        {
            assert(false);
        }

        p_buf->start += r_len;
    }

    return 0;
}

////////////////////////////////////////
// http 解析


static int on_message_begin_klua_khttp_rtp(http_parser* p_parser)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;
    //KLB_LOG("on_message_begin\n");

    return 0;
}

static int on_url_klua_khttp_rtp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_status_klua_khttp_rtp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_header_field_klua_khttp_rtp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_header_value_klua_khttp_rtp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_headers_complete_klua_khttp_rtp(http_parser* p_parser)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_body_klua_khttp_rtp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_message_complete_klua_khttp_rtp(http_parser* p_parser)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_chunk_header_klua_khttp_rtp(http_parser* p_parser)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;

    return 0;
}

static int on_chunk_complete_klua_khttp_rtp(http_parser* p_parser)
{
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_parser->data;
    klua_khttp_rtp_t* p_khttp = p_inter->p_ktcp;


    return 0;
}

////////////////////////////////////////
// klb_multiplex_ops_t callback

static int cb_remove_klua_khttp_rtp(void* p_lparam, void* p_wparam, int id)
{
    //klua_ktcp_t* p_kmnp = (klua_ktcp_t*)p_lparam;
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    KLB_FREE_BY(p_inter, free_klua_khttp_rtp_inter);

    return 0;
}

static int cb_recv_klua_khttp_rtp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_rtp_t* p_kmnp = (klua_khttp_rtp_t*)p_lparam;
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_wparam;

    klb_socket_t* p_socket = p_inter->p_socket;

    if (p_inter->close || KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int recv = 0;
    bool first = true;

    while (true)
    {
        klb_buf_t* p_buf = p_inter->p_r_buf;

        int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            while (true)
            {
                if (0 != klua_khttp_rtp_parse(p_kmnp, p_inter, p_socket))
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
                // select成功, 却第一次读不到数据
                err = KLB_SOCKET_DISCONNECT;
            }
            break; // 无法再接收
        }

        first = false;
    }

    if (KLB_SOCKET_OK != err)
    {
        assert(false);
    }

    return recv;
}

static int cb_send_klua_khttp_rtp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_rtp_t* p_kmnp = (klua_khttp_rtp_t*)p_lparam;
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_wparam;

    klb_socket_t* p_socket = p_inter->p_socket;

    if (p_inter->close || KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;
    bool first = true;

    while (true)
    {
        if (NULL == p_inter->p_w_cur && 0 < klb_list_size(p_inter->p_w_list))
        {
            p_inter->p_w_cur = (klb_buf_t*)klb_list_pop_head(p_inter->p_w_list);
        }

        klb_buf_t* p_buf = p_inter->p_w_cur;
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
                p_inter->p_w_cur = NULL;
                KLB_FREE(p_buf);
            }
        }
        else
        {
            if (first)
            {
                // select成功, 却第一次无法写, 出现错误
                err = KLB_SOCKET_DISCONNECT;
            }

            break; // 无法再发送
        }

        first = false;
    }

    if (NULL == p_inter->p_w_cur && klb_list_size(p_inter->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_inter->p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        assert(false);
    }

    return send;
}

static int cb_timer_klua_khttp_rtp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_rtp_t* p_kmnp = (klua_khttp_rtp_t*)p_lparam;
    klua_khttp_rtp_inter_t* p_inter = (klua_khttp_rtp_inter_t*)p_wparam;

    if (p_inter->close)
    {
        return 0;
    }

    return 0;
}

////////////////////////////////////////
// lua export

static klua_khttp_rtp_t* new_klua_khttp_rtp(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = (klua_khttp_rtp_t*)lua_newuserdata(L, sizeof(klua_khttp_rtp_t));
    KLB_MEMSET(p_kmnp, 0, sizeof(klua_khttp_rtp_t));
    luaL_setmetatable(L, KLUA_KHTTP_RTP_HANDLE);
    return p_kmnp;
}

static klua_khttp_rtp_t* to_klua_khttp_rtp(lua_State* L, int index)
{
    klua_khttp_rtp_t* p_kmnp = (klua_khttp_rtp_t*)luaL_checkudata(L, index, KLUA_KHTTP_RTP_HANDLE);
    luaL_argcheck(L, NULL != p_kmnp, index, "'ktcp' expected");
    return p_kmnp;
}

static int klua_khttp_rtp_tostring(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);

    lua_pushfstring(L, "ktcp:%p", p_kmnp);
    return 1;
}

static int klua_khttp_rtp_close(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);

    if (NULL != p_kmnp->p_inter)
    {
        p_kmnp->p_inter->close = true;
        klb_socket_closing(p_kmnp->p_inter->p_socket);

        klb_multiplex_remove(p_kmnp->p_multi, p_kmnp->p_inter->id);

        // p_ktcp->p_inter 由 remove 后释放
        p_kmnp->p_inter = NULL;
    }

    return 0;
}

static int klua_khttp_rtp_send_text(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    size_t body_len = 0;
    const char* p_body = luaL_checklstring(L, 3, &body_len);

    klb_buf_t* p_buf = klb_mnp_pack_text(0, 0, p_head, head_len, p_body, body_len);

    if (NULL != p_buf)
    {
        klb_list_push_tail(p_kmnp->p_inter->p_w_list, p_buf);
        klb_socket_set_writing(p_kmnp->p_inter->p_socket, true);
    }

    return 0;
}

static int klua_khttp_rtp_send_binary(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    size_t body_len = 0;
    const char* p_body = luaL_checklstring(L, 3, &body_len);

    klb_buf_t* p_buf = klb_mnp_pack_binary(0, 0, p_head, head_len, p_body, body_len);

    if (NULL != p_buf)
    {
        klb_list_push_tail(p_kmnp->p_inter->p_w_list, p_buf);
        klb_socket_set_writing(p_kmnp->p_inter->p_socket, true);
    }

    return 0;
}

static int klua_khttp_rtp_send_media(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);

    return 0;
}

static int klua_khttp_rtp_co_recv(lua_State* L)
{
    klua_khttp_rtp_t* p_kmnp = to_klua_khttp_rtp(L, 1);
    klua_check_coroutine(L, "co_recv must in coroutine!");

    if (0 < klb_list_size(p_kmnp->p_inter->p_r_list))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_list_pop_head(p_kmnp->p_inter->p_r_list);

        klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + p_buf->start);
        assert(p_buf->end - p_buf->start == p_com->size);

        int t = *((int*)p_buf->p_buf);

        lua_pushstring(L, (KLB_MNP_TEXT == t) ? "text" : "binary");      // 类型

        lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t), p_com->head); // head
        lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t) + p_com->head, p_com->size - p_com->head - sizeof(klb_mnp_common_t)); // body

        KLB_FREE(p_buf->p_buf);
        KLB_FREE(p_buf);

        return 3;
    }

    assert(NULL == p_kmnp->co_recv);
    p_kmnp->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_khttp_rtp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_khttp_rtp_close },

        { "send_text",      klua_khttp_rtp_send_text },
        { "send_binary",    klua_khttp_rtp_send_binary },
        { "send_media",     klua_khttp_rtp_send_media },

        { "co_recv",        klua_khttp_rtp_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_khttp_rtp_close },
        { "__close",         klua_khttp_rtp_close },
        { "__tostring",      klua_khttp_rtp_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTP_RTP_HANDLE);/* metatable for KLUA_KUDP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

#define KLUA_KHTTP_RTP_LISTEN_HANDLE    "KLUA_KHTTP_RTP_LISTEN_HANDLE*"

typedef struct klua_khttp_rtp_listen_param_t_
{
    int max_connect;                            ///< 操作系统监听socket, 能缓存的最大连接数
}klua_khttp_rtp_listen_param_t;

typedef struct klua_khttp_rtp_listen_t_
{
    klua_khttp_rtp_listen_param_t    param;          ///< 参数

                                                     // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_accept;      ///< sync的"co_accept"函数对应的协程
    };

    // 其他扩展等
    struct
    {
        klb_multiplex_t*        p_multi;        ///< 复用
        klua_ex_coroutine_t*    p_ex_coroutine; ///< ex co
    };

    klb_listen_t*               p_listen;       ///< C
}klua_khttp_rtp_listen_t;

////////////////////////////////////////
static klua_khttp_rtp_listen_t* new_klua_khttp_rtp_listen(lua_State* L)
{
    klua_khttp_rtp_listen_t* p_listen = (klua_khttp_rtp_listen_t*)lua_newuserdata(L, sizeof(klua_khttp_rtp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_khttp_rtp_listen_t));
    luaL_setmetatable(L, KLUA_KHTTP_RTP_LISTEN_HANDLE);
    return p_listen;
}

static klua_khttp_rtp_listen_t* to_klua_khttp_rtp_listen(lua_State* L, int index)
{
    klua_khttp_rtp_listen_t* p_listen = (klua_khttp_rtp_listen_t*)luaL_checkudata(L, index, KLUA_KHTTP_RTP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ktcp listen' expected");
    return p_listen;
}

static int klua_khttp_rtp_listen_tostring(lua_State* L)
{
    klua_khttp_rtp_listen_t* p_listen = to_klua_khttp_rtp_listen(L, 1);

    lua_pushfstring(L, "ktcp.listen:%p,port:%d", p_listen, klb_listen_port(p_listen->p_listen));
    return 1;
}

static int klua_khttp_rtp_listen_close(lua_State* L)
{
    klua_khttp_rtp_listen_t* p_listen = to_klua_khttp_rtp_listen(L, 1);

    if (NULL != p_listen->p_listen)
    {
        klb_listen_close(p_listen->p_listen);
        KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    }

    return 0;
}

static int klua_khttp_rtp_listen_co_accept(lua_State* L)
{
    klua_khttp_rtp_listen_t* p_listen = to_klua_khttp_rtp_listen(L, 1);
    klua_check_coroutine(L, "kmnp co_accept must in coroutine!");

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////

static void klua_khttp_rtp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_khttp_rtp_listen_close },       ///< 

        { "co_accept",      klua_khttp_rtp_listen_co_accept },  ///< 
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_khttp_rtp_listen_close },
        { "__close",         klua_khttp_rtp_listen_close },
        { "__tostring",      klua_khttp_rtp_listen_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTP_RTP_LISTEN_HANDLE);/* metatable for KLUA_KTCP_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


#define KLUA_KHTTP_MNP_RBUF_MAX          (1024 * 8)              ///< 临时读取缓存最大值
#define KLUA_KHTTP_MNP_READ_MAX          (1024 * 512)            ///< 缓存最大值后,暂停读取

static void default_param_klua_khttp_rtp(klua_khttp_rtp_param_t* p_param)
{
    p_param->http_type = HTTP_RESPONSE;
    p_param->rbuf_max = KLUA_KHTTP_MNP_RBUF_MAX; // 临时读取缓存最大值
    p_param->read_max = KLUA_KHTTP_MNP_READ_MAX; // 缓存最大值后,暂停读取
    p_param->tls = false;
}

static void check_param_klua_khttp_rtp(lua_State* L, int from, klua_khttp_rtp_param_t* p_param)
{
    static const char* options[] = { "rbuf_max", "read_max", "tls", NULL };

    int num = lua_gettop(L) - from;

    for (int i = 1; i < num; i += 2)
    {
        int idx = luaL_checkoption(L, from + i, NULL, options);
        int ud = from + i + 1;
        switch (idx)
        {
        case 0:
            p_param->rbuf_max = luaL_checkinteger(L, ud);
            break;
        case 1:
            p_param->read_max = luaL_checkinteger(L, ud);
            break;
        case 2:
            p_param->tls = luaL_checkboolean(L, ud);
            break;
        default:
            break;
        }
    }
}

klua_khttp_rtp_t* new_connect_klua_khttp_rtp(lua_State* L, klb_socket_fd fd, klua_khttp_rtp_param_t* p_param)
{
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = NULL;
    if (p_param->tls)
    {
        p_socket = klb_socket_tls_async_create(fd, false, NULL);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    klua_khttp_rtp_inter_t* p_inter = KLB_MALLOCZ(klua_khttp_rtp_inter_t, 1, 0);
    klua_khttp_rtp_t* p_kmnp = new_klua_khttp_rtp(L);
    p_kmnp->L = L;
    p_kmnp->p_env = klua_env_get_by_L(L);
    p_kmnp->p_multi = klua_ex_multiplex_get_by_L(L);
    p_kmnp->p_inter = p_inter;

    memcpy(&p_inter->param, p_param, sizeof(klua_khttp_rtp_param_t));
    p_inter->p_ktcp = p_kmnp;
    p_inter->close = false;
    p_inter->p_socket = p_socket;

    p_inter->p_w_list = klb_list_create();
    p_inter->p_r_buf = klb_buf_malloc(p_inter->param.rbuf_max, false);

    p_inter->p_r_list = klb_list_create();

    p_inter->p_txt = klb_buffer_create(4096);
    p_inter->p_media = klb_buffer_create(4096);

    // http parser
    http_parser_init(&p_inter->parser, p_param->http_type);
    http_parser_settings_init(&p_inter->settings);

    p_inter->parser.data = p_inter; // 回调函数绑定的用户字段

    p_inter->settings.on_message_begin = on_message_begin_klua_khttp_rtp;
    p_inter->settings.on_url = on_url_klua_khttp_rtp;
    p_inter->settings.on_status = on_status_klua_khttp_rtp;
    p_inter->settings.on_header_field = on_header_field_klua_khttp_rtp;
    p_inter->settings.on_header_value = on_header_value_klua_khttp_rtp;
    p_inter->settings.on_headers_complete = on_headers_complete_klua_khttp_rtp;
    p_inter->settings.on_body = on_body_klua_khttp_rtp;
    p_inter->settings.on_message_complete = on_message_complete_klua_khttp_rtp;
    p_inter->settings.on_chunk_header = on_chunk_header_klua_khttp_rtp;
    p_inter->settings.on_chunk_complete = on_chunk_complete_klua_khttp_rtp;

    // 放入 multiplex
    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klua_khttp_rtp;
    o.cb_recv = cb_recv_klua_khttp_rtp;
    o.cb_send = cb_send_klua_khttp_rtp;
    o.cb_timer = cb_timer_klua_khttp_rtp;
    o.p_lparam = p_kmnp;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_kmnp->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return p_kmnp;
}

static int lib_klua_khttp_rtp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    // param
    klua_khttp_rtp_param_t param = { 0 };
    default_param_klua_khttp_rtp(&param);
    check_param_klua_khttp_rtp(L, 2, &param);

    // socket fd
    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    // new connect
    new_connect_klua_khttp_rtp(L, fd, &param);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

static int on_accept_klua_khttp_rtp_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_khttp_rtp_listen_t* p_listen = (klua_khttp_rtp_listen_t*)ptr;

    bool close = true;

    if (p_listen->co_accept)
    {
        // 先判定是否是协程使用环境
        lua_State* L = klua_ex_coroutine_rawgeti(p_listen->p_ex_coroutine, p_listen->co_accept);
        if (NULL != L)
        {
            p_listen->co_accept = NULL; // 协程模式下, 一次"accept", 对应一次唤醒, 唤醒后清空

                                        // param
            klua_khttp_rtp_param_t param = { 0 };
            default_param_klua_khttp_rtp(&param);
            param.http_type = HTTP_REQUEST;

            new_connect_klua_khttp_rtp(L, fd, &param);               /* 1st argument */
            int status = lua_pcall(L, 1, 0, 0);                 /* do the call */
            klua_env_report_by_L(L, status);
        }
    }

    return 0;
}

static int lib_klua_khttp_rtp_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_khttp_rtp_listen_t* p_listen = new_klua_khttp_rtp_listen(L);
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_multi = klua_ex_multiplex_get_by_L(L);
    p_listen->p_ex_coroutine = klua_ex_get_coroutine(p_listen->p_env);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_khttp_rtp_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, p_listen->param.max_connect);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// http-rtp
int klua_open_khttp_rtp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    lib_klua_khttp_rtp_connect },

        { "listen",     lib_klua_khttp_rtp_listen },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // http-rtp listen
    klua_khttp_rtp_listen_createmeta(L);

    // http-rtp
    klua_khttp_rtp_createmeta(L);

    return 1;
}
