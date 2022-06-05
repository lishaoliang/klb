// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbutil/klb_list.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/extension/klua_ex_multiplex.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// tcp
#define KLUA_KTCP_HANDLE            "KLUA_KTCP_HANDLE*"     ///< Lua meta标示


typedef struct klua_ktcp_t_ klua_ktcp_t;


// 参数
typedef struct klua_ktcp_param_t_
{
    int     rbuf_max;       ///< 临时读取缓存最大值
    int     read_max;       ///< 缓存数据最大值后,暂停读取
    bool    tls;            ///< TLS
}klua_ktcp_param_t;


// C
typedef struct klua_ktcp_inter_t_
{
    klua_ktcp_param_t           param;          ///< 参数

    klua_ktcp_t*                p_ktcp;         ///< ktcp
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
    };

    // recv接收相关
    struct
    {
        klb_buf_t*              p_r_buf;        ///< 临时读取缓存

        klb_list_t*             p_r_list;       ///< 读取的数据列表: klb_buf_t*
        int                     read_num;       ///< p_r_list列表中缓存的数据量: 当达到一定值时, 暂停读取, 直到再次消费数据
    };
}klua_ktcp_inter_t;


typedef struct klua_ktcp_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_recv;        ///< co_recv携程
    };

    struct
    {
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    klua_ktcp_inter_t*          p_inter;        ///< C内部实现
}klua_ktcp_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_co_recv_klua_ktcp(klua_ktcp_t* p_ktcp, klb_buf_t* p_buf)
{
    assert(NULL != p_ktcp);

    if (NULL != p_ktcp->co_recv)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_ktcp->p_env), p_ktcp->co_recv);
        if (NULL == L) return -1; // 未处理

        p_ktcp->co_recv = NULL; // 清空

        lua_pushlstring(L, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

        int status = lua_pcall(L, 1, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static void free_klua_ktcp_inter(klua_ktcp_inter_t* p_inter)
{
    // 清空
    while (0 < klb_list_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_list_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    // 清空
    while (0 < klb_list_size(p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_list_pop_head(p_inter->p_r_list);
        KLB_FREE(p_tmp);
    }

    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);

    KLB_FREE_BY(p_inter->p_w_list, klb_list_destroy);
    KLB_FREE_BY(p_inter->p_r_list, klb_list_destroy);
    KLB_FREE(p_inter->p_w_cur);
    KLB_FREE(p_inter->p_r_buf);
    KLB_FREE(p_inter);
}

static int cb_remove_klua_ktcp(void* p_lparam, void* p_wparam, int id)
{
    //klua_ktcp_t* p_ktcp = (klua_ktcp_t*)p_lparam;
    klua_ktcp_inter_t* p_inter = (klua_ktcp_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    KLB_FREE_BY(p_inter, free_klua_ktcp_inter);

    return 0;
}

static int cb_recv_klua_ktcp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_ktcp_t* p_ktcp = (klua_ktcp_t*)p_lparam;
    klua_ktcp_inter_t* p_inter = (klua_ktcp_inter_t*)p_wparam;

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

        int r = klb_socket_recv(p_socket, p_buf->p_buf, p_buf->buf_len);

        if (0 < r)
        {
            p_buf->end = r;
            recv += r;

            //
            if (call_lua_co_recv_klua_ktcp(p_ktcp, p_buf) < 0)
            {
                // 未处理, 放入列表
                int total_len = p_buf->end;
                klb_buf_t* p_tmp = klb_buf_malloc(total_len, false);
                memcpy(p_tmp->p_buf, p_buf->p_buf, p_buf->end);
                p_tmp->end = total_len;

                klb_list_push_tail(p_inter->p_r_list, p_tmp);

                p_inter->read_num += p_buf->end; // 缓存的数据量
                if (p_inter->param.read_max < p_inter->read_num)
                {
                    klb_socket_set_reading(p_socket, false); // 超过缓存量, 暂缓下次读取
                }
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

    }

    return recv;
}

static int cb_send_klua_ktcp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_ktcp_t* p_ktcp = (klua_ktcp_t*)p_lparam;
    klua_ktcp_inter_t* p_inter = (klua_ktcp_inter_t*)p_wparam;

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

    }

    return send;
}

static int cb_timer_klua_ktcp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_ktcp_t* p_ktcp = (klua_ktcp_t*)p_lparam;
    klua_ktcp_inter_t* p_inter = (klua_ktcp_inter_t*)p_wparam;

    if (p_inter->close)
    {
        return 0;
    }

    return 0;
}

////////////////////////////////////////
static klua_ktcp_t* new_klua_ktcp(lua_State* L)
{
    klua_ktcp_t* p_ktcp = (klua_ktcp_t*)lua_newuserdata(L, sizeof(klua_ktcp_t));
    KLB_MEMSET(p_ktcp, 0, sizeof(klua_ktcp_t));
    luaL_setmetatable(L, KLUA_KTCP_HANDLE);
    return p_ktcp;
}

static klua_ktcp_t* to_klua_ktcp(lua_State* L, int index)
{
    klua_ktcp_t* p_ktcp = (klua_ktcp_t*)luaL_checkudata(L, index, KLUA_KTCP_HANDLE);
    luaL_argcheck(L, NULL != p_ktcp, index, "'ktcp' expected");
    return p_ktcp;
}

static int klua_ktcp_tostring(lua_State* L)
{
    klua_ktcp_t* p_ktcp = to_klua_ktcp(L, 1);

    lua_pushfstring(L, "ktcp:%p", p_ktcp);
    return 1;
}

static int klua_ktcp_close(lua_State* L)
{
    klua_ktcp_t* p_ktcp = to_klua_ktcp(L, 1);

    if (NULL != p_ktcp->p_inter)
    {
        p_ktcp->p_inter->close = true;
        klb_socket_closing(p_ktcp->p_inter->p_socket);

        klb_multiplex_remove(p_ktcp->p_multi, p_ktcp->p_inter->id);

        // p_ktcp->p_inter 由 remove 后释放
        p_ktcp->p_inter = NULL;
    }

    return 0;
}

static int klua_ktcp_send(lua_State* L)
{
    klua_ktcp_t* p_ktcp = to_klua_ktcp(L, 1);

    size_t body_len = 0;
    const char* p_body = luaL_checklstring(L, 2, &body_len);

    if (body_len <= 0)
    {
        return 0;
    }

    klb_buf_t* p_buf = klb_buf_malloc(body_len, false);
    memcpy(p_buf->p_buf, p_body, body_len);
    p_buf->end = body_len;

    klb_list_push_tail(p_ktcp->p_inter->p_w_list, p_buf);
    klb_socket_set_writing(p_ktcp->p_inter->p_socket, true);

    return 0;
}

static int klua_ktcp_co_recv(lua_State* L)
{
    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "ktcp.connect():co_recv must in coroutine!");
        return 0;
    }

    klua_ktcp_t* p_ktcp = to_klua_ktcp(L, 1);

    if (0 < klb_list_size(p_ktcp->p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_list_pop_head(p_ktcp->p_inter->p_r_list);

        p_ktcp->p_inter->read_num -= (p_tmp->end - p_tmp->start);
        assert(0 <= p_ktcp->p_inter->read_num);
        if (p_ktcp->p_inter->read_num * 2 < p_ktcp->p_inter->param.read_max)
        {
            // 一半数据量以下, 开启读取
            klb_socket_set_reading(p_ktcp->p_inter->p_socket, true);
        }

        lua_pushlstring(L, p_tmp->p_buf + p_tmp->start, p_tmp->end - p_tmp->start);

        KLB_FREE(p_tmp);
        return 1;
    }

    assert(0 == p_ktcp->p_inter->read_num);
    assert(NULL == p_ktcp->co_recv);
    p_ktcp->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_ktcp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_ktcp_close },

        { "send",           klua_ktcp_send },
        { "co_recv",        klua_ktcp_co_recv },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_ktcp_close },
        { "__close",         klua_ktcp_close },
        { "__tostring",      klua_ktcp_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KTCP_HANDLE);/* metatable for KLUA_KUDP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

#define KLUA_KTCP_LISTEN_HANDLE    "KLUA_KTCP_LISTEN_HANDLE*"

typedef struct klua_ktcp_listen_param_t_
{
    int max_connect;                            ///< 操作系统监听socket, 能缓存的最大连接数
}klua_ktcp_listen_param_t;

typedef struct klua_ktcp_listen_t_
{
    klua_ktcp_listen_param_t    param;          ///< 参数

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
}klua_ktcp_listen_t;

////////////////////////////////////////
static klua_ktcp_listen_t* new_klua_ktcp_listen(lua_State* L)
{
    klua_ktcp_listen_t* p_listen = (klua_ktcp_listen_t*)lua_newuserdata(L, sizeof(klua_ktcp_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_ktcp_listen_t));
    luaL_setmetatable(L, KLUA_KTCP_LISTEN_HANDLE);
    return p_listen;
}

static klua_ktcp_listen_t* to_klua_ktcp_listen(lua_State* L, int index)
{
    klua_ktcp_listen_t* p_listen = (klua_ktcp_listen_t*)luaL_checkudata(L, index, KLUA_KTCP_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'ktcp listen' expected");
    return p_listen;
}

static int klua_ktcp_listen_tostring(lua_State* L)
{
    klua_ktcp_listen_t* p_listen = to_klua_ktcp_listen(L, 1);

    lua_pushfstring(L, "ktcp.listen:%p,port:%d", p_listen, klb_listen_port(p_listen->p_listen));
    return 1;
}

static int klua_ktcp_listen_close(lua_State* L)
{
    klua_ktcp_listen_t* p_listen = to_klua_ktcp_listen(L, 1);

    if (NULL != p_listen->p_listen)
    {
        klb_listen_close(p_listen->p_listen);
        KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    }

    return 0;
}

static int klua_ktcp_listen_co_accept(lua_State* L)
{
    klua_ktcp_listen_t* p_listen = to_klua_ktcp_listen(L, 1);

    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "khttp.listen():accept must in coroutine!");
        return 0;
    }

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////

static void klua_ktcp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_ktcp_listen_close },       ///< 

        { "co_accept",      klua_ktcp_listen_co_accept },  ///< 
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_ktcp_listen_close },
        { "__close",         klua_ktcp_listen_close },
        { "__tostring",      klua_ktcp_listen_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KTCP_LISTEN_HANDLE);/* metatable for KLUA_KTCP_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


#define KLUA_KTCP_RBUF_MAX          (1024 * 8)              ///< 临时读取缓存最大值
#define KLUA_KTCP_READ_MAX          (1024 * 512)            ///< 缓存最大值后,暂停读取

static void default_param_klua_ktcp(klua_ktcp_param_t* p_param)
{
    p_param->rbuf_max = KLUA_KTCP_RBUF_MAX; // 临时读取缓存最大值
    p_param->read_max = KLUA_KTCP_READ_MAX; // 缓存最大值后,暂停读取
    p_param->tls = false;
}

static void check_param_klua_ktcp(lua_State* L, int from, klua_ktcp_param_t* p_param)
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

klua_ktcp_t* new_connect_klua_ktcp(lua_State* L, klb_socket_fd fd, klua_ktcp_param_t* p_param)
{
    assert(INVALID_SOCKET != fd);

    klb_socket_t* p_socket = NULL;
    if (p_param->tls)
    {
        p_socket = klb_socket_tls_async_create(fd);
    }
    else
    {
        p_socket = klb_socket_async_create(fd);
    }

    klua_ktcp_inter_t* p_inter = KLB_MALLOCZ(klua_ktcp_inter_t, 1, 0);
    klua_ktcp_t* p_ktcp = new_klua_ktcp(L);
    p_ktcp->L = L;
    p_ktcp->p_env = klua_env_get_by_L(L);
    p_ktcp->p_multi = klua_ex_multiplex_get_by_L(L);
    p_ktcp->p_inter = p_inter;

    memcpy(&p_inter->param, p_param, sizeof(klua_ktcp_param_t));
    p_inter->p_ktcp = p_ktcp;
    p_inter->close = false;
    p_inter->p_socket = p_socket;

    p_inter->p_w_list = klb_list_create();
    p_inter->p_r_buf = klb_buf_malloc(p_inter->param.rbuf_max, false);
    p_inter->p_r_list = klb_list_create();

    // 放入 multiplex
    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klua_ktcp;
    o.cb_recv = cb_recv_klua_ktcp;
    o.cb_send = cb_send_klua_ktcp;
    o.cb_timer = cb_timer_klua_ktcp;
    o.p_lparam = p_ktcp;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_ktcp->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return p_ktcp;
}

static int lib_klua_ktcp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    // param
    klua_ktcp_param_t param = { 0 };
    default_param_klua_ktcp(&param);
    check_param_klua_ktcp(L, 2, &param);

    // socket fd
    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, 0);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    // new connect
    new_connect_klua_ktcp(L, fd, &param);
    return 1;
}

//////////////////////////////////////////////////////////////////////////

static int on_accept_klua_ktcp_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr)
{
    klua_ktcp_listen_t* p_listen = (klua_ktcp_listen_t*)ptr;

    bool close = true;

    if (p_listen->co_accept)
    {
        // 先判定是否是协程使用环境
        lua_State* L = klua_ex_coroutine_rawgeti(p_listen->p_ex_coroutine, p_listen->co_accept);
        if (NULL != L)
        {
            p_listen->co_accept = NULL; // 协程模式下, 一次"accept", 对应一次唤醒, 唤醒后清空

            // param
            klua_ktcp_param_t param = { 0 };
            default_param_klua_ktcp(&param);

            new_connect_klua_ktcp(L, fd, &param);               /* 1st argument */
            int status = lua_pcall(L, 1, 0, 0);                 /* do the call */
            klua_env_report_by_L(L, status);
        }
    }

    return 0;
}

static int lib_klua_ktcp_listen(lua_State* L)
{
    int port = (int)luaL_checkinteger(L, 1);

    klua_ktcp_listen_t* p_listen = new_klua_ktcp_listen(L);
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);
    p_listen->p_multi = klua_ex_multiplex_get_by_L(L);
    p_listen->p_ex_coroutine = klua_ex_get_coroutine(p_listen->p_env);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_ktcp_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, p_listen->param.max_connect);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_ktcp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",    lib_klua_ktcp_connect },

        { "listen",     lib_klua_ktcp_listen },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // ktcp listen
    klua_ktcp_listen_createmeta(L);

    // ktcp
    klua_ktcp_createmeta(L);

    return 1;
}
