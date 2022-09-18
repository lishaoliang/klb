// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "kpa_rtsp/kpa_rtsp_client.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_multiplex.h"
#include <stdlib.h>
#include <assert.h>


#define KPA_RTSP_CLIENT_HANDLE      "KPA_RTSP_CLIENT_HANDLE*"



//////////////////////////////////////////////////////////////////////////
// lua

static kpa_rtsp_client_t* new_kpa_rtsp_client(lua_State* L)
{
    kpa_rtsp_client_t* p_rtsp = (kpa_rtsp_client_t*)lua_newuserdata(L, sizeof(kpa_rtsp_client_t));
    KLB_MEMSET(p_rtsp, 0, sizeof(kpa_rtsp_client_t));
    luaL_setmetatable(L, KPA_RTSP_CLIENT_HANDLE);
    return p_rtsp;
}

static kpa_rtsp_client_t* to_kpa_rtsp_client(lua_State* L, int index)
{
    kpa_rtsp_client_t* p_rtsp = (kpa_rtsp_client_t*)luaL_checkudata(L, index, KPA_RTSP_CLIENT_HANDLE);
    luaL_argcheck(L, NULL != p_rtsp, index, "'kpa_rtsp' expected");
    return p_rtsp;
}

static int kpa_rtsp_client_tostring(lua_State* L)
{
    kpa_rtsp_client_t* p_rtsp = to_kpa_rtsp_client(L, 1);

    lua_pushfstring(L, "kpa_rtsp:%p", p_rtsp);
    return 1;
}

static int kpa_rtsp_client_close(lua_State* L)
{
    return 0;
}

void kpa_rtsp_client_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",          kpa_rtsp_client_close },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            kpa_rtsp_client_close },
        { "__close",         kpa_rtsp_client_close },
        { "__tostring",      kpa_rtsp_client_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KPA_RTSP_CLIENT_HANDLE); /* metatable for x handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

int kpa_rtsp_client_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);
    int port = (int)luaL_checkinteger(L, 2);
    const char* p_url = luaL_checkstring(L, 3);

    kpa_rtsp_client_t* p_rtsp = new_kpa_rtsp_client(L);

    p_rtsp->p_multi = klua_multiplex_get_by_L(L);
    p_rtsp->p_ex_co = klua_coroutine_get_by_L(L);

    p_rtsp->p_inter = kpa_rtsp_client_inter_create(p_rtsp->p_multi, p_host, port, p_url);

    return 1;
}
