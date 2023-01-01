// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbthird/http_parser.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include "klbnet/klb_listen.h"
#include <assert.h>


#define KLUA_KHTTP_HANDLE           "KLUA_KHTTP_HANDLE*"    ///< Lua meta标示
#define KLUA_KHTTP_TIMEOUT          (20 * 1000)             ///< 默认请求超时


typedef enum klua_khttp_parser_status_e_
{
    KLUA_KHTTP_PARSER_ERR       = -1,
    KLUA_KHTTP_PARSER           = 0,
    KLUA_KHTTP_PARSER_CHUNK     = 1,
    KLUA_KHTTP_PARSER_OVER      = 2,
}klua_khttp_parser_status_e;


typedef struct klua_khttp_t_ klua_khttp_t;


// 参数
typedef struct klua_khttp_param_t_
{
    enum http_parser_type   http_type;          ///< http类型       
    bool                    tls;                ///< openssl: 默认false
    int64_t                 timeout;            ///< 请求超时: 默认KLUA_KHTTP_TIMEOUT
}klua_khttp_param_t;


// C
typedef struct klua_khttp_inter_t_
{
    klua_khttp_param_t          param;          ///< 参数

    klua_khttp_t*               p_khttp;        ///< khttp
    bool                        is_close;       ///< 是否关闭: true.关闭; false.未关闭

    int64_t                     connect_tc;     ///< 发起连接的时间

    // socket相关
    struct
    {
        klb_socket_t*           p_socket;       ///< socket
        int                     id;             ///< 被分配的ID, 大于0的正整数
    };

    // send发送相关
    struct
    {
        klb_nlist_t*             p_w_list;       ///< klb_buf_t*
        klb_buf_t*              p_w_cur;        ///< 当前正在发送的缓存
        lua_Integer             w_len;          ///< 当前等待发送的缓存量
    };

    // recv接收相关
    struct
    {
        klb_buf_t*              p_r_buf;        ///< 临时读取缓存
    };

    // http
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
}klua_khttp_inter_t;


// Lua userdata
typedef struct klua_khttp_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        int                     reg_on_recv;    ///< Lua脚本函数
        lua_State*              co_recv;        ///< "recv"函数对应的协程
    };

    struct
    {
        klua_ex_multiplex_t*    p_ex;           ///< 复用扩展
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    klua_khttp_inter_t*         p_inter;        ///< C内部实现
}klua_khttp_t;


//////////////////////////////////////////////////////////////////////////

static void free_klua_khttp(klua_khttp_t* p_khttp)
{
    // unref 注册的调用函数
    klua_unref_registryindex(p_khttp->L, p_khttp->reg_on_recv);
}

static void free_klua_khttp_inter(klua_khttp_inter_t* p_inter)
{
    assert(NULL != p_inter);

    // 销毁 socket
    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);

    // w
    while (0 < klb_nlist_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    // 销毁
    KLB_FREE_BY(p_inter->header_field, sdsfree);

    KLB_FREE_BY(p_inter->p_w_cur, free);
    KLB_FREE_BY(p_inter->p_w_list, klb_nlist_destroy);
    KLB_FREE_BY(p_inter->p_r_buf, free);
    KLB_FREE_BY(p_inter->p_body, klb_buffer_destroy);

    KLB_FREE(p_inter);
}

static void close_klua_khttp(klua_khttp_t* p_khttp)
{
    klua_khttp_inter_t* p_inter = p_khttp->p_inter;

    // 注意: 本函数 可能多次被调用
    // 典型场景: 1. 先关闭, 后面被GC调用; 2. 未关闭, 直接被GC调用

    if (NULL != p_inter)
    {
        p_inter->is_close = true;
        p_inter->p_khttp = NULL;

        if (KLB_SOCKET_OK == klb_socket_get_status(p_inter->p_socket))
        {
            // 关闭socket 的状态, multiplex 模块不再回调数据
            klb_socket_set_status(p_inter->p_socket, KLB_SOCKET_CLOSEING);
        }

        // 移除socket
        klb_multiplex_remove(p_khttp->p_multi, p_inter->id);

        p_khttp->p_inter = NULL;
    }

    // 释放Lua 部分资源
    free_klua_khttp(p_khttp);
}

/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_khttp       khttp指针
/// @param [in]  *p_msg         消息类型: "url", "header", "header_complete", "body"
/// @param [in]  *p_s1          字符串1
/// @param [in]  s1_len         字符串1长度
/// @param [in]  *p_s2          字符串2
/// @param [in]  s2_len         字符串2长度
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_khttp(klua_khttp_t* p_khttp, const char* p_msg, const char* p_s1, int s1_len, const char* p_s2, int s2_len)
{
    assert(NULL != p_khttp);

    if (p_khttp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_khttp->p_env), p_khttp->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_khttp->co_recv = NULL; // 清空

        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushlstring(L, p_s1, s1_len);                           /* 2st argument */
        lua_pushlstring(L, p_s2, s2_len);                           /* 3st argument */
        int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        if (p_khttp->reg_on_recv <= 0) return EXIT_FAILURE;

        lua_State* L = p_khttp->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_khttp->reg_on_recv);    /* to call reg in protected mode */
        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushlstring(L, p_s1, s1_len);                           /* 2st argument */
        lua_pushlstring(L, p_s2, s2_len);                           /* 3st argument */
        int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
        klua_env_report(p_khttp->p_env, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

static int call_lua_reg_on_recv_klua_khttp_error(klua_khttp_t* p_khttp, const char* p_msg, const char* p_s1)
{
    assert(NULL != p_khttp);

    if (p_khttp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_khttp->p_env), p_khttp->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_khttp->co_recv = NULL; // 清空

        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushstring(L, p_s1);                                    /* 2st argument */
        lua_pushstring(L, "");                                      /* 3st argument */
        int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        if (p_khttp->reg_on_recv <= 0) return EXIT_FAILURE;

        lua_State* L = p_khttp->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_khttp->reg_on_recv);    /* to call reg in protected mode */
        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushstring(L, p_s1);                                    /* 2st argument */
        lua_pushstring(L, "");                                      /* 3st argument */
        int status = lua_pcall(L, 3, 0, 0);                         /* do the call */
        klua_env_report(p_khttp->p_env, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

static int on_message_begin_klua_khttp(http_parser* p_parser)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_message_begin\n");

    return 0;
}

static int on_url_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;

    if (p_inter->is_close)
    {
        return 0;
    }

    sds url = (0 < length) ? sdsnewlen(at, length) : sdsnew("");
    const char* p_method = http_method_str((enum http_method)p_parser->method);

    call_lua_reg_on_recv_klua_khttp(p_khttp, "url", url, sdslen(url), p_method, strlen(p_method));

    KLB_FREE_BY(url, sdsfree);
    return 0;
}

static int on_status_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_status\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    sds code_str = NULL;
    if (0 < length)
    {
        code_str = sdsnewlen(at, length);
    } 
    else
    {
        code_str = sdsnew("");
    }

    KLB_FREE_BY(p_inter->header_field, sdsfree);

    // eg. "HTTP/1.1 200 OK"
    p_inter->header_field = sdsnew("");
    p_inter->header_field = sdscatfmt(p_inter->header_field, "HTTP/%u.%u %u %S", p_parser->http_major, p_parser->http_minor, p_parser->status_code, code_str);

    call_lua_reg_on_recv_klua_khttp(p_khttp, "status",
        p_inter->header_field, sdslen(p_inter->header_field),
        NULL, 0);

    KLB_FREE_BY(code_str, sdsfree);
    return 0;
}

static int on_header_field_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_header_field\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    KLB_FREE_BY(p_inter->header_field, sdsfree);
    p_inter->header_field = sdsnewlen(at, length);

    return 0;
}

static int on_header_value_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_header_value\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    call_lua_reg_on_recv_klua_khttp(p_khttp, "header",
                                p_inter->header_field, sdslen(p_inter->header_field),
                                at, length);

    return 0;
}

static int on_headers_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_headers_complete\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    p_inter->content_length = p_parser->content_length;

    call_lua_reg_on_recv_klua_khttp(p_khttp, "header_complete", NULL, 0, NULL, 0);

    return 0;
}

static int on_body_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_body\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    klb_buffer_write(p_inter->p_body, at, length);

    return 0;
}

static int on_message_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_message_complete\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    p_inter->parser_status = KLUA_KHTTP_PARSER_OVER;

    return 0;
}

static int on_chunk_header_klua_khttp(http_parser* p_parser)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_chunk_header\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    p_inter->parser_status = KLUA_KHTTP_PARSER_CHUNK;

    return 0;
}

static int on_chunk_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_parser->data;
    klua_khttp_t* p_khttp = p_inter->p_khttp;
    //KLB_LOG("on_chunk_complete\n");

    if (p_inter->is_close)
    {
        return 0;
    }

    if (0 == p_parser->content_length)
    {
        //KLB_LOG("on_chunk_header end\n");
        p_inter->parser_status = KLUA_KHTTP_PARSER_OVER;
    }

    return 0;
}

static char s_klua_khttp_error_str[KLB_SOCKET_ERR_MAX][16] = {
    "Ok",
    "Disconnect",
    "Timeout",
    "Protocol"
};

static void klua_khttp_error(klua_khttp_t* p_khttp, klua_khttp_inter_t* p_inter, klb_socket_status_e err)
{
    klb_socket_t* p_socket = p_inter->p_socket;

    // 设置socket错误状态
    klb_socket_set_status(p_socket, err);

    // 调用错误回调
    call_lua_reg_on_recv_klua_khttp_error(p_khttp, "disconnect", s_klua_khttp_error_str[err]);
}

static int cb_klua_khttp_remove(void* p_lparam, void* p_wparam, int id)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    free_klua_khttp_inter(p_inter);

    return 0;
}

static int cb_klua_khttp_recv(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_wparam;

    klb_socket_t* p_socket = p_inter->p_socket;

    if (p_inter->is_close || KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_buf_t* p_buf = p_inter->p_r_buf;

    int r = klb_socket_recv(p_socket, (uint8_t*)(p_buf->p_buf + p_buf->end), p_buf->buf_len - p_buf->end);
    
    if (0 < r)
    {
        p_buf->end += r;

        int parser = http_parser_execute(&p_inter->parser, &p_inter->settings, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

        if (0 == p_inter->parser.http_errno)
        {
            if (0 < parser)
            {
                p_buf->start += parser;

                if (KLUA_KHTTP_PARSER_OVER == p_inter->parser_status)
                {
                    //
                    klb_buf_t* p_body = klb_buffer_join(p_inter->p_body, NULL, NULL);

                    call_lua_reg_on_recv_klua_khttp(p_khttp, "body",
                                p_body->p_buf + p_body->start, p_body->end - p_body->start,
                                NULL, 0);

                    KLB_FREE(p_body);

                    klb_buffer_reset(p_inter->p_body);
                }

                if (p_buf->end <= p_buf->start)
                {
                    p_buf->start = 0;
                    p_buf->end = 0;
                }
                else
                {
                    memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
                    p_buf->end = p_buf->end - p_buf->start;
                    p_buf->start = 0;
                }
            }
        }
        else
        {
            p_inter->parser_status = KLUA_KHTTP_PARSER_ERR;
        }
    }

    return r;
}

static int cb_klua_khttp_send(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_wparam;

    klb_socket_t* p_socket = p_inter->p_socket;

    if (p_inter->is_close || KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;

    // select/epool 之后, 首次调用send 返回-1时, 才认为网络断开 
    bool first = true;

    while (true)
    {
        if (NULL == p_inter->p_w_cur && 0 < klb_nlist_size(p_inter->p_w_list))
        {
            p_inter->p_w_cur = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_w_list);
        }

        klb_buf_t* p_buf = p_inter->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        int w = klb_socket_send(p_socket, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start);
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
        else if (0 == w)
        {
            break; // 无法再发送
        }
        else
        {
            if (first)
            {
                err = KLB_SOCKET_DISCONNECT;
            }
            break; // 出现错误
        }

        first = false;
    }

    if (NULL == p_inter->p_w_cur && klb_nlist_size(p_inter->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_inter->p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klua_khttp_error(p_khttp, p_inter, err);
    }

    return send;
}

static int cb_klua_khttp_timer(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klua_khttp_inter_t* p_inter = (klua_khttp_inter_t*)p_wparam;

    if (p_inter->is_close)
    {
        return 0;
    }

    if (p_inter->param.timeout <= ABS_SUB(now, p_inter->connect_tc))
    {
        // 超时了
        klua_khttp_error(p_khttp, p_inter, KLB_SOCKET_TIMEOUT);
    }

    return 0;
}

////////////////////////////////////////

static klua_khttp_t* new_klua_khttp(lua_State* L)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)lua_newuserdata(L, sizeof(klua_khttp_t));
    KLB_MEMSET(p_khttp, 0, sizeof(klua_khttp_t));
    luaL_setmetatable(L, KLUA_KHTTP_HANDLE);

    return p_khttp;
}

static klua_khttp_t* to_klua_khttp(lua_State* L, int index)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)luaL_checkudata(L, index, KLUA_KHTTP_HANDLE);
    luaL_argcheck(L, NULL != p_khttp, index, "'khttp' expected");
    return p_khttp;
}

static int klua_khttp_gc(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    close_klua_khttp(p_khttp);

    return 0;
}

static int klua_khttp_tostring(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    lua_pushfstring(L, "khttp:%p", p_khttp);
    return 1;
}

static int klua_khttp_on_recv(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    p_khttp->reg_on_recv = klua_ref_registryindex(L, 2);

    return 0;
}

static int klua_khttp_send(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);
    
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

    klb_nlist_push_tail(p_khttp->p_inter->p_w_list, p_buf);
    klb_socket_set_writing(p_khttp->p_inter->p_socket, true);

    return 0;
}

static int klua_khttp_disconnect(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    close_klua_khttp(p_khttp);

    return 0;
}

static int klua_khttp_co_recv(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "khttp.connect():recv must in coroutine!");
        return 0;
    }

    // todo. 未能及时 recv 数据时的处理

    assert(NULL == p_khttp->co_recv);
    p_khttp->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_khttp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_khttp_disconnect },

        { "send",           klua_khttp_send },

        { "on_recv",        klua_khttp_on_recv },
        { "co_recv",        klua_khttp_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            klua_khttp_gc},
        {"__close",         klua_khttp_gc},
        {"__tostring",      klua_khttp_tostring},
        {NULL,              NULL}
    };

    luaL_newmetatable(L, KLUA_KHTTP_HANDLE);/* metatable for KLUA_KHTTP_META handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

static void default_klua_khttp_param(klua_khttp_param_t* p_param)
{
    p_param->http_type = HTTP_RESPONSE;
    p_param->tls = false;
    p_param->timeout = KLUA_KHTTP_TIMEOUT;
}

static void check_klua_khttp_param(lua_State* L, int from, klua_khttp_param_t* p_param)
{
    static const char* options[] = { "tls", "timeout", NULL };

    int num = lua_gettop(L) - from;

    for (int i = 1; i < num; i+=2)
    {
        int idx = luaL_checkoption(L, from + i, NULL, options);
        int ud = from + i + 1;
        switch (idx)
        {
        case 0:
            p_param->tls = luaL_checkboolean(L, ud);
            break;
        case 1:            
            {
                int64_t timeout = luaL_checkinteger(L, ud);
                if (0 < timeout)
                {
                    p_param->timeout = timeout;
                }
            }
            break;
        default:
            break;
        }
    }
}

klua_khttp_t* new_connect_klua_khttp(lua_State* L, klb_socket_fd fd, klua_khttp_param_t* p_param)
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

    klua_khttp_inter_t* p_inter = KLB_MALLOCZ(klua_khttp_inter_t, 1, 0);
    klua_khttp_t* p_khttp = new_klua_khttp(L);
    p_khttp->L = L;
    p_khttp->p_env = klua_env_get_by_L(L);
    p_khttp->p_ex = klua_ex_get_multiplex(p_khttp->p_env);
    p_khttp->p_multi = klua_ex_multiplex_get(p_khttp->p_ex);
    p_khttp->p_inter = p_inter;

    memcpy(&p_inter->param, p_param, sizeof(klua_khttp_param_t));
    p_inter->p_khttp = p_khttp;
    p_inter->is_close = false;
    p_inter->connect_tc = klua_env_get_tick_count(p_khttp->p_env);
    p_inter->p_socket = p_socket;
    p_inter->p_w_list = klb_nlist_create();

    p_inter->p_r_buf = klb_buf_malloc(1024 * 16, false);

    p_inter->p_body = klb_buffer_create(1024 * 16);

    // http parser
    http_parser_init(&p_inter->parser, p_param->http_type);
    http_parser_settings_init(&p_inter->settings);

    p_inter->parser.data = p_inter; // 回调函数绑定的用户字段

    p_inter->settings.on_message_begin = on_message_begin_klua_khttp;
    p_inter->settings.on_url = on_url_klua_khttp;
    p_inter->settings.on_status = on_status_klua_khttp;
    p_inter->settings.on_header_field = on_header_field_klua_khttp;
    p_inter->settings.on_header_value = on_header_value_klua_khttp;
    p_inter->settings.on_headers_complete = on_headers_complete_klua_khttp;
    p_inter->settings.on_body = on_body_klua_khttp;
    p_inter->settings.on_message_complete = on_message_complete_klua_khttp;
    p_inter->settings.on_chunk_header = on_chunk_header_klua_khttp;
    p_inter->settings.on_chunk_complete = on_chunk_complete_klua_khttp;

    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_klua_khttp_remove;
    o.cb_recv = cb_klua_khttp_recv;
    o.cb_send = cb_klua_khttp_send;
    o.cb_timer = cb_klua_khttp_timer;
    o.p_lparam = p_khttp;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_khttp->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return p_khttp;
}

static int klua_khttp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    klua_khttp_param_t param;
    default_klua_khttp_param(&param);
    check_klua_khttp_param(L, 2, &param);

    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);

        assert(false);
        return 1;
    }

    new_connect_klua_khttp(L, fd, &param);
    return 1;
}


//////////////////////////////////////////////////////////////////////////
// http listen

#define KLUA_KHTTP_LISTEN_HANDLE    "KLUA_KHTTP_LISTEN_HANDLE*"


typedef struct klua_khttp_listen_param_t_{
    int max_connect;                            ///< 操作系统监听socket, 能缓存的最大连接数
}klua_khttp_listen_param_t;


typedef struct klua_khttp_listen_t_
{
    klua_khttp_listen_param_t   param;

    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        int                     reg_on_accept;  ///< Lua脚本函数(async)
        lua_State*              co_accept;      ///< sync的"accept"函数对应的协程
    };

    struct
    {
        klua_ex_multiplex_t*    p_ex;           ///< 复用扩展
        klb_multiplex_t*        p_multi;        ///< 复用
        klua_ex_coroutine_t*    p_ex_coroutine; ///< ex co
    };

    klb_listen_t*               p_listen;       ///< C
}klua_khttp_listen_t;


//////////////////////////////////////

static klua_khttp_listen_t* new_klua_khttp_listen(lua_State* L)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)lua_newuserdata(L, sizeof(klua_khttp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_khttp_listen_t));
    luaL_setmetatable(L, KLUA_KHTTP_LISTEN_HANDLE);
    return p_listen;
}

static klua_khttp_listen_t* to_klua_khttp_listen(lua_State* L, int index)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)luaL_checkudata(L, index, KLUA_KHTTP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'khttp listen' expected");
    return p_listen;
}

static void close_klua_khttp_listen(klua_khttp_listen_t* p_listen)
{
    assert(NULL == p_listen->co_accept);

    if (NULL != p_listen->p_listen)
    {
        klb_listen_close(p_listen->p_listen);
        KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    }

    klua_unref_registryindex(p_listen->L, p_listen->reg_on_accept);
}

static int klua_khttp_listen_on_accept(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    klua_unref_registryindex(L, p_listen->reg_on_accept);   // 若已设置过, 
    p_listen->reg_on_accept = klua_ref_registryindex(L, 2);

    return 0;
}

static int klua_khttp_listen_close(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);
    close_klua_khttp_listen(p_listen);
    return 0;
}

static int klua_khttp_listen_accept(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "khttp.listen():accept must in coroutine!");
        return 0;
    }

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

static int klua_khttp_listen_gc(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    close_klua_khttp_listen(p_listen);
    return 0;
}

static int klua_khttp_listen_tostring(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    lua_pushfstring(L, "khttp.listen:%p,port:%d", p_listen, klb_listen_port(p_listen->p_listen));
    return 1;
}

//////////////////////////////////////////////////

static void klua_khttp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "on_accept",      klua_khttp_listen_on_accept },  ///< async
        { "co_accept",      klua_khttp_listen_accept },     ///< sync
        { "close",          klua_khttp_listen_close },      ///< async/sync

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_khttp_listen_gc },
        { "__close",         klua_khttp_listen_gc },
        { "__tostring",      klua_khttp_listen_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KHTTP_LISTEN_HANDLE);/* metatable for KLUA_KHTTP_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

///////////////////////////////////////

static int on_accept_klua_khttp_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)ptr;
    lua_State* L = klua_env_get_L(p_listen->p_env);
    
    klua_khttp_param_t param;
    default_klua_khttp_param(&param);
    param.http_type = HTTP_REQUEST;

    if (p_listen->co_accept)
    {
        // 先判定是否是协程使用环境
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_listen->p_env), p_listen->co_accept);
        if (NULL == L) 
        {
            KLB_SOCKET_CLOSE(fd);
            return EXIT_FAILURE;
        }

        p_listen->co_accept = NULL; // 协程模式下, 一次"accept", 对应一次唤醒, 唤醒后清空

        new_connect_klua_khttp(L, fd, &param);              /* 1st argument */
        int status = lua_pcall(L, 1, 0, 0);                 /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else
    {
        // 再判定是否使用回调接收
        // 若也没有回调, 则直接关闭socket fd
        if (p_listen->reg_on_accept <= 0)
        {
            KLB_SOCKET_CLOSE(fd);
            return EXIT_FAILURE;
        }

        lua_State* L = p_listen->L;
        KLUA_HELP_TOP_B(L);
        lua_rawgeti(L, LUA_REGISTRYINDEX, p_listen->reg_on_accept); /* to call reg in protected mode */

        new_connect_klua_khttp(L, fd, &param);          /* 1st argument */
        int status = lua_pcall(L, 1, 0, 0);             /* do the call */
        klua_env_report_by_L(L, status);

        KLUA_HELP_TOP_E(L);
        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return 0;
}

static void default_klua_khttp_listen_param(klua_khttp_listen_param_t* p_param)
{
    p_param->max_connect = 20;
}

static void check_klua_khttp_listen_param(lua_State* L, int from, klua_khttp_listen_param_t* p_param)
{
    static const char* options[] = { "max_connect", NULL };

    int num = lua_gettop(L) - from;

    for (int i = 1; i < num; i += 2)
    {
        int idx = luaL_checkoption(L, from + i, NULL, options);
        int ud = from + i + 1;
        switch (idx)
        {
        case 0:
            {
                int max_connect = luaL_checkinteger(L, ud);
                if (0 < max_connect)
                {
                    p_param->max_connect = max_connect;
                }
            }
            break;
        default:
            break;
        }
    }
}

static int klua_khttp_listen(lua_State* L)
{
    int port = luaL_checkinteger(L, 1);

    klua_khttp_listen_param_t param = { 0 };
    default_klua_khttp_listen_param(&param);
    check_klua_khttp_listen_param(L, 1, &param);

    klua_khttp_listen_t* p_listen = new_klua_khttp_listen(L);
    memcpy(&p_listen->param, &param, sizeof(klua_khttp_listen_param_t));
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_ex = klua_ex_get_multiplex(p_listen->p_env);
    p_listen->p_multi = klua_ex_multiplex_get(p_listen->p_ex);
    p_listen->p_ex_coroutine = klua_ex_get_coroutine(p_listen->p_env);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_khttp_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, p_listen->param.max_connect);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_khttp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    klua_khttp_connect },

        { "listen",     klua_khttp_listen },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KHTTP_LISTEN_HANDLE
    klua_khttp_listen_createmeta(L);

    // KLUA_KHTTP_HANDLE
    klua_khttp_createmeta(L);

    return 1;
}
