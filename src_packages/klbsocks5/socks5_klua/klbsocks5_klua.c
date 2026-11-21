// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbsocks5/klbsocks5.h"
#include "klbsocks5/core/klb_socks5_wire.h"
#include "klua/klua.h"

#include <string.h>


static int klua_ksocks5_available(lua_State* L)
{
    lua_pushboolean(L, 1);
    return 1;
}


static int push_encode_result_(lua_State* L, uint8_t* p_buf, int n)
{
    if (n < 0)
    {
        lua_pushnil(L);
        lua_pushstring(L, "encode fail");
        return 2;
    }

    lua_pushlstring(L, (const char*)p_buf, (size_t)n);
    return 1;
}


static int klua_ksocks5_encode_greeting(lua_State* L)
{
    uint8_t buf[8];
    int with_auth = 0;
    int n = 0;

    with_auth = lua_toboolean(L, 1);
    n = klb_socks5_encode_greeting(buf, (int)sizeof(buf), with_auth);
    return push_encode_result_(L, buf, n);
}


static int klua_ksocks5_parse_method_select(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    uint8_t method = 0;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_method_select((const uint8_t*)p, (int)len, &method);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad method select");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)method);
    return 1;
}


static int klua_ksocks5_encode_userpass(lua_State* L)
{
    size_t user_len = 0;
    size_t pass_len = 0;
    const char* p_user = NULL;
    const char* p_pass = NULL;
    uint8_t buf[KLB_SOCKS5_ENCODE_MAX];
    int n = 0;

    p_user = luaL_checklstring(L, 1, &user_len);
    p_pass = luaL_checklstring(L, 2, &pass_len);
    n = klb_socks5_encode_userpass(buf, (int)sizeof(buf),
        p_user, (int)user_len, p_pass, (int)pass_len);
    return push_encode_result_(L, buf, n);
}


static int klua_ksocks5_parse_userpass_status(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    uint8_t status = 0;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_userpass_status((const uint8_t*)p, (int)len, &status);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad auth status");
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)status);
    return 1;
}


static int klua_ksocks5_encode_connect(lua_State* L)
{
    size_t host_len = 0;
    const char* p_host = NULL;
    int port = 0;
    uint8_t buf[KLB_SOCKS5_ENCODE_MAX];
    int n = 0;

    p_host = luaL_checklstring(L, 1, &host_len);
    port = (int)luaL_checkinteger(L, 2);

    if (port < 0)
    {
        port = 0;
    }

    if (port > 65535)
    {
        port = 65535;
    }

    n = klb_socks5_encode_connect(buf, (int)sizeof(buf),
        p_host, (int)host_len, (uint16_t)port);
    return push_encode_result_(L, buf, n);
}


static int klua_ksocks5_encode_method_select(lua_State* L)
{
    uint8_t buf[4];
    uint8_t method = 0;
    int n = 0;

    method = (uint8_t)luaL_checkinteger(L, 1);
    n = klb_socks5_encode_method_select(buf, (int)sizeof(buf), method);
    return push_encode_result_(L, buf, n);
}


static int klua_ksocks5_parse_greeting(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    uint8_t methods[256];
    int nmethods = 0;
    int consumed = 0;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_greeting((const uint8_t*)p, (int)len,
        methods, (int)sizeof(methods), &nmethods, &consumed);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad greeting");
        return 2;
    }

    lua_createtable(L, 0, 2);
    lua_pushlstring(L, (const char*)methods, (size_t)nmethods);
    lua_setfield(L, -2, "methods");
    lua_pushinteger(L, (lua_Integer)consumed);
    lua_setfield(L, -2, "consumed");
    return 1;
}


static int klua_ksocks5_choose_method(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    int want_user = 0;
    uint8_t method = 0;

    p = luaL_checklstring(L, 1, &len);
    want_user = lua_toboolean(L, 2);
    method = klb_socks5_choose_method((const uint8_t*)p, (int)len, want_user);
    lua_pushinteger(L, (lua_Integer)method);
    return 1;
}


static int klua_ksocks5_parse_userpass(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    char user[256];
    char pass[256];
    int user_len = 0;
    int pass_len = 0;
    int consumed = 0;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_userpass_req((const uint8_t*)p, (int)len,
        user, (int)sizeof(user), &user_len,
        pass, (int)sizeof(pass), &pass_len, &consumed);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad auth");
        return 2;
    }

    lua_createtable(L, 0, 3);
    lua_pushlstring(L, user, (size_t)user_len);
    lua_setfield(L, -2, "user");
    lua_pushlstring(L, pass, (size_t)pass_len);
    lua_setfield(L, -2, "pass");
    lua_pushinteger(L, (lua_Integer)consumed);
    lua_setfield(L, -2, "consumed");
    return 1;
}


static int klua_ksocks5_encode_userpass_status(lua_State* L)
{
    uint8_t buf[4];
    uint8_t status = 0;
    int n = 0;

    status = (uint8_t)luaL_checkinteger(L, 1);
    n = klb_socks5_encode_userpass_status(buf, (int)sizeof(buf), status);
    return push_encode_result_(L, buf, n);
}


static int klua_ksocks5_parse_request(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    klb_socks5_request_t req;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_request((const uint8_t*)p, (int)len, &req);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad request");
        return 2;
    }

    lua_createtable(L, 0, 5);
    lua_pushinteger(L, (lua_Integer)req.cmd);
    lua_setfield(L, -2, "cmd");
    lua_pushinteger(L, (lua_Integer)req.atyp);
    lua_setfield(L, -2, "atyp");
    lua_pushstring(L, req.host);
    lua_setfield(L, -2, "host");
    lua_pushinteger(L, (lua_Integer)req.port);
    lua_setfield(L, -2, "port");
    lua_pushinteger(L, (lua_Integer)req.consumed);
    lua_setfield(L, -2, "consumed");
    return 1;
}


static int klua_ksocks5_parse_reply(lua_State* L)
{
    size_t len = 0;
    const char* p = NULL;
    klb_socks5_reply_t rsp;
    int rc = 0;

    p = luaL_checklstring(L, 1, &len);
    rc = klb_socks5_parse_reply((const uint8_t*)p, (int)len, &rsp);

    if (1 == rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "short");
        return 2;
    }

    if (0 != rc)
    {
        lua_pushnil(L);
        lua_pushstring(L, "bad reply");
        return 2;
    }

    lua_createtable(L, 0, 5);
    lua_pushinteger(L, (lua_Integer)rsp.rep);
    lua_setfield(L, -2, "rep");
    lua_pushinteger(L, (lua_Integer)rsp.atyp);
    lua_setfield(L, -2, "atyp");
    lua_pushstring(L, rsp.host);
    lua_setfield(L, -2, "host");
    lua_pushinteger(L, (lua_Integer)rsp.port);
    lua_setfield(L, -2, "port");
    lua_pushinteger(L, (lua_Integer)rsp.consumed);
    lua_setfield(L, -2, "consumed");
    return 1;
}


static int klua_ksocks5_encode_reply(lua_State* L)
{
    uint8_t buf[16];
    uint8_t rep = 0;
    int n = 0;

    rep = (uint8_t)luaL_checkinteger(L, 1);
    n = klb_socks5_encode_reply(buf, (int)sizeof(buf), rep);
    return push_encode_result_(L, buf, n);
}


int klua_open_ksocks5(lua_State* L)
{
    static luaL_Reg lib[] = {
        { "available",              klua_ksocks5_available },
        { "encode_greeting",        klua_ksocks5_encode_greeting },
        { "parse_method_select",    klua_ksocks5_parse_method_select },
        { "encode_userpass",        klua_ksocks5_encode_userpass },
        { "parse_userpass_status",  klua_ksocks5_parse_userpass_status },
        { "encode_connect",         klua_ksocks5_encode_connect },
        { "encode_method_select",   klua_ksocks5_encode_method_select },
        { "parse_greeting",         klua_ksocks5_parse_greeting },
        { "choose_method",          klua_ksocks5_choose_method },
        { "parse_userpass",         klua_ksocks5_parse_userpass },
        { "encode_userpass_status", klua_ksocks5_encode_userpass_status },
        { "parse_request",          klua_ksocks5_parse_request },
        { "parse_reply",            klua_ksocks5_parse_reply },
        { "encode_reply",           klua_ksocks5_encode_reply },
        { NULL, NULL }
    };

    luaL_newlib(L, lib);

    lua_pushinteger(L, KLB_SOCKS5_VER);
    lua_setfield(L, -2, "VER");
    lua_pushinteger(L, KLB_SOCKS5_METHOD_NONE);
    lua_setfield(L, -2, "METHOD_NONE");
    lua_pushinteger(L, KLB_SOCKS5_METHOD_USERPASS);
    lua_setfield(L, -2, "METHOD_USERPASS");
    lua_pushinteger(L, KLB_SOCKS5_METHOD_NO_ACCEPT);
    lua_setfield(L, -2, "METHOD_NO_ACCEPT");
    lua_pushinteger(L, KLB_SOCKS5_CMD_CONNECT);
    lua_setfield(L, -2, "CMD_CONNECT");
    lua_pushinteger(L, KLB_SOCKS5_REP_CMD_NOT_SUPP);
    lua_setfield(L, -2, "REP_CMD_NOT_SUPP");
    lua_pushinteger(L, KLB_SOCKS5_REP_ATYP_NOT_SUPP);
    lua_setfield(L, -2, "REP_ATYP_NOT_SUPP");

    return 1;
}

// end
