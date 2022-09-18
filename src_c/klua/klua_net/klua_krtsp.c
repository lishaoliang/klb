// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbutil/klb_nlist.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_help.h"
#include "klbmem/klb_buffer.h"
#include "klbutil/klb_log.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbmem/klb_buf_atom.h"
#include "libavutil/avstring.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// rtsp
#define KLUA_KRTSP_HANDLE            "KLUA_KRTSP_HANDLE*"     ///< Lua meta标示


typedef struct klua_krtsp_t_ klua_krtsp_t;


// 参数
typedef struct klua_krtsp_param_t_
{
    int     rbuf_max;       ///< 临时读取缓存最大值
    int     read_max;       ///< 缓存数据最大值后,暂停读取
    bool    tls;            ///< TLS
}klua_krtsp_param_t;


// C
typedef struct klua_krtsp_inter_t_
{
    klua_krtsp_param_t          param;          ///< 参数

    klua_krtsp_t*               p_ktcp;         ///< ktcp
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
        klb_nlist_t*             p_w_list;       ///< 待发送数据(非媒体,文本/二进制等)列表: klb_buf_t*
        klb_buf_t*              p_w_cur;        ///< 当前正在发送的缓存
        int                     w_start;        ///< 当前发送的起始点
    };

    // recv接收相关
    struct
    {
        klb_buf_t*              p_r_buf;        ///< 临时读取缓存
        klb_nlist_t*             p_r_list;       ///< 
    };

    // 解析
    struct
    {
        klb_mnp_t               mnp;            ///< mnp头
        int                     remain_len;     ///< 剩下的数据长度

        klb_buffer_t*           p_txt;          ///< 文本/二进制
        klb_buffer_t*           p_media;        ///< 媒体数据
    };
}klua_krtsp_inter_t;


typedef struct klua_krtsp_t_
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

    klua_krtsp_inter_t*         p_inter;        ///< C内部实现
}klua_krtsp_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_co_recv_klua_krtsp(klua_krtsp_t* p_kmnp, const char* p_type, char* p_head, int head_len, char* p_body, int body_len)
{
    assert(NULL != p_kmnp);

    if (NULL != p_kmnp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kmnp->p_env), p_kmnp->co_recv);
        if (NULL == L) return -1; // 未处理

        p_kmnp->co_recv = NULL; // 清空

        lua_pushstring(L, p_type);              // 类型
        lua_pushlstring(L, p_head, head_len);   // head
        lua_pushlstring(L, p_body, body_len);   // body

        int status = lua_pcall(L, 3, 0, 0);     /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static int call_lua_co_recv_media_klua_krtsp(klua_krtsp_t* p_kmnp, const char* p_type, klb_buf_t* p_buf)
{
    assert(NULL != p_kmnp);

    if (NULL != p_kmnp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kmnp->p_env), p_kmnp->co_recv);
        if (NULL == L) return -1; // 未处理

        p_kmnp->co_recv = NULL; // 清空

        lua_pushstring(L, p_type);              // 类型
        lua_pushlightuserdata(L, p_buf);        // media

        int status = lua_pcall(L, 2, 0, 0); /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static void free_klua_krtsp_inter(klua_krtsp_inter_t* p_inter)
{
    // 清空
    while (0 < klb_nlist_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    while (0 < klb_nlist_size(p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_r_list);
        KLB_FREE(p_tmp);
    }

    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);
    KLB_FREE_BY(p_inter->p_w_list, klb_nlist_destroy);
    KLB_FREE_BY(p_inter->p_r_list, klb_nlist_destroy);
    KLB_FREE_BY(p_inter->p_txt, klb_buffer_destroy);
    KLB_FREE_BY(p_inter->p_media, klb_buffer_destroy);

    KLB_FREE(p_inter->p_w_cur);
    KLB_FREE(p_inter->p_r_buf);
    KLB_FREE(p_inter);
}

static int cb_remove_klua_krtsp(void* p_lparam, void* p_wparam, int id)
{
    //klua_ktcp_t* p_kmnp = (klua_ktcp_t*)p_lparam;
    klua_krtsp_inter_t* p_inter = (klua_krtsp_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    KLB_FREE_BY(p_inter, free_klua_krtsp_inter);

    return 0;
}

static int parse_rnrn_size_klua_krtsp(char* p_data, int data_len, int* p_out_head_len, int* p_out_body_len)
{
    static char s_rnrn[] = "\r\n\r\n";

    if (data_len < sizeof(s_rnrn))
    {
        return 1; // 数据不足
    }

    char* p_rnrn = av_strnstr(p_data, s_rnrn, data_len);
    if (NULL == p_rnrn)
    {
        return 1; // 未找到结束
    }

    int head_len = p_rnrn - p_data + sizeof(s_rnrn) - 1;

    static char s_Content_Length[] = "Content-Length";
    static char s_Content_length[] = "Content-length";
    static char s_content_length[] = "content-length";

    char* p_body_len = av_strnstr(p_data, s_Content_Length, head_len);
    if (NULL == p_body_len) p_body_len = av_strnstr(p_data, s_Content_length, head_len);
    if (NULL == p_body_len) p_body_len = av_strnstr(p_data, s_content_length, head_len);

    int body_len = 0;
    if (NULL != p_body_len)
    {
        //sscanf(p_body_len, "%*[^:]:%d", &body_len);
        av_sscanf(p_body_len, "%*[^:]:%d", &body_len);
    }

    if (data_len < head_len + body_len)
    {
        return 1; // 数据不足
    }

    *p_out_head_len = head_len;
    *p_out_body_len = body_len;

    return 0;
}

static int klua_krtsp_parse(klua_krtsp_t* p_kmnp, klua_krtsp_inter_t* p_inter, klb_socket_t* p_socket)
{
    klb_buf_t* p_buf = p_inter->p_r_buf;

    char* p_data = p_buf->p_buf + p_buf->start;
    int data_len = p_buf->end - p_buf->start;
    if (data_len <= 0)
    {
        return 1; // 无数据了
    }

    int head_len = 0, body_len = 0;
    if (parse_rnrn_size_klua_krtsp(p_data, data_len, &head_len, &body_len) != 0)
    {
        return 1; // 数据不足
    }

    call_lua_co_recv_klua_krtsp(p_kmnp, "text", p_data, head_len, p_data + head_len, body_len);

    p_buf->start += head_len + body_len;

    return 0;
}

static int cb_recv_klua_krtsp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_krtsp_t* p_kmnp = (klua_krtsp_t*)p_lparam;
    klua_krtsp_inter_t* p_inter = (klua_krtsp_inter_t*)p_wparam;

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

        int r = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            recv += r;

            while (true)
            {
                if (0 != klua_krtsp_parse(p_kmnp, p_inter, p_socket))
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

static int cb_send_klua_krtsp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_krtsp_t* p_kmnp = (klua_krtsp_t*)p_lparam;
    klua_krtsp_inter_t* p_inter = (klua_krtsp_inter_t*)p_wparam;

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
        if (NULL == p_inter->p_w_cur && 0 < klb_nlist_size(p_inter->p_w_list))
        {
            p_inter->p_w_cur = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_w_list);
            p_inter->w_start = p_inter->p_w_cur->start;
        }

        klb_buf_t* p_buf = p_inter->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, (const uint8_t*)(p_buf->p_buf + p_inter->w_start), p_buf->end - p_inter->w_start);

        if (0 < w)
        {
            send += w;
            p_inter->w_start += w;

            if (p_buf->end <= p_inter->w_start)
            {
                p_inter->p_w_cur = p_buf->p_next;
                p_inter->w_start = 0;

                klb_buf_unref(p_buf);
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

    if (NULL == p_inter->p_w_cur && klb_nlist_size(p_inter->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_inter->p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        assert(false);
    }

    return send;
}

static int cb_timer_klua_krtsp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_krtsp_t* p_kmnp = (klua_krtsp_t*)p_lparam;
    klua_krtsp_inter_t* p_inter = (klua_krtsp_inter_t*)p_wparam;

    if (p_inter->close)
    {
        return 0;
    }

    return 0;
}

////////////////////////////////////////
static klua_krtsp_t* new_klua_krtsp(lua_State* L)
{
    klua_krtsp_t* p_kmnp = (klua_krtsp_t*)lua_newuserdata(L, sizeof(klua_krtsp_t));
    KLB_MEMSET(p_kmnp, 0, sizeof(klua_krtsp_t));
    luaL_setmetatable(L, KLUA_KRTSP_HANDLE);
    return p_kmnp;
}

static klua_krtsp_t* to_klua_krtsp(lua_State* L, int index)
{
    klua_krtsp_t* p_kmnp = (klua_krtsp_t*)luaL_checkudata(L, index, KLUA_KRTSP_HANDLE);
    luaL_argcheck(L, NULL != p_kmnp, index, "'ktcp' expected");
    return p_kmnp;
}

static int klua_krtsp_tostring(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);

    lua_pushfstring(L, "ktcp:%p", p_kmnp);
    return 1;
}

static int klua_krtsp_close(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);

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

static int klua_krtsp_send_text(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);

    size_t len = 0;
    const char* ptr = luaL_checklstring(L, 2, &len);

    if (len <= 0)
    {
        return 0;
    }

    klb_buf_t* p_buf = klb_buf_malloc(KLB_PADDING_4(len), false);

    memcpy(p_buf->p_buf, ptr, len);
    p_buf->p_buf[len] = '\0';
    p_buf->end = len;

    klb_nlist_push_tail(p_kmnp->p_inter->p_w_list, p_buf);
    klb_socket_set_writing(p_kmnp->p_inter->p_socket, true);

    return 0;
}

static int klua_krtsp_send_binary(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);

    size_t body_len = 0;
    const char* p_body = luaL_checklstring(L, 3, &body_len);

    klb_buf_t* p_buf = klb_mnp_pack_binary(0, 0, (const uint8_t*)p_head, head_len, (const uint8_t*)p_body, body_len);

    if (NULL != p_buf)
    {
        klb_nlist_push_tail(p_kmnp->p_inter->p_w_list, p_buf);
        klb_socket_set_writing(p_kmnp->p_inter->p_socket, true);
    }

    return 0;
}

static int klua_krtsp_send_media(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);
    klb_buf_t* p_frame = (klb_buf_t*)luaL_checklightuserdata(L, 2);

    klb_buf_ref_next(p_frame);
    klb_nlist_push_tail(p_kmnp->p_inter->p_w_list, p_frame);
    klb_socket_set_writing(p_kmnp->p_inter->p_socket, true);

    return 0;
}

static int klua_krtsp_co_recv(lua_State* L)
{
    klua_krtsp_t* p_kmnp = to_klua_krtsp(L, 1);
    klua_check_coroutine(L, "co_recv must in coroutine!");

    if (0 < klb_nlist_size(p_kmnp->p_inter->p_r_list))
    {
        klb_buf_t* p_buf = (klb_buf_t*)klb_nlist_pop_head(p_kmnp->p_inter->p_r_list);

        if (KLB_MNP_TEXT == p_buf->udata ||
            KLB_MNP_BINARY == p_buf->udata)
        {
            klb_mnp_common_t* p_com = (klb_mnp_common_t*)(p_buf->p_buf + p_buf->start);
            assert(p_buf->end - p_buf->start == p_com->size);

            lua_pushstring(L, (KLB_MNP_TEXT == p_buf->udata) ? "text" : "binary");   // 类型

            lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t), p_com->head); // head
            lua_pushlstring(L, p_buf->p_buf + p_buf->start + sizeof(klb_mnp_common_t) + p_com->head, p_com->size - p_com->head - sizeof(klb_mnp_common_t)); // body

            KLB_FREE(p_buf);
        }
        else if (KLB_MNP_MEDIA == p_buf->udata)
        {
            lua_pushstring(L, "media");         // 类型 "media"
            lua_pushnil(L);                     // nil
            lua_pushlightuserdata(L, p_buf);    // ptr

            klb_buf_unref_next(p_buf);
        }
        else
        {
            assert(false);
        }

        return 3;
    }

    assert(NULL == p_kmnp->co_recv);
    p_kmnp->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_krtsp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtsp_close },

        { "send_text",      klua_krtsp_send_text },
        { "send_binary",    klua_krtsp_send_binary },
        { "send_media",     klua_krtsp_send_media },

        { "co_recv",        klua_krtsp_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krtsp_close },
        { "__close",         klua_krtsp_close },
        { "__tostring",      klua_krtsp_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSP_HANDLE);/* metatable for KLUA_KRTSP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

#define KLUA_KRTSP_LISTEN_HANDLE    "KLUA_KRTSP_LISTEN_HANDLE*"

typedef struct klua_krtsp_listen_param_t_
{
    int max_connect;                            ///< 操作系统监听socket, 能缓存的最大连接数
}klua_krtsp_listen_param_t;

typedef struct klua_krtsp_listen_t_
{
    klua_krtsp_listen_param_t    param;          ///< 参数

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
}klua_krtsp_listen_t;

////////////////////////////////////////
static klua_krtsp_listen_t* new_klua_krtsp_listen(lua_State* L)
{
    klua_krtsp_listen_t* p_listen = (klua_krtsp_listen_t*)lua_newuserdata(L, sizeof(klua_krtsp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_krtsp_listen_t));
    luaL_setmetatable(L, KLUA_KRTSP_LISTEN_HANDLE);
    return p_listen;
}

static klua_krtsp_listen_t* to_klua_krtsp_listen(lua_State* L, int index)
{
    klua_krtsp_listen_t* p_listen = (klua_krtsp_listen_t*)luaL_checkudata(L, index, KLUA_KRTSP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ktcp listen' expected");
    return p_listen;
}

static int klua_krtsp_listen_tostring(lua_State* L)
{
    klua_krtsp_listen_t* p_listen = to_klua_krtsp_listen(L, 1);

    lua_pushfstring(L, "ktcp.listen:%p,port:%d", p_listen, klb_listen_port(p_listen->p_listen));
    return 1;
}

static int klua_krtsp_listen_close(lua_State* L)
{
    klua_krtsp_listen_t* p_listen = to_klua_krtsp_listen(L, 1);

    if (NULL != p_listen->p_listen)
    {
        klb_listen_close(p_listen->p_listen);
        KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    }

    return 0;
}

static int klua_krtsp_listen_co_accept(lua_State* L)
{
    klua_krtsp_listen_t* p_listen = to_klua_krtsp_listen(L, 1);
    klua_check_coroutine(L, "kmnp co_accept must in coroutine!");

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////

static void klua_krtsp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krtsp_listen_close },       ///< 

        { "co_accept",      klua_krtsp_listen_co_accept },  ///< 
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krtsp_listen_close },
        { "__close",         klua_krtsp_listen_close },
        { "__tostring",      klua_krtsp_listen_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSP_LISTEN_HANDLE);/* metatable for KLUA_KRTSP_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


#define KLUA_KRTSP_RBUF_MAX          (1024 * 8)              ///< 临时读取缓存最大值
#define KLUA_KRTSP_READ_MAX          (1024 * 512)            ///< 缓存最大值后,暂停读取

static void default_param_klua_krtsp(klua_krtsp_param_t* p_param)
{
    p_param->rbuf_max = KLUA_KRTSP_RBUF_MAX; // 临时读取缓存最大值
    p_param->read_max = KLUA_KRTSP_READ_MAX; // 缓存最大值后,暂停读取
    p_param->tls = false;
}

static void check_param_klua_krtsp(lua_State* L, int from, klua_krtsp_param_t* p_param)
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

klua_krtsp_t* new_connect_klua_krtsp(lua_State* L, klb_socket_fd fd, klua_krtsp_param_t* p_param)
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

    klua_krtsp_inter_t* p_inter = KLB_MALLOCZ(klua_krtsp_inter_t, 1, 0);
    klua_krtsp_t* p_kmnp = new_klua_krtsp(L);
    p_kmnp->L = L;
    p_kmnp->p_env = klua_env_get_by_L(L);
    p_kmnp->p_multi = klua_ex_multiplex_get_by_L(L);
    p_kmnp->p_inter = p_inter;

    memcpy(&p_inter->param, p_param, sizeof(klua_krtsp_param_t));
    p_inter->p_ktcp = p_kmnp;
    p_inter->close = false;
    p_inter->p_socket = p_socket;

    p_inter->p_w_list = klb_nlist_create();
    p_inter->p_r_buf = klb_buf_malloc(p_inter->param.rbuf_max, false);

    p_inter->p_r_list = klb_nlist_create();

    p_inter->p_txt = klb_buffer_create(4096);
    p_inter->p_media = klb_buffer_create(4096);


    // 放入 multiplex
    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klua_krtsp;
    o.cb_recv = cb_recv_klua_krtsp;
    o.cb_send = cb_send_klua_krtsp;
    o.cb_timer = cb_timer_klua_krtsp;
    o.p_lparam = p_kmnp;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_kmnp->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return p_kmnp;
}

static int lib_klua_krtsp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    // param
    klua_krtsp_param_t param = { 0 };
    default_param_klua_krtsp(&param);
    check_param_klua_krtsp(L, 2, &param);

    // socket fd
    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    // new connect
    new_connect_klua_krtsp(L, fd, &param);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

static int on_accept_klua_krtsp_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_krtsp_listen_t* p_listen = (klua_krtsp_listen_t*)ptr;

    bool close = true;

    if (p_listen->co_accept)
    {
        // 先判定是否是协程使用环境
        lua_State* L = klua_ex_coroutine_rawgeti(p_listen->p_ex_coroutine, p_listen->co_accept);
        if (NULL != L)
        {
            p_listen->co_accept = NULL; // 协程模式下, 一次"accept", 对应一次唤醒, 唤醒后清空

                                        // param
            klua_krtsp_param_t param = { 0 };
            default_param_klua_krtsp(&param);

            new_connect_klua_krtsp(L, fd, &param);               /* 1st argument */
            int status = lua_pcall(L, 1, 0, 0);                 /* do the call */
            klua_env_report_by_L(L, status);
        }
    }

    return 0;
}

static int lib_klua_krtsp_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_krtsp_listen_t* p_listen = new_klua_krtsp_listen(L);
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_multi = klua_ex_multiplex_get_by_L(L);
    p_listen->p_ex_coroutine = klua_ex_get_coroutine(p_listen->p_env);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_krtsp_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, p_listen->param.max_connect);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// rtsp
int klua_open_krtsp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    lib_klua_krtsp_connect },

        { "listen",     lib_klua_krtsp_listen },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // krtsp listen
    klua_krtsp_listen_createmeta(L);

    // krtsp
    klua_krtsp_createmeta(L);

    return 1;
}
