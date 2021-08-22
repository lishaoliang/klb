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
#include "klbnet/klb_listen.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// http listen

#define KLUA_KHTTP_LISTEN_HANDLE    "KLUA_KHTTP_LISTEN_HANDLE*"


typedef struct klua_khttp_listen_t_
{
    // Lua相关
    struct
    {
        lua_State*  L;       ///< L
        klua_env_t* p_env;                  ///< lua环境

        int         reg_on_recv;            ///< Lua脚本函数
    };

    struct
    {
        klua_ex_multiplex_t* p_ex;           ///< 复用扩展
        klb_multiplex_t*    p_multi;        ///< 复用
    };

    klb_listen_t* p_listen;

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

static void free_klua_khttp_listen(klua_khttp_listen_t* p_listen)
{
}

static int klua_khttp_listen_close(lua_State* L)
{
    return 0;
}

static int klua_khttp_listen_gc(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    return 0;
}

static int klua_khttp_listen_tostring(lua_State* L)
{
    klua_khttp_listen_t* p_listen = to_klua_khttp_listen(L, 1);

    lua_pushfstring(L, "khttp listen:%p", p_listen);
    return 1;
}

//////////////////////////////////////////////////


static void klua_khttp_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_khttp_listen_close },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        {"__index",         NULL},  /* place holder */
        {"__gc",            klua_khttp_listen_gc},
        {"__close",         klua_khttp_listen_gc},
        {"__tostring",      klua_khttp_listen_tostring},
        {NULL,              NULL}
    };

    luaL_newmetatable(L, KLUA_KHTTP_LISTEN_HANDLE);/* metatable for KLUA_KHTTP_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

///////////////////////////////////////

static int on_accept_klua_khttp_listen(void* ptr, klb_socket_fd fd, const struct sockaddr_in* p_addr)
{
    klua_khttp_listen_t* p_listen = (klua_khttp_listen_t*)ptr;

    KLB_SOCKET_CLOSE(fd);

    return 0;
}

static int klua_khttp_listen(lua_State* L)
{
    int port = luaL_checkinteger(L, 1);

    klua_khttp_listen_t* p_listen = new_klua_khttp_listen(L);
    p_listen->L = L;
    p_listen->p_env = klua_env_get_by_L(L);;
    p_listen->p_ex = klua_ex_get_multiplex(p_listen->p_env);
    p_listen->p_multi = klua_ex_multiplex_get(p_listen->p_ex);

    p_listen->p_listen = klb_listen_create(p_listen->p_multi);

    klb_listen_set_accept(p_listen->p_listen, on_accept_klua_khttp_listen, p_listen);

    klb_listen_open(p_listen->p_listen, port, 20);

    return 1;
}
