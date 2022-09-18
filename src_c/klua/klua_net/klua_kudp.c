// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_multiplex.h"
#include "klbutil/klb_nlist.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klua/extension/klua_ex_coroutine.h"
#include <assert.h>


#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/un.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


#define KLUA_KUDP_HANDLE            "KLUA_KUDP_HANDLE*"     ///< Lua meta标示


typedef struct klua_kudp_t_ klua_kudp_t;


// 参数
typedef struct klua_kudp_param_t_
{
    int     rbuf_max;       ///< 临时读取缓存最大值
    int     read_max;       ///< 缓存数据最大值后,暂停读取
}klua_kudp_param_t;


// C
typedef struct klua_kudp_inter_t_
{
    klua_kudp_param_t           param;          ///< 参数

    klua_kudp_t*                p_kudp;         ///< kudp
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
        klb_nlist_t*             p_w_list;       ///< 待发送数据列表: klb_buf_t*
        klb_buf_t*              p_w_cur;        ///< 当前正在发送的缓存
    };

    // recv接收相关
    struct
    {
        klb_buf_t*              p_r_buf;        ///< 临时读取缓存

        klb_nlist_t*             p_r_list;       ///< 读取的数据列表: klb_buf_t*
        int                     read_num;       ///< p_r_list列表中缓存的数据量: 当达到一定值时, 暂停读取, 直到再次消费数据
    };
}klua_kudp_inter_t;


typedef struct klua_kudp_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        klua_env_t*             p_env;          ///< lua环境

        lua_State*              co_recvfrom;    ///< co_recvfrom携程
    };

    struct
    {
        klb_multiplex_t*        p_multi;        ///< 复用
    };

    klua_kudp_inter_t*          p_inter;        ///< C内部实现
}klua_kudp_t;


//////////////////////////////////////////////////////////////////////////

static int call_lua_reg_on_recvfrom_klua_kudp(klua_kudp_t* p_kudp, struct sockaddr_in* p_addr, klb_buf_t* p_buf)
{
    assert(NULL != p_kudp);

    if (NULL != p_kudp->co_recvfrom)
    {
        lua_State* L = klua_ex_coroutine_rawgeti(klua_ex_get_coroutine(p_kudp->p_env), p_kudp->co_recvfrom);
        if (NULL == L) return -1; // 未处理

        p_kudp->co_recvfrom = NULL; // 清空

        lua_pushstring(L, inet_ntoa(p_addr->sin_addr));
        lua_pushinteger(L, ntohs(p_addr->sin_port));
        lua_pushlstring(L, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

        int status = lua_pcall(L, 3, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

static void free_klua_kudp_inter(klua_kudp_inter_t* p_inter)
{
    // 清空
    while (0 < klb_nlist_size(p_inter->p_w_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_w_list);
        KLB_FREE(p_tmp);
    }

    // 清空
    while (0 < klb_nlist_size(p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_inter->p_r_list);
        KLB_FREE(p_tmp);
    }

    KLB_FREE_BY(p_inter->p_socket, klb_socket_destroy);

    KLB_FREE_BY(p_inter->p_w_list, klb_nlist_destroy);
    KLB_FREE_BY(p_inter->p_r_list, klb_nlist_destroy);
    KLB_FREE(p_inter->p_w_cur);
    KLB_FREE(p_inter->p_r_buf);
    KLB_FREE(p_inter);
}

static int cb_remove_klua_kudp(void* p_lparam, void* p_wparam, int id)
{
    //klua_kudp_t* p_kudp = (klua_kudp_t*)p_lparam;
    klua_kudp_inter_t* p_inter = (klua_kudp_inter_t*)p_wparam;

    // 可以被移除时, 与lua环境的 gc 没有必然的次序关系
    // 所以 1.与lua 关联的在 gc 或 gc 之前释放; 2. C部分在这里释放
    KLB_FREE_BY(p_inter, free_klua_kudp_inter);

    return 0;
}

static int cb_recv_klua_kudp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kudp_t* p_kudp = (klua_kudp_t*)p_lparam;
    klua_kudp_inter_t* p_inter = (klua_kudp_inter_t*)p_wparam;

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

        struct sockaddr_in addr = { 0 };
        int addr_len = sizeof(addr);

        int r = klb_socket_recvfrom(p_socket, (uint8_t*)p_buf->p_buf, p_buf->buf_len, (struct sockaddr*)&addr, &addr_len);

        if (0 < r)
        {
            p_buf->end = r;
            recv += r;

            //
            if (call_lua_reg_on_recvfrom_klua_kudp(p_kudp, &addr, p_buf) < 0)
            {
                // 未处理, 放入列表
                int total_len = p_buf->end + sizeof(addr);
                klb_buf_t* p_tmp = klb_buf_malloc(total_len, false);
                memcpy(p_tmp->p_buf, &addr, sizeof(addr));
                memcpy(p_tmp->p_buf + sizeof(addr), p_buf->p_buf, p_buf->end);

                p_tmp->start = sizeof(addr);
                p_tmp->end = total_len;

                klb_nlist_push_tail(p_inter->p_r_list, p_tmp);

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
        //assert(false);
    }

    return recv;
}

static int cb_send_klua_kudp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kudp_t* p_kudp = (klua_kudp_t*)p_lparam;
    klua_kudp_inter_t* p_inter = (klua_kudp_inter_t*)p_wparam;

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
        }

        klb_buf_t* p_buf = p_inter->p_w_cur;
        if (NULL == p_buf)
        {
            // 无数据可发送
            break;
        }

        struct sockaddr_in* p_addr = (struct sockaddr_in*)p_buf->p_buf;

        int w = klb_socket_sendto(p_socket, (const uint8_t*)(p_buf->p_buf + p_buf->start), p_buf->end - p_buf->start, (const struct sockaddr*)p_addr, sizeof(struct sockaddr_in));
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
                // select成功, UDP却第一次无法写, 出现错误: 可能原因为目标地址无法写
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

    }

    return send;
}

static int cb_timer_klua_kudp(void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_kudp_t* p_kudp = (klua_kudp_t*)p_lparam;
    klua_kudp_inter_t* p_inter = (klua_kudp_inter_t*)p_wparam;

    if (p_inter->close)
    {
        return 0;
    }

    return 0;
}

////////////////////////////////////////
static klua_kudp_t* new_klua_kudp(lua_State* L)
{
    klua_kudp_t* p_kudp = (klua_kudp_t*)lua_newuserdata(L, sizeof(klua_kudp_t));
    KLB_MEMSET(p_kudp, 0, sizeof(klua_kudp_t));
    luaL_setmetatable(L, KLUA_KUDP_HANDLE);
    return p_kudp;
}

static klua_kudp_t* to_klua_kudp(lua_State* L, int index)
{
    klua_kudp_t* p_kudp = (klua_kudp_t*)luaL_checkudata(L, index, KLUA_KUDP_HANDLE);
    luaL_argcheck(L, NULL != p_kudp, index, "'kudp' expected");
    return p_kudp;
}

static int klua_kudp_tostring(lua_State* L)
{
    klua_kudp_t* p_kudp = to_klua_kudp(L, 1);

    lua_pushfstring(L, "kudp:%p", p_kudp);
    return 1;
}

static int klua_kudp_close(lua_State* L)
{
    klua_kudp_t* p_kudp = to_klua_kudp(L, 1);

    if (NULL != p_kudp->p_inter)
    {
        p_kudp->p_inter->close = true;
        klb_socket_closing(p_kudp->p_inter->p_socket);

        klb_multiplex_remove(p_kudp->p_multi, p_kudp->p_inter->id);

        // p_kudp->p_inter 由 remove 后释放
        p_kudp->p_inter = NULL;
    }

    return 0;
}

static int klua_kudp_sendto(lua_State* L)
{
    klua_kudp_t* p_kudp = to_klua_kudp(L, 1);

    const char* p_ip = luaL_checkstring(L, 2);
    int port = luaL_checkinteger(L, 3);

    size_t body_len = 0;
    const char* p_body = luaL_checklstring(L, 4, &body_len);

    if (body_len <= 0)
    {
        return 0;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(p_ip); // "127.0.0.1"
    addr.sin_port = htons(port);

    klb_buf_t* p_buf = klb_buf_malloc(body_len + sizeof(addr), false);
    memcpy(p_buf->p_buf, &addr, sizeof(addr));
    memcpy(p_buf->p_buf + sizeof(addr), p_body, body_len);

    p_buf->start = sizeof(addr);
    p_buf->end = body_len + sizeof(addr);

    klb_nlist_push_tail(p_kudp->p_inter->p_w_list, p_buf);
    klb_socket_set_writing(p_kudp->p_inter->p_socket, true);

    return 0;
}

static int klua_kudp_co_recvfrom(lua_State* L)
{
    if (!klua_is_coroutine(L))
    {
        luaL_error(L, "kudp.new():co_recvfrom must in coroutine!");
        return 0;
    }

    klua_kudp_t* p_kudp = to_klua_kudp(L, 1);

    if (0 < klb_nlist_size(p_kudp->p_inter->p_r_list))
    {
        klb_buf_t* p_tmp = (klb_buf_t*)klb_nlist_pop_head(p_kudp->p_inter->p_r_list);

        p_kudp->p_inter->read_num -= (p_tmp->end - p_tmp->start);
        assert(0 <= p_kudp->p_inter->read_num);
        if (p_kudp->p_inter->read_num * 2 < p_kudp->p_inter->param.read_max)
        {
            // 一半数据量以下, 开启读取
            klb_socket_set_reading(p_kudp->p_inter->p_socket, true);
        }

        struct sockaddr_in* p_addr = (struct sockaddr_in*)p_tmp->p_buf;

        lua_pushstring(L, inet_ntoa(p_addr->sin_addr));
        lua_pushinteger(L, ntohs(p_addr->sin_port));
        lua_pushlstring(L, p_tmp->p_buf + p_tmp->start, p_tmp->end - p_tmp->start);

        KLB_FREE(p_tmp);
        return 3;
    }

    assert(0 == p_kudp->p_inter->read_num);
    assert(NULL == p_kudp->co_recvfrom);
    p_kudp->co_recvfrom = L;

    return lua_yield(L, lua_gettop(L));
}

static void klua_kudp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_kudp_close },

        { "sendto",         klua_kudp_sendto },
        { "co_recvfrom",    klua_kudp_co_recvfrom },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kudp_close },
        { "__close",         klua_kudp_close },
        { "__tostring",      klua_kudp_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KUDP_HANDLE);/* metatable for KLUA_KUDP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////


#define KLUA_KUDP_RBUF_MAX          (1024 * 8)              ///< 临时读取缓存最大值
#define KLUA_KUDP_READ_MAX          (1024 * 512)            ///< 缓存最大值后,暂停读取

static void default_param_klua_kudp(klua_kudp_param_t* p_param)
{
    p_param->rbuf_max = KLUA_KUDP_RBUF_MAX; // 临时读取缓存最大值
    p_param->read_max = KLUA_KUDP_READ_MAX; // 缓存最大值后,暂停读取
}

static void check_param_klua_kudp(lua_State* L, int from, klua_kudp_param_t* p_param)
{
    static const char* options[] = { "rbuf_max", "read_max", NULL };

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
        default:
            break;
        }
    }
}

static int lib_klua_kudp_new(lua_State* L)
{
    // socket
    klb_socket_fd fd = klb_socket_udp();
    assert(INVALID_SOCKET != fd);

    if (LUA_TSTRING == lua_type(L, 1) && LUA_TNUMBER == lua_type(L, 2))
    {
        const char* p_ip = luaL_checkstring(L, 1);
        int port = luaL_checkinteger(L, 2);

        struct sockaddr_in addr = { 0 };
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(p_ip); // "127.0.0.1"
        addr.sin_port = htons(port);

        klb_socket_bind(fd, (const struct sockaddr*)&addr, sizeof(addr));
    }

    // param
    klua_kudp_param_t param = { 0 };
    default_param_klua_kudp(&param);
    check_param_klua_kudp(L, 2, &param);

    klb_socket_t* p_socket = klb_socket_async_create(fd);
    klua_kudp_inter_t* p_inter = KLB_MALLOCZ(klua_kudp_inter_t, 1, 0);
    klua_kudp_t* p_kudp = new_klua_kudp(L);
    p_kudp->L = L;
    p_kudp->p_env = klua_env_get_by_L(L);
    p_kudp->p_multi = klua_ex_multiplex_get_by_L(L);
    p_kudp->p_inter = p_inter;

    memcpy(&p_inter->param, &param, sizeof(param));
    p_inter->p_kudp = p_kudp;
    p_inter->close = false;
    p_inter->p_socket = p_socket;

    p_inter->p_w_list = klb_nlist_create();
    p_inter->p_r_buf = klb_buf_malloc(p_inter->param.rbuf_max, false);
    p_inter->p_r_list = klb_nlist_create();

    // 放入 multiplex
    klb_multiplex_ops_t o = { 0 };
    o.cb_remove = cb_remove_klua_kudp;
    o.cb_recv = cb_recv_klua_kudp;
    o.cb_send = cb_send_klua_kudp;
    o.cb_timer = cb_timer_klua_kudp;
    o.p_lparam = p_kudp;
    o.p_wparam = p_inter;

    p_inter->id = klb_multiplex_push_socket(p_kudp->p_multi, p_inter->p_socket, &o);
    assert(0 <= p_inter->id);

    // 开启读取
    klb_socket_set_reading(p_inter->p_socket, true);

    return 1;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_kudp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new",        lib_klua_kudp_new },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // kudp
    klua_kudp_createmeta(L);

    return 1;
}
