// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_net/klua_krtsp_serve.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klbnet/klb_netmulti.h"
#include "klbutil/klb_nlist.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// klua rtsp serve

#define KLUA_KRTSPSERVE_HANDLE          "KRTSPSERVE_HANDLE*"

typedef struct klua_krtspserve_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程
        klua_env_t*             p_env;          ///< lua环境
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 服务连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
    };
}klua_krtspserve_t;

////////////////////////////////////////
static klua_krtspserve_t* new_klua_krtspserve(lua_State* L)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)lua_newuserdata(L, sizeof(klua_krtspserve_t));
    KLB_MEMSET(p_serve, 0, sizeof(klua_krtspserve_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_HANDLE);
    return p_serve;
}

static klua_krtspserve_t* to_klua_krtspserve(lua_State* L, int index)
{
    klua_krtspserve_t* p_serve = (klua_krtspserve_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_HANDLE);
    luaL_argcheck(L, NULL != p_serve, index, "'krtspserve' expected");
    return p_serve;
}

static int klua_krtspserve_tostring(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    lua_pushfstring(L, "krtspserve:%p", p_serve);
    return 1;
}

static int klua_krtspserve_close(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    return 0;
}

static int klua_krtspserve_send_text(lua_State* L)
{
    klua_krtspserve_t* p_serve = to_klua_krtspserve(L, 1);

    return 0;
}

static int klua_krtspserve_co_recv(lua_State* L)
{
    klua_krtspserve_t* p_listen = to_klua_krtspserve(L, 1);
    klua_check_coroutine(L, "krtspserve:co_recv must in coroutine!");

    assert(NULL == p_listen->co_recv);
    p_listen->co_recv = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtspserve_close },            ///< 

        { "send_text",      klua_krtspserve_send_text },        ///< 

        { "co_recv",        klua_krtspserve_co_recv },          ///< 

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_close },
        { "__close",        klua_krtspserve_close },
        { "__tostring",     klua_krtspserve_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_HANDLE);   /* metatable for KLUA_KRTSPSERVE_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}



//////////////////////////////////////////////////////////////////////////
// klua rtsp serve listen

#define KLUA_KRTSPSERVE_LISTEN_HANDLE    "KRTSPSERVE_LISTEN_HANDLE*"


typedef struct klua_krtspserve_listen_t_
{
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
        klb_netmulti_t*         p_multi;        ///< 复用
        klua_ex_coroutine_t*    p_ex_coroutine; ///< ex co
    };
}klua_krtspserve_listen_t;

////////////////////////////////////////
static klua_krtspserve_listen_t* new_klua_krtspserve_listen(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)lua_newuserdata(L, sizeof(klua_krtspserve_listen_t));
    KLB_MEMSET(p_listen, 0, sizeof(klua_krtspserve_listen_t));
    luaL_setmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    return p_listen;
}

static klua_krtspserve_listen_t* to_klua_krtspserve_listen(lua_State* L, int index)
{
    klua_krtspserve_listen_t* p_listen = (klua_krtspserve_listen_t*)luaL_checkudata(L, index, KLUA_KRTSPSERVE_LISTEN_HANDLE);
    luaL_argcheck(L, NULL != p_listen, index, "'krtspserve listen' expected");
    return p_listen;
}

static int klua_krtspserve_listen_tostring(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    lua_pushfstring(L, "krtspserve.listen:%p", p_listen);
    return 1;
}

static int klua_krtspserve_listen_close(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);

    //if (NULL != p_listen->p_listen)
    //{
    //    klb_listen_close(p_listen->p_listen);
    //    KLB_FREE_BY(p_listen->p_listen, klb_listen_destroy);
    //}

    return 0;
}

static int klua_krtspserve_listen_co_accept(lua_State* L)
{
    klua_krtspserve_listen_t* p_listen = to_klua_krtspserve_listen(L, 1);
    klua_check_coroutine(L, "krtspserve:co_accept must in coroutine!");

    assert(NULL == p_listen->co_accept);
    p_listen->co_accept = L;

    return lua_yield(L, lua_gettop(L));
}

//////////////////////////////////////////////////
// createmeta

void klua_krtspserve_listen_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          klua_krtspserve_listen_close },       ///< 

        { "co_accept",      klua_krtspserve_listen_co_accept },  ///< 
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_krtspserve_listen_close },
        { "__close",        klua_krtspserve_listen_close },
        { "__tostring",     klua_krtspserve_listen_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPSERVE_LISTEN_HANDLE);/* metatable for KLUA_KRTSPSERVE_LISTEN_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////

int klua_krtsp_serve_listen(lua_State* L)
{
    return 0;
}

//end
