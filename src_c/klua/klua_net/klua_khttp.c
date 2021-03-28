#include "klua/klua.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbthird/http_parser.h"
#include "klbutil/klb_list.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_KHTTP_HANDLE     "KLUA_KHTTP_HANDLE*"


typedef enum klua_khttp_parser_status_e_
{
    KLUA_KHTTP_PARSER_ERR       = -1,
    KLUA_KHTTP_PARSER           = 0,
    KLUA_KHTTP_PARSER_CHUNK     = 1,
    KLUA_KHTTP_PARSER_OVER      = 2,
}klua_khttp_parser_status_e;


typedef struct klua_khttp_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        int                     reg_on_recv;    ///< Lua脚本函数
    };

    struct
    {
        klua_ex_multiplex_t*    p_ex;           ///< 复用扩展
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    // socket相关
    struct
    {
        klb_socket_t*           p_socket;       ///< socket
        int                     id;             ///< 被分配的ID, 大于0的正整数
    };

    // send发送相关
    struct
    {
        klb_list_t*             p_w_list;       ///< klb_buf_t*
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
}klua_khttp_t;


//////////////////////////////////////////////////////////////////////////
static void free_klua_khttp(klua_khttp_t* p_khttp);


/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_khttp       khttp指针
/// @param [in]  *p_msg         消息类型: "http", "header", "header_complete", "body"
/// @param [in]  *p_s1          字符串1
/// @param [in]  s1_len         字符串1长度
/// @param [in]  *p_s2          字符串2
/// @param [in]  s2_len         字符串2长度
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_khttp(klua_khttp_t* p_khttp, const char* p_msg, const char* p_s1, int s1_len, const char* p_s2, int s2_len)
{
    assert(NULL != p_khttp);
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

static int call_lua_reg_on_recv_klua_khttp_error(klua_khttp_t* p_khttp, const char* p_msg, const char* p_s1)
{
    assert(NULL != p_khttp);
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

static int on_message_begin_klua_khttp(http_parser* p_parser)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_message_begin\n");

    return 0;
}

static int on_status_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_status\n");

    sds code_str = NULL;
    if (0 < length)
    {
        code_str = sdsnewlen(at, length);
    } 
    else
    {
        code_str = sdsnew("");
    }

    KLB_FREE_BY(p_khttp->header_field, sdsfree);

    // eg. "HTTP/1.1 200 OK"
    p_khttp->header_field = sdsnew("");
    p_khttp->header_field = sdscatfmt(p_khttp->header_field, "HTTP/%u.%u %u %S", p_parser->http_major, p_parser->http_minor, p_parser->status_code, code_str);

    call_lua_reg_on_recv_klua_khttp(p_khttp, "http",
        p_khttp->header_field, sdslen(p_khttp->header_field),
        NULL, 0);

    KLB_FREE_BY(code_str, sdsfree);
    return 0;
}

static int on_header_field_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_header_field\n");

    KLB_FREE_BY(p_khttp->header_field, sdsfree);
    p_khttp->header_field = sdsnewlen(at, length);

    return 0;
}

static int on_header_value_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_header_value\n");

    call_lua_reg_on_recv_klua_khttp(p_khttp, "header",
                                p_khttp->header_field, sdslen(p_khttp->header_field),
                                at, length);

    return 0;
}

static int on_headers_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_headers_complete\n");

    p_khttp->content_length = p_parser->content_length;

    call_lua_reg_on_recv_klua_khttp(p_khttp, "header_complete", NULL, 0, NULL, 0);

    return 0;
}

static int on_body_klua_khttp(http_parser* p_parser, const char* at, size_t length)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_body\n");

    klb_buffer_write(p_khttp->p_body, at, length);

    return 0;
}

static int on_message_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_message_complete\n");

    p_khttp->parser_status = KLUA_KHTTP_PARSER_OVER;

    return 0;
}

static int on_chunk_header_klua_khttp(http_parser* p_parser)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_chunk_header\n");

    p_khttp->parser_status = KLUA_KHTTP_PARSER_CHUNK;

    return 0;
}

static int on_chunk_complete_klua_khttp(http_parser* p_parser)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_parser->data;
    //KLB_LOG("on_chunk_complete\n");

    if (0 == p_parser->content_length)
    {
        //KLB_LOG("on_chunk_header end\n");
        p_khttp->parser_status = KLUA_KHTTP_PARSER_OVER;
    }

    return 0;
}

static char s_klua_khttp_error_str[KLB_SOCKET_ERR_MAX][16] = {
    "Ok",
    "Error",
    "Disconnect",
    "Timeout",
    "Protocol"
};

static void klua_khttp_error(klua_khttp_t* p_khttp, klb_socket_status_e err)
{
    klb_socket_t* p_socket = p_khttp->p_socket;

    // 设置socket错误状态
    p_socket->status = err;

    // 调用错误回调
    call_lua_reg_on_recv_klua_khttp_error(p_khttp, "disconnect", s_klua_khttp_error_str[err]);
}

static int cb_klua_khttp_remove(void* p_lparam, void* p_wparam, int id)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;

    free_klua_khttp(p_khttp);

    return 0;
}

static int cb_klua_khttp_recv(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klb_socket_t* p_socket = p_khttp->p_socket;

    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_buf_t* p_buf = p_khttp->p_r_buf;

    int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);
    
    if (0 < r)
    {
        p_buf->end += r;

        size_t parser = http_parser_execute(&p_khttp->parser, &p_khttp->settings, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        
        if (0 == p_khttp->parser.http_errno)
        {
            if (0 < parser)
            {
                p_buf->start += parser;

                if (KLUA_KHTTP_PARSER_OVER == p_khttp->parser_status)
                {
                    //
                    klb_buf_t* p_body = klb_buffer_join(p_khttp->p_body);

                    call_lua_reg_on_recv_klua_khttp(p_khttp, "body",
                                                p_body->p_buf + p_body->start, p_body->end - p_body->start,
                                                NULL, 0);
                    KLB_FREE(p_body);

                    klb_buffer_reset(p_khttp->p_body);
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
            p_khttp->parser_status = KLUA_KHTTP_PARSER_ERR;
        }
    }

    return r;
}

static int cb_klua_khttp_send(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;
    klb_socket_t* p_socket = p_khttp->p_socket;

    if (KLB_SOCKET_OK != p_socket->status)
    {
        return 0;
    }

    klb_socket_status_e err = KLB_SOCKET_OK;
    int send = 0;

    while (true)
    {
        if (NULL == p_khttp->p_w_cur && 0 < klb_list_size(p_khttp->p_w_list))
        {
            p_khttp->p_w_cur = (klb_buf_t*)klb_list_pop_head(p_khttp->p_w_list);
        }

        klb_buf_t* p_buf = p_khttp->p_w_cur;
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
                p_khttp->p_w_cur = NULL;
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

    if (NULL == p_khttp->p_w_cur && klb_list_size(p_khttp->p_w_list) <= 0)
    {
        klb_socket_set_sending(p_khttp->p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klua_khttp_error(p_khttp, err);
    }

    return send;
}

//////////////////////////////////////////////////////////////////////////
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

static void free_klua_khttp(klua_khttp_t* p_khttp)
{
    if (0 < p_khttp->id)
    {
        lua_State* L = klua_env_get_L(p_khttp->p_env);

        // 销毁 socket
        KLB_FREE_BY(p_khttp->p_socket, klb_socket_destroy);

        // unref 注册的调用函数
        klua_unref_registryindex(L, p_khttp->reg_on_recv);

        // w
        while (0 < klb_list_size(p_khttp->p_w_list))
        {
            klb_buf_t* p_tmp = klb_list_pop_head(p_khttp->p_w_list);
            KLB_FREE(p_tmp);
        }

        // 销毁
        KLB_FREE_BY(p_khttp->header_field, sdsfree);

        KLB_FREE_BY(p_khttp->p_w_cur, free);
        KLB_FREE_BY(p_khttp->p_w_list, klb_list_destroy);
        KLB_FREE_BY(p_khttp->p_r_buf, free);
        KLB_FREE_BY(p_khttp->p_body, klb_buffer_destroy);

        p_khttp->id = 0;
    }
}

static int klua_khttp_gc(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    return 0;
}

static int klua_khttp_tostring(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    lua_pushfstring(L, "khttp:%p", p_khttp);
    return 1;
}

static int klua_khttp_set_on_recv(lua_State* L)
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

    klb_list_push_tail(p_khttp->p_w_list, p_buf);
    klb_socket_set_sending(p_khttp->p_socket, true);

    return 0;
}

static int klua_khttp_close(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    klb_multiplex_remove(p_khttp->p_multi, p_khttp->id);

    return 0;
}

static void klua_khttp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "set_on_recv",    klua_khttp_set_on_recv },

        { "send",           klua_khttp_send },
        { "close",          klua_khttp_close },

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

static int klua_khttp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    bool tls = false;
    if (lua_isboolean(L, 3))
    {
        tls = lua_toboolean(L, 3);
    }   

    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);

        assert(false);
        return 1;
    }

    klb_socket_t* p_socket = NULL;
    if (tls)
    {
        p_socket = klb_socket_tls_async_create(fd);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    klua_khttp_t* p_khttp = new_klua_khttp(L);
    p_khttp->L = L;
    p_khttp->p_env = klua_env_get_by_L(L);;
    p_khttp->p_socket = p_socket;
    p_khttp->p_ex = klua_ex_get_multiplex(p_khttp->p_env);
    p_khttp->p_multi = klua_ex_multiplex_get(p_khttp->p_ex);

    p_khttp->p_w_list = klb_list_create();

    p_khttp->p_r_buf = klb_buf_malloc(1024 * 16, false);

    p_khttp->p_body = klb_buffer_create(1024 * 16);

    // http parser
    http_parser_init(&p_khttp->parser, HTTP_RESPONSE);
    http_parser_settings_init(&p_khttp->settings);

    p_khttp->parser.data = p_khttp; // 回调函数绑定的用户字段

    p_khttp->settings.on_message_begin = on_message_begin_klua_khttp;
    p_khttp->settings.on_url = NULL;
    p_khttp->settings.on_status = on_status_klua_khttp;
    p_khttp->settings.on_header_field = on_header_field_klua_khttp;
    p_khttp->settings.on_header_value = on_header_value_klua_khttp;
    p_khttp->settings.on_headers_complete = on_headers_complete_klua_khttp;
    p_khttp->settings.on_body = on_body_klua_khttp;
    p_khttp->settings.on_message_complete = on_message_complete_klua_khttp;
    p_khttp->settings.on_chunk_header = on_chunk_header_klua_khttp;
    p_khttp->settings.on_chunk_complete = on_chunk_complete_klua_khttp;

    klb_multiplex_obj_t o = { 0 };
    o.cb_remove = cb_klua_khttp_remove;
    o.cb_recv = cb_klua_khttp_recv;
    o.cb_send = cb_klua_khttp_send;
    o.p_lparam = p_khttp;

    p_khttp->id = klb_multiplex_push_socket(p_khttp->p_multi, p_khttp->p_socket, &o);
    assert(0 <= p_khttp->id);

    
    // 开启读取
    klb_socket_set_reading(p_khttp->p_socket, true);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_khttp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    klua_khttp_connect },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // KLUA_KHTTP_META
    klua_khttp_createmeta(L);

    return 1;
}
