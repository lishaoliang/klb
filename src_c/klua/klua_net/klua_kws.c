// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_listen.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klbnet/klb_websocket.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_buffer.h"
#include "klbthird/http_parser.h"
#include "klbutil/klb_list.h"
#include "klbutil/klb_log.h"
#include "klbthird/sds.h"
#include <assert.h>


#define KLUA_KWEBSOCKET_R_BUF_MAX           (16 * 1024)     ///< 默认临时读取缓存
#define KLUA_KWEBSOCKET_TIMEOUT             (20 * 1000)     ///< 默认请求超时(单位毫秒)


typedef struct klua_kwebsocket_t_ klua_kwebsocket_t;


typedef enum klua_kwebsocket_status_e_
{
    KLUA_KWS_HTTP,                          ///< http阶段
    KLUA_KWS_HTTP_OVER,                     ///< http结束
    KLUA_KWS_WS_HEAD,                       ///< websocket阶段
    KLUA_KWS_WS_BODY                        ///< websocket阶段
}klua_kwebsocket_status_e;

// 参数
typedef struct klua_kwebsocket_param_t_
{
    enum http_parser_type   http_type;      ///< ws-http/wss-https类型       
    bool                    tls;            ///< openssl: 默认false
    int64_t                 timeout;        ///< 请求超时: 默认KLUA_KWEBSOCKET_TIMEOUT
}klua_kwebsocket_param_t;


// ws的C内部实现
typedef struct klua_kwebsocket_inter_t_
{
    klua_kwebsocket_param_t param;          ///< 可初始化/配置参数

    klua_kwebsocket_t*      p_kws;          ///< Lua的 userdata
    bool                    is_close;       ///< 是否关闭: true.关闭; false.未关闭
    int64_t                 connect_tc;     ///< 发起连接的时间

    // socket相关
    struct
    {
        klb_socket_t*       p_socket;       ///< socket
        int                 id;             ///< 被分配的ID, 大于0的正整数
    };

    // send发送相关
    struct
    {
        klb_list_t*         p_w_list;       ///< klb_buf_t*
        klb_buf_t*          p_w_cur;        ///< 当前正在发送的缓存
    };

    // recv接收相关
    struct
    {
        klb_buf_t*          p_r_buf;        ///< 临时读取缓存
    };

    // http_parser
    struct
    {
        http_parser             parser;         ///< HTTP解析器
        http_parser_settings    settings;       ///< 解析器设置
        uint64_t                content_length; ///< 内容长度
    };

    // ws
    struct
    {
        klb_websocket_t         ws;
        uint64_t                ws_remaining;

        klb_buffer_t*           p_ws_text;
        klb_buffer_t*           p_ws_binary;
    };

    klua_kwebsocket_status_e    status;         ///< 当前状态
}klua_kwebsocket_inter_t;


//////////////////////////////////////////////////////////////////////////
// 客户端


#define KLUA_KWEBSOCKET_HANDLE     "KLUA_KWEBSOCKET_HANDLE*"

// ws Lua userdata
typedef struct klua_kwebsocket_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_recv;        ///< "recv"函数对应的协程

        klb_multiplex_t*        p_multi;        ///< 复用
    };

    klua_kwebsocket_inter_t*    p_inter;        ///< C内部实现
}klua_kwebsocket_t;

/////////////////////////////////////


/// @brief 从C调用Lua脚本注册的的函数
/// @param [in]  *p_khttp       khttp指针
/// @param [in]  *p_msg         消息类型: "text", "binary"
/// @param [in]  *p_s1          字符串1
/// @param [in]  s1_len         字符串1长度
/// @return int  0.成功; 非0.失败
static int call_lua_reg_on_recv_klua_kwebsocket(klua_kwebsocket_t* p_kws, const char* p_msg, const char* p_s1, int s1_len)
{
    assert(NULL != p_kws);

    if (p_kws->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kws->p_env), p_kws->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_kws->co_recv = NULL; // 清空

        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushlstring(L, p_s1, s1_len);                           /* 2st argument */
        int status = lua_pcall(L, 2, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return -1;
}

/// @param [in]  *p_msg         消息类型: "disconnect"
static int call_lua_reg_on_recv_klua_kwebsocket_error(klua_kwebsocket_t* p_kws, const char* p_msg, const char* p_s1)
{
    assert(NULL != p_kws);

    if (p_kws->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kws->p_env), p_kws->co_recv);
        if (NULL == L) return EXIT_FAILURE;

        p_kws->co_recv = NULL; // 清空

        lua_pushstring(L, p_msg);                                   /* 1st argument */
        lua_pushstring(L, p_s1);                                    /* 2st argument */
        int status = lua_pcall(L, 2, 0, 0);                         /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    return -1;
}

static void klua_kwebsocket_error(klua_kwebsocket_t* p_kws, klb_socket_status_e err)
{
    klb_socket_closing(p_kws->p_inter->p_socket);

    call_lua_reg_on_recv_klua_kwebsocket_error(p_kws, "disconnect", "");
}

/////////////////////////////////////
// C 实现


static int on_message_begin_klua_kwebsocket(http_parser* p_parser)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;
    //KLB_LOG("on_message_begin\n");

    return 0;
}

static int on_url_klua_kwebsocket(http_parser* p_parser, const char* at, size_t length)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_status_klua_kwebsocket(http_parser* p_parser, const char* at, size_t length)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_header_field_klua_kwebsocket(http_parser* p_parser, const char* at, size_t length)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_header_value_klua_kwebsocket(http_parser* p_parser, const char* at, size_t length)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_headers_complete_klua_kwebsocket(http_parser* p_parser)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_body_klua_kwebsocket(http_parser* p_parser, const char* at, size_t length)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_message_complete_klua_kwebsocket(http_parser* p_parser)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    // 非 chunk 编码时,结束
    p_inter->status = KLUA_KWS_HTTP_OVER;

    return 0;
}

static int on_chunk_header_klua_kwebsocket(http_parser* p_parser)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    return 0;
}

static int on_chunk_complete_klua_kwebsocket(http_parser* p_parser)
{
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_parser->data;
    klua_kwebsocket_t* p_kws = p_inter->p_kws;

    if (0 == p_parser->content_length)
    {
        // chunk 编码时, 最后的块长度为0. 表示结束了
        p_inter->status = KLUA_KWS_HTTP_OVER;
    }

    return 0;
}

/////////////////////////////////////////

static void free_klua_kwebsocket_inter(klua_kwebsocket_inter_t* p_inter)
{
    while (0 < klb_list_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = klb_list_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);
    KLB_FREE_BY(p_inter->p_w_list, klb_list_destroy);
    KLB_FREE(p_inter->p_w_cur);
    KLB_FREE(p_inter->p_r_buf);
    KLB_FREE_BY(p_inter->p_ws_text, klb_buffer_destroy);
    KLB_FREE_BY(p_inter->p_ws_binary, klb_buffer_destroy);
    KLB_FREE(p_inter);
}

// 当关闭连接,被klb_multiplex_t移除后,释放
static int on_remove_klua_kwebsocket(void* p_lparam, void* p_wparam, int id)
{
    //klua_kwebsocket_t* p_khttp = (klua_kwebsocket_t*)p_lparam;
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    KLB_FREE_BY(p_inter, free_klua_kwebsocket_inter);

    return 0;
}

static int parse_http_klua_kwebsocket(klua_kwebsocket_t* p_kws)
{
    klua_kwebsocket_inter_t* p_inter = p_kws->p_inter;
    klb_buf_t* p_buf = p_inter->p_r_buf;

    int ret = 0;

    // http 阶段
    int parser = http_parser_execute(&p_inter->parser, &p_inter->settings, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

    if (0 == p_inter->parser.http_errno)
    {
        if (0 < parser)
        {
            p_buf->start += parser;

            if (KLUA_KWS_HTTP_OVER == p_inter->status)
            {
                // 转入 ws 阶段
                p_inter->status = KLUA_KWS_WS_HEAD;
            }
        }
    }
    else
    {
        // http解析错误
        ret = 1;
    }

    if (p_buf->end <= p_buf->start)
    {
        p_buf->start = 0;
        p_buf->end = 0;
    }
    else if (0 != p_buf->start)
    {
        memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        p_buf->end = p_buf->end - p_buf->start;
        p_buf->start = 0;
    }

    return ret;
}

static int recv_http_klua_kwebsocket(klua_kwebsocket_t* p_kws, bool* p_is_first, bool* p_is_end, klb_socket_status_e* p_code)
{
    klua_kwebsocket_inter_t* p_inter = p_kws->p_inter;
    klb_socket_t* p_socket = p_inter->p_socket;

    int read = 0;
    if (0 != parse_http_klua_kwebsocket(p_kws))
    {
        *p_code = KLB_SOCKET_ERR_PROTOCOL; // http协议解析失败
        *p_is_end = true;
        return read;
    }

    if (KLUA_KWS_WS_HEAD <= p_inter->status)
    {
        return read; // 解析http后, 即可能状态变更
    }

    klb_buf_t* p_buf = p_inter->p_r_buf;
    while (true)
    {
        int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            read += r;
            *p_is_first = false;

            if (0 != parse_http_klua_kwebsocket(p_kws))
            {
                *p_code = KLB_SOCKET_ERR_PROTOCOL; // http协议解析失败
                *p_is_end = true;
                return read;
            }

            if (KLUA_KWS_WS_HEAD <= p_inter->status)
            {
                return read; // 解析http后, 即可能状态变更
            }
        }
        else
        {
            if (*p_is_first)
            {
                *p_code = KLB_SOCKET_DISCONNECT; // 网络读写错误
                *p_is_end = true;
                return read;
            }

            *p_is_first = false;
            *p_is_end = true;
            break;
        }
    }

    return read;
}

static int parse_ws_klua_kwebsocket(klua_kwebsocket_t* p_kws)
{
    klua_kwebsocket_inter_t* p_inter = p_kws->p_inter;
    klb_buf_t* p_buf = p_inter->p_r_buf;

    while (true)
    {
        char* p_data = p_buf->p_buf + p_buf->start;
        int data_len = p_buf->end - p_buf->start;

        if (KLUA_KWS_WS_HEAD == p_inter->status)
        {
            klb_websocket_t ws = { 0 };
            if (0 == klb_websocket_parse(&ws, p_data, data_len))
            {
                p_buf->start += ws.head_len;

                memcpy(&p_inter->ws, &ws, sizeof(klb_websocket_t));
                p_inter->ws_remaining = ws.payload_len;

                p_inter->status = KLUA_KWS_WS_BODY;
            }
            else
            {
                break;
            }
        }
        else
        {
            assert(KLUA_KWS_WS_BODY == p_inter->status);

            if (0 < data_len)
            {
                int cp_len = (int)MIN(p_inter->ws_remaining, data_len);
                assert(0 < cp_len);

                uint8_t opcode = p_inter->ws.opcode;

                switch (opcode)
                {
                case KLB_WEBSOCKET_OPCODE_TEXT:
                    {
                        if (0 != p_inter->ws.mask)
                        {
                            klb_websocket_mask(p_inter->ws.mask_key, p_data, cp_len);
                        }

                        klb_buffer_write(p_inter->p_ws_text, p_data, cp_len);
                    } 
                    break;
                case KLB_WEBSOCKET_OPCODE_BINARY:
                    {
                        if (0 != p_inter->ws.mask)
                        {
                            klb_websocket_mask(p_inter->ws.mask_key, p_data, cp_len);
                        }

                        klb_buffer_write(p_inter->p_ws_binary, p_data, cp_len);
                    }
                    break;
                default:
                    break;
                }

                p_buf->start += cp_len;
                p_inter->ws_remaining -= cp_len;

                if (0 == p_inter->ws_remaining)
                {
                    switch (opcode)
                    {
                    case KLB_WEBSOCKET_OPCODE_TEXT:
                        {
                            klb_buf_t* p_text = klb_buffer_join(p_inter->p_ws_text, NULL, NULL);

                            call_lua_reg_on_recv_klua_kwebsocket(p_kws, "text", p_text->p_buf + p_text->start, p_text->end - p_text->start);

                            KLB_FREE(p_text);

                            klb_buffer_reset(p_inter->p_ws_text);
                        }
                        break;
                    case KLB_WEBSOCKET_OPCODE_BINARY:
                        {
                            klb_buf_t* p_binary = klb_buffer_join(p_inter->p_ws_binary, NULL, NULL);
                            
                            call_lua_reg_on_recv_klua_kwebsocket(p_kws, "binary", p_binary->p_buf + p_binary->start, p_binary->end - p_binary->start);

                            KLB_FREE(p_binary);

                            klb_buffer_reset(p_inter->p_ws_binary);
                        }
                        break;
                    }

                    p_inter->status = KLUA_KWS_WS_HEAD;
                }
            }
            else
            {
                break;
            }
        }
    }

    if (p_buf->end <= p_buf->start)
    {
        p_buf->start = 0;
        p_buf->end = 0;
    }
    else if (0 != p_buf->start)
    {
        memmove(p_buf->p_buf, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        p_buf->end = p_buf->end - p_buf->start;
        p_buf->start = 0;
    }

    return 0;
}

static int recv_ws_klua_kwebsocket(klua_kwebsocket_t* p_kws, bool* p_is_first, bool* p_is_end, klb_socket_status_e* p_code)
{
    klua_kwebsocket_inter_t* p_inter = p_kws->p_inter;
    klb_socket_t* p_socket = p_inter->p_socket;

    int read = 0; 
    if (0 != parse_ws_klua_kwebsocket(p_kws))
    {
        *p_code = KLB_SOCKET_ERR_PROTOCOL; // 网络协议错误
        *p_is_end = true;
        return read;
    }

    klb_buf_t* p_buf = p_inter->p_r_buf;

    while (true)
    {
        // 读数据
        int r = klb_socket_recv(p_socket, p_buf->p_buf + p_buf->end, p_buf->buf_len - p_buf->end);

        if (0 < r)
        {
            p_buf->end += r;
            read += r;
            *p_is_first = false;

            if (0 != parse_ws_klua_kwebsocket(p_kws))
            {
                *p_code = KLB_SOCKET_ERR_PROTOCOL;  // 网络协议错误
                *p_is_end = true;
                return read;
            }     
        }
        else
        {
            if (*p_is_first)
            {
                *p_code = KLB_SOCKET_DISCONNECT; // 网络读写错误
                *p_is_end = true;
                return read;
            }

            *p_is_first = false;
            *p_is_end = true;
            break;
        }
    }

    return read;
}

// 当网络上有数据可以接收时, 将系统socket中的数据全部读取
static int on_recv_klua_kwebsocket(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kwebsocket_t* p_kws = (klua_kwebsocket_t*)p_lparam;
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_wparam;

    if (p_inter->is_close || KLB_SOCKET_OK != p_inter->p_socket->status)
    {
        return 0;
    }

    int r = 0;
    bool is_first = true, is_end = false;
    klb_socket_status_e code = KLB_SOCKET_OK;

    while (!is_end)
    {
        if (p_inter->status < KLUA_KWS_WS_HEAD)
        {
            r = recv_http_klua_kwebsocket(p_kws, &is_first, &is_end, &code);
        }
        else
        {
            r = recv_ws_klua_kwebsocket(p_kws, &is_first, &is_end, &code);
        }
    }

    if (KLB_SOCKET_OK != code)
    {
        klua_kwebsocket_error(p_kws, code);
    }

    return r;
}

// 当网络可以发送数据时, 填充系统socket直到满, 或暂无数据
static int on_send_klua_kwebsocket(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kwebsocket_t* p_kws = (klua_kwebsocket_t*)p_lparam;
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_wparam;

    klb_socket_t* p_socket = p_inter->p_socket;

    if (p_inter->is_close || KLB_SOCKET_OK != p_socket->status)
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

        int s = klb_socket_send(p_socket, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);
        if (0 < s)
        {
            send += s;
            p_buf->start += s;

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
                err = KLB_SOCKET_DISCONNECT;
                break;
            }

            break;
        }

        first = false;
    }

    if (NULL == p_inter->p_w_cur && klb_list_size(p_inter->p_w_list) <= 0)
    {
        klb_socket_set_writing(p_inter->p_socket, false);   // 无数据可写
    }

    if (KLB_SOCKET_OK != err)
    {
        klua_kwebsocket_error(p_kws, err);
    }

    return send;
}

// 当定时轮询时: 处理超时等
static int on_timer_klua_kwebsocket(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kwebsocket_t* p_kws = (klua_kwebsocket_t*)p_lparam;
    klua_kwebsocket_inter_t* p_inter = (klua_kwebsocket_inter_t*)p_wparam;

    if (p_inter->is_close)
    {
        return 0;
    }

    if ( p_inter->status < KLUA_KWS_WS_HEAD && p_inter->param.timeout <= ABS_SUB(now, p_inter->connect_tc))
    {
        // http阶段超时了
        klua_kwebsocket_error(p_kws, KLB_SOCKET_TIMEOUT);
    }

    return 0;
}

/////////////////////////////////////
// Lua 接口

static klua_kwebsocket_t* new_klua_kwebsocket(lua_State* L)
{
    klua_kwebsocket_t* p_kws = (klua_kwebsocket_t*)lua_newuserdata(L, sizeof(klua_kwebsocket_t));
    KLB_MEMSET(p_kws, 0, sizeof(klua_kwebsocket_t));
    luaL_setmetatable(L, KLUA_KWEBSOCKET_HANDLE);

    return p_kws;
}

static klua_kwebsocket_t* to_klua_kwebsocket(lua_State* L, int index)
{
    klua_kwebsocket_t* p_kws = (klua_kwebsocket_t*)luaL_checkudata(L, index, KLUA_KWEBSOCKET_HANDLE);
    luaL_argcheck(L, NULL != p_kws, index, "'kwebsocket' expected");
    return p_kws;
}

static int klua_kwebsocket_tostring(lua_State* L)
{
    klua_kwebsocket_t* p_kws = to_klua_kwebsocket(L, 1);

    lua_pushfstring(L, "kwebsocket:%p", p_kws);
    return 1;
}

static int klua_kwebsocket_close(lua_State* L)
{
    klua_kwebsocket_t* p_kws = to_klua_kwebsocket(L, 1);

    if (NULL != p_kws->p_inter)
    {
        p_kws->p_inter->is_close = true;
        klb_socket_closing(p_kws->p_inter->p_socket);
        klb_multiplex_remove(p_kws->p_multi, p_kws->p_inter->id);

        p_kws->p_inter = NULL;
    }

    return 0;
}

static int klua_kwebsocket_send_text(lua_State* L)
{
    klua_kwebsocket_t* p_kws = to_klua_kwebsocket(L, 1);

    size_t text_len = 0;
    const char* p_text = luaL_checklstring(L, 2, &text_len);

    klb_buf_t* p_buf = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_TEXT, NULL, p_text, text_len);

    klb_list_push_tail(p_kws->p_inter->p_w_list, p_buf);    // 放入待发送列表
    klb_socket_set_writing(p_kws->p_inter->p_socket, true); // 有数据可写

    return 0;
}

static int klua_kwebsocket_send_binary(lua_State* L)
{
    klua_kwebsocket_t* p_kws = to_klua_kwebsocket(L, 1);

    size_t binary_len = 0;
    const char* p_binary = luaL_checklstring(L, 2, &binary_len);

    klb_buf_t* p_buf = klb_websocket_pack_fin(KLB_WEBSOCKET_OPCODE_BINARY, NULL, p_binary, binary_len);

    klb_list_push_tail(p_kws->p_inter->p_w_list, p_buf);    // 放入待发送列表
    klb_socket_set_writing(p_kws->p_inter->p_socket, true); // 有数据可写

    return 0;
}

static int klua_kwebsocket_co_recv(lua_State* L)
{
    klua_kwebsocket_t* p_kws = to_klua_kwebsocket(L, 1);
    klua_check_coroutine(L, "kws.connect():co_recv must in coroutine!");

    // todo. 未能及时 recv 数据时的处理

    assert(NULL == p_kws->co_recv);
    p_kws->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_kwebsocket_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_kwebsocket_close },

        { "send_text",      klua_kwebsocket_send_text },
        { "send_binary",    klua_kwebsocket_send_binary },

        { "co_recv",        klua_kwebsocket_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kwebsocket_close },
        { "__close",         klua_kwebsocket_close },
        { "__tostring",      klua_kwebsocket_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KWEBSOCKET_HANDLE);/* metatable for KLUA_KWEBSOCKET_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

////////////////////////////////////

static void default_klua_kwebsocket_param(klua_kwebsocket_param_t* p_param)
{
    p_param->http_type = HTTP_RESPONSE;
    p_param->tls = false;
    p_param->timeout = KLUA_KWEBSOCKET_TIMEOUT;
}

static void send_http_request_klua_kwebsocket(klua_kwebsocket_t* p_kws, const char* p_host, int port, const char* p_path)
{
    sds str = sdsnew("");

    str = sdscatfmt(str, "GET %s HTTP/1.1\r\n", p_path);
    str = sdscatfmt(str, "Host: %s:%i\r\n", p_host, port);
    str = sdscat(str, "Connection: Upgrade\r\n");
    str = sdscat(str, "Upgrade: websocket\r\n");
    str = sdscat(str, "Sec-WebSocket-Version: 13\r\n");
    str = sdscat(str, "Sec-WebSocket-Extensions: permessage-deflate\r\n");
    str = sdscat(str, "Sec-WebSocket-Key: fcoOCnZknPpAcsTt5Ptf0w==\r\n");
    str = sdscat(str, "Content-Length: 0\r\n");
    str = sdscat(str, "\r\n");

    int str_len = sdslen(str);

    klb_buf_t* p_http_text = klb_buf_malloc(str_len, false);
    memcpy(p_http_text->p_buf, str, str_len);
    p_http_text->end = str_len;

    KLB_FREE_BY(str, sdsfree);

    klb_list_push_tail(p_kws->p_inter->p_w_list, p_http_text);  // 放入待发送列表
    klb_socket_set_writing(p_kws->p_inter->p_socket, true);     // 有数据可写
}


static klua_kwebsocket_t* new_connect_klua_kwebsocket(lua_State* L, klb_socket_fd fd, klua_kwebsocket_param_t* p_param)
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

    klua_kwebsocket_inter_t* p_inter = KLB_MALLOCZ(klua_kwebsocket_inter_t, 1, 0);
    klua_kwebsocket_t* p_kws = new_klua_kwebsocket(L);
    p_kws->L = L;
    p_kws->p_env = klua_env_get_by_L(L);
    p_kws->p_multi = klua_ex_multiplex_get_by_L(L);
    p_kws->p_inter = p_inter;

    memcpy(&p_inter->param, p_param, sizeof(klua_kwebsocket_param_t));
    p_inter->p_kws = p_kws;
    p_inter->is_close = false;
    p_inter->connect_tc = klua_env_get_tick_count(p_kws->p_env);
    p_inter->p_socket = p_socket;
    p_inter->p_w_list = klb_list_create();

    p_inter->p_r_buf = klb_buf_malloc(KLUA_KWEBSOCKET_R_BUF_MAX, false);

    // http parser
    http_parser_init(&p_inter->parser, p_param->http_type);
    http_parser_settings_init(&p_inter->settings);

    p_inter->parser.data = p_inter; // 回调函数绑定的用户字段

    p_inter->settings.on_message_begin = on_message_begin_klua_kwebsocket;
    p_inter->settings.on_url = on_url_klua_kwebsocket;
    p_inter->settings.on_status = on_status_klua_kwebsocket;
    p_inter->settings.on_header_field = on_header_field_klua_kwebsocket;
    p_inter->settings.on_header_value = on_header_value_klua_kwebsocket;
    p_inter->settings.on_headers_complete = on_headers_complete_klua_kwebsocket;
    p_inter->settings.on_body = on_body_klua_kwebsocket;
    p_inter->settings.on_message_complete = on_message_complete_klua_kwebsocket;
    p_inter->settings.on_chunk_header = on_chunk_header_klua_kwebsocket;
    p_inter->settings.on_chunk_complete = on_chunk_complete_klua_kwebsocket;

    p_inter->p_ws_text = klb_buffer_create(0);
    p_inter->p_ws_binary = klb_buffer_create(0);

    p_inter->status = KLUA_KWS_HTTP;

    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = on_remove_klua_kwebsocket;
    o.cb_recv = on_recv_klua_kwebsocket;
    o.cb_send = on_send_klua_kwebsocket;
    o.cb_timer = on_timer_klua_kwebsocket;
    o.p_lparam = p_kws;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_kws->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return p_kws;
}

static int lib_klua_kwebsocket_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    const char* p_path = luaL_checkstring(L, 3);

    // step1. 参数
    klua_kwebsocket_param_t param = { 0 };
    default_klua_kwebsocket_param(&param);

    // step2. 创建 socket
    klb_socket_fd fd = klb_socket_connect(p_host, port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);

        assert(false);
        return 1;
    }

    // step3. 创建对象, 并压lua栈, 输出userdata
    klua_kwebsocket_t* p_kws = new_connect_klua_kwebsocket(L, fd, &param);

    // step4. 发送初始 http 请求数据
    send_http_request_klua_kwebsocket(p_kws, p_host, port, p_path);

    return 1;
}


//////////////////////////////////////////////////////////////////////////
// 服务端

#define KLUA_KWEBSOCKET_LISTEN_HANDLE     "KLUA_KWEBSOCKET_LISTEN_HANDLE*"


typedef struct klua_kwebsocket_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_accept;      ///< sync的"accept"函数对应的协程
    };

    struct
    {
        klb_multiplex_t*        p_multi;        ///< 复用
        klua_ex_coroutine_t*    p_ex_coroutine; ///< ex co
    };

    klb_listen_t*               p_listen;       ///< C
}klua_kwebsocket_listen_t;


//////////////////////////////////////
// Lua 接口

static klua_kwebsocket_listen_t* new_klua_kwebsocket_listen(lua_State* L)
{
    klua_kwebsocket_listen_t* p_listen = (klua_kwebsocket_listen_t*)lua_newuserdata(L, sizeof(klua_kwebsocket_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_kwebsocket_listen_t));
    luaL_setmetatable(L, KLUA_KWEBSOCKET_LISTEN_HANDLE);
    return p_listen;
}

static klua_kwebsocket_listen_t* to_klua_kwebsocket_listen(lua_State* L, int index)
{
    klua_kwebsocket_listen_t* p_listen = (klua_kwebsocket_listen_t*)luaL_checkudata(L, index, KLUA_KWEBSOCKET_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'kwebsocket listen' expected");
    return p_listen;
}

static int klua_kwebsocket_listen_tostring(lua_State* L)
{
    klua_kwebsocket_listen_t* p_listen = to_klua_kwebsocket_listen(L, 1);

    lua_pushfstring(L, "kwebsocket.listen:%p,port:%d", p_listen, klb_listen_port(p_listen->p_listen));
    return 1;
}

static int klua_kwebsocket_listen_close(lua_State* L)
{
    klua_kwebsocket_listen_t* p_listen = to_klua_kwebsocket_listen(L, 1);

    assert(NULL == p_listen->co_accept);

    if (NULL != p_listen->p_listen)
    {
        klb_listen_close(p_listen->p_listen);
        KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    }

    return 0;
}

static int klua_kwebsocket_listen_co_accept(lua_State* L)
{ 
    klua_kwebsocket_listen_t* p_listen = to_klua_kwebsocket_listen(L, 1);
    klua_check_coroutine(L, "kws.listen():co_accept must in coroutine!");

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

/////////////////////////////////////

static void klua_kwebsocket_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kwebsocket_listen_close },     ///< async/sync
        { "co_accept",      klua_kwebsocket_listen_co_accept }, ///< sync

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kwebsocket_listen_close },
        { "__close",         klua_kwebsocket_listen_close },
        { "__tostring",      klua_kwebsocket_listen_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KWEBSOCKET_LISTEN_HANDLE);/* metatable for KLUA_KWEBSOCKET_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

///////////////////////////////////////

static int on_accept_klua_kwebsocket_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr, bool tls, const klb_socket_tls_param_t* p_tls_param)
{
    klua_kwebsocket_listen_t* p_listen = (klua_kwebsocket_listen_t*)ptr;

    bool close = true;

    if (p_listen->co_accept)
    {
        // 先判定是否是协程使用环境
        lua_State* L = klua_ex_coroutine_rawgeti(p_listen->p_ex_coroutine, p_listen->co_accept);
        if (NULL != L)
        {
            p_listen->co_accept = NULL; // 协程模式下, 一次"accept", 对应一次唤醒, 唤醒后清空

            // param
            klua_kwebsocket_param_t param = { 0 };
            default_klua_kwebsocket_param(&param);
            param.http_type = HTTP_REQUEST;

            new_connect_klua_kwebsocket(L, fd, &param);     /* 1st argument */
            int status = lua_pcall(L, 1, 0, 0);             /* do the call */
            klua_env_report_by_L(L, status);
        }
    }

    return 0;
}

static int lib_klua_kwebsocket_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_kwebsocket_listen_t* p_listen = new_klua_kwebsocket_listen(L);
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_multi = klua_ex_multiplex_get_by_L(L);
    p_listen->p_ex_coroutine = klua_ex_get_coroutine(p_listen->p_env);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_kwebsocket_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, 20);

    return 1;
}

//////////////////////////////////////////////////////////////////////////
// 导出 table

int klua_open_kws(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    lib_klua_kwebsocket_connect },

        { "listen",     lib_klua_kwebsocket_listen },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // 服务端, 监听
    klua_kwebsocket_listen_createmeta(L);

    // 客户端
    klua_kwebsocket_createmeta(L);

    return 1;
}
