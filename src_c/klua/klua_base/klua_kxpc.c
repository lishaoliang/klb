// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klua/klua.h"
#include "klbrt/klb_xpc.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include <assert.h>
#include <string.h>


#define KLUA_KXPC_HANDLE    "KLUA_KXPC_HANDLE*"
#define KLB_XPC_RECV_MAX    (8u * 1024u)


typedef struct klua_kxpc_t_
{
    klb_xpc_t*      p_xpc;
}klua_kxpc_t;


//////////////////////////////////////////////////////////////////////////


static klua_kxpc_t* new_klua_kxpc(lua_State* L)
{
    klua_kxpc_t* p = (klua_kxpc_t*)lua_newuserdata(L, sizeof(klua_kxpc_t));
    KLB_MEMSET(p, 0, sizeof(*p));
    luaL_setmetatable(L, KLUA_KXPC_HANDLE);
    return p;
}


static klua_kxpc_t* to_klua_kxpc(lua_State* L, int index)
{
    klua_kxpc_t* p = (klua_kxpc_t*)luaL_checkudata(L, index, KLUA_KXPC_HANDLE);
    luaL_argcheck(L, NULL != p, index, "'kxpc' expected");
    return p;
}


static int pack_from_klua_kxpc(lua_State* L, int base_idx, const void** pp_data, uint32_t* p_len, klb_buf_t** pp_buf)
{
    klb_buf_t* p_buf = NULL;
    uint32_t len = 0;

    assert(NULL != pp_data);
    assert(NULL != p_len);
    assert(NULL != pp_buf);

    p_buf = luaseri_map_binary_pack(L, base_idx);
    len = (uint32_t)(p_buf->end - p_buf->start);
    *pp_data = p_buf->p_buf + p_buf->start;
    *p_len = len;
    *pp_buf = p_buf;
    return 0;
}


//////////////////////////////////////////////////////////////////////////


static int klua_kxpc_close(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);

    if (NULL != p->p_xpc)
    {
        klb_xpc_destroy(p->p_xpc);
        p->p_xpc = NULL;
    }

    return 0;
}


static int klua_kxpc_tostring(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);

    if (NULL == p->p_xpc)
    {
        lua_pushstring(L, "kxpc:closed");
    }
    else
    {
        lua_pushfstring(L, "kxpc:%s", klb_xpc_name(p->p_xpc));
    }

    return 1;
}


static int klua_kxpc_name(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);

    if (NULL == p->p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, klb_xpc_name(p->p_xpc));
    return 1;
}


static int klua_kxpc_is_owner(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);

    if (NULL == p->p_xpc)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, klb_xpc_is_owner(p->p_xpc) ? 1 : 0);
    return 1;
}


static int klua_kxpc_post(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);
    const char* p_dst = NULL;
    const void* p_data = NULL;
    uint32_t len = 0;
    klb_buf_t* p_buf = NULL;
    int rc = 0;

    if (NULL == p->p_xpc)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    p_dst = luaL_checkstring(L, 2);
    pack_from_klua_kxpc(L, 2, &p_data, &len, &p_buf);
    rc = klb_xpc_post(p->p_xpc, p_dst, p_data, len);
    KLB_FREE(p_buf);
    lua_pushboolean(L, (0 == rc) ? 1 : 0);
    return 1;
}


static int klua_kxpc_notify(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);
    const char* p_dst = NULL;
    const void* p_data = NULL;
    uint32_t len = 0;
    klb_buf_t* p_buf = NULL;
    int rc = 0;

    if (NULL == p->p_xpc)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    p_dst = luaL_checkstring(L, 2);
    pack_from_klua_kxpc(L, 2, &p_data, &len, &p_buf);
    rc = klb_xpc_notify(p->p_xpc, p_dst, p_data, len);
    KLB_FREE(p_buf);
    lua_pushboolean(L, (0 == rc) ? 1 : 0);
    return 1;
}


static int klua_kxpc_response(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);
    const char* p_dst = NULL;
    uint32_t corr = 0;
    const void* p_data = NULL;
    uint32_t len = 0;
    klb_buf_t* p_buf = NULL;
    int rc = 0;

    if (NULL == p->p_xpc)
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    p_dst = luaL_checkstring(L, 2);
    corr = (uint32_t)luaL_checkinteger(L, 3);
    pack_from_klua_kxpc(L, 3, &p_data, &len, &p_buf);
    rc = klb_xpc_response(p->p_xpc, p_dst, corr, p_data, len);
    KLB_FREE(p_buf);
    lua_pushboolean(L, (0 == rc) ? 1 : 0);
    return 1;
}


static int klua_kxpc_request(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);
    const char* p_dst = NULL;
    uint32_t timeout_ms = 0;
    const void* p_data = NULL;
    uint32_t len = 0;
    klb_buf_t* p_buf = NULL;
    uint8_t out[KLB_XPC_RECV_MAX];
    uint32_t out_len = 0;
    int rc = 0;
    int n = 0;

    if (NULL == p->p_xpc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "closed");
        return 2;
    }

    p_dst = luaL_checkstring(L, 2);
    timeout_ms = (uint32_t)luaL_checkinteger(L, 3);
    pack_from_klua_kxpc(L, 3, &p_data, &len, &p_buf);
    rc = klb_xpc_request(p->p_xpc, p_dst, p_data, len, out, sizeof(out), &out_len, timeout_ms);
    KLB_FREE(p_buf);

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "failed");
        return 2;
    }

    n = luaseri_map_binary_unpack(L, 0, (const char*)out, (int)out_len);
    return n;
}


static int klua_kxpc_recv(lua_State* L)
{
    klua_kxpc_t* p = to_klua_kxpc(L, 1);
    uint32_t timeout_ms = 0;
    uint8_t out[KLB_XPC_RECV_MAX];
    char src[KLB_XPC_NAME_BUF];
    uint32_t out_len = 0;
    uint32_t corr = 0;
    int type = 0;
    int rc = 0;
    int n = 0;

    if (NULL == p->p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    timeout_ms = (uint32_t)luaL_optinteger(L, 2, (lua_Integer)(uint32_t)-1);
    rc = klb_xpc_recv(p->p_xpc, &type, src, out, sizeof(out), &out_len, &corr, timeout_ms);
    if (0 != rc)
    {
        lua_settop(L, 0);
        lua_pushnil(L);
        return 1;
    }

    // 清掉 self/timeout, 再压返回值; unpack 的 settop(base) 不会误留入参
    lua_settop(L, 0);
    lua_pushinteger(L, type);
    lua_pushstring(L, src);
    lua_pushinteger(L, (lua_Integer)corr);
    n = luaseri_map_binary_unpack(L, 3, (const char*)out, (int)out_len);
    return 3 + n;
}


static void createmeta_klua_kxpc(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "close",      klua_kxpc_close },
        { "name",       klua_kxpc_name },
        { "is_owner",   klua_kxpc_is_owner },
        { "post",       klua_kxpc_post },
        { "notify",     klua_kxpc_notify },
        { "response",   klua_kxpc_response },
        { "request",    klua_kxpc_request },
        { "recv",       klua_kxpc_recv },
        { NULL,         NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",    NULL },
        { "__gc",       klua_kxpc_close },
        { "__close",    klua_kxpc_close },
        { "__tostring", klua_kxpc_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KXPC_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////


static int lib_klua_kxpc_new_module(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_create(p_name);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_new_module_unix(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_create_unix(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_open(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_open(p_name);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_open_unix(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_open_unix(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_new_module_npipe(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_create_npipe(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_open_npipe(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_open_npipe(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_new_module_sysmq(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_create_sysmq(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


static int lib_klua_kxpc_open_sysmq(lua_State* L)
{
    const char* p_name = luaL_checkstring(L, 1);
    const char* p_path = luaL_optstring(L, 2, NULL);
    klua_kxpc_t* p = NULL;
    klb_xpc_t* p_xpc = NULL;

    p_xpc = klb_xpc_open_sysmq(p_name, p_path);
    if (NULL == p_xpc)
    {
        lua_pushnil(L);
        return 1;
    }

    p = new_klua_kxpc(L);
    p->p_xpc = p_xpc;
    return 1;
}


int klua_open_kxpc(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new_module",         lib_klua_kxpc_new_module },
        { "new_module_unix",    lib_klua_kxpc_new_module_unix },
        { "new_module_npipe",   lib_klua_kxpc_new_module_npipe },
        { "new_module_sysmq",   lib_klua_kxpc_new_module_sysmq },
        { "open",               lib_klua_kxpc_open },
        { "open_unix",          lib_klua_kxpc_open_unix },
        { "open_npipe",         lib_klua_kxpc_open_npipe },
        { "open_sysmq",         lib_klua_kxpc_open_sysmq },
        { "new",                lib_klua_kxpc_open },
        { NULL,                 NULL }
    };

    luaL_newlib(L, lib);
    createmeta_klua_kxpc(L);

    lua_pushinteger(L, KLB_XPC_POST);
    lua_setfield(L, -2, "POST");
    lua_pushinteger(L, KLB_XPC_REQUEST);
    lua_setfield(L, -2, "REQUEST");
    lua_pushinteger(L, KLB_XPC_RESPONSE);
    lua_setfield(L, -2, "RESPONSE");
    lua_pushinteger(L, KLB_XPC_NOTIFY);
    lua_setfield(L, -2, "NOTIFY");

    lua_pushinteger(L, KLB_XPC_TRANSPORT_MSGQ);
    lua_setfield(L, -2, "TRANSPORT_MSGQ");
    lua_pushinteger(L, KLB_XPC_TRANSPORT_UNIX);
    lua_setfield(L, -2, "TRANSPORT_UNIX");
    lua_pushinteger(L, KLB_XPC_TRANSPORT_NPIPE);
    lua_setfield(L, -2, "TRANSPORT_NPIPE");
    lua_pushinteger(L, KLB_XPC_TRANSPORT_SYSMQ);
    lua_setfield(L, -2, "TRANSPORT_SYSMQ");

    return 1;
}

// end
