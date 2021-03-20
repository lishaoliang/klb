#include "klua/klua.h"
#include "socket/klb_socket.h"
#include "socket/klb_socket_base.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "mem/klb_mem.h"
#include "parser/http_parser.h"
#include "log/klb_log.h"
#include <assert.h>


#define KLUA_KHTTP_HANDLE     "KLUA_KHTTP_HANDLE*"


typedef struct klua_khttp_t_
{
    klua_env_t*             p_env;
    klua_ex_multiplex_t*    p_multiplex;

    klb_socket_t*           p_socket;
    int                     id;
}klua_khttp_t;


//////////////////////////////////////////////////////////////////////////

static int cb_klua_khttp_recv(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;


    char buf[4096] = { 0 };
    int r = klb_socket_recv(p_khttp->p_socket, buf, 4000);

    KLB_LOG("recv:%s\n", buf);


    return 0;
}

static int cb_klua_khttp_send(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now)
{
    klua_khttp_t* p_khttp = (klua_khttp_t*)p_lparam;

    return 0;
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

static int klua_khttp_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    lua_Integer port = luaL_checkinteger(L, 2);

    klua_env_t* p_env = klua_env_get_by_L(L);

    klb_socket_fd fd = klb_socket_connect(p_host, (int)port, -1);
    if (INVALID_SOCKET == fd)
    {
        lua_pushnil(L);
        return 1;
    }

    //klb_socket_set_block(fd, false);

    klb_socket_t* p_socket = KLB_MALLOC(klb_socket_t, 1, 0);
    KLB_MEMSET(p_socket, 0, sizeof(klb_socket_t));
    klb_socket_base_construct(p_socket, fd);

    klua_khttp_t* p_khttp = new_klua_khttp(L);
    p_khttp->p_env = p_env;
    p_khttp->p_socket = p_socket;
    p_khttp->p_multiplex = klua_ex_get_multiplex(p_khttp->p_env);

    p_khttp->id = 100;

    klua_ex_multiplex_obj_t o = { 0 };
    o.cb_recv = cb_klua_khttp_recv;
    o.cb_send = cb_klua_khttp_send;
    o.p_lparam = p_khttp;

    klua_ex_multiplex_push_socket(p_khttp->p_multiplex, p_khttp->id, p_socket, &o);

    return 1;
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

static int klua_khttp_send(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);
    
    size_t len = 0;
    const char* ptr = luaL_checklstring(L, 2, &len);

    klb_sleep(100);

    int send = klb_socket_send(p_khttp->p_socket, ptr, len);
    KLB_LOG("send = %d\n", send);

    return 0;
}

static int klua_khttp_close(lua_State* L)
{
    klua_khttp_t* p_khttp = to_klua_khttp(L, 1);

    return 0;
}

static void klua_khttp_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
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

    luaL_newmetatable(L, KLUA_KHTTP_HANDLE);  /* metatable for KLUA_KHTTP_META handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

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
