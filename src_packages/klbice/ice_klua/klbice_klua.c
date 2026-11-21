// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbice_klua.c
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbice 的 Lua 绑定 (扩展库 "kice")
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.1 实现 agent 工厂与 userdata 方法面
///////////////////////////////////////////////////////////////////////////
#include "klbice/klbice.h"
#include "klbice/core/klb_ice.h"
#include "klbnet/klb_netcode.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klbmem/klb_mem.h"
#include <string.h>


/// @def   KLUA_KICE_HANDLE
/// @brief Lua userdata 元表名
#define KLUA_KICE_HANDLE                "KICE_AGENT_HANDLE*"

/// @def   KLUA_KICE_TEXT_MAX
/// @brief 凭据/候选文本缓冲长度
#define KLUA_KICE_TEXT_MAX              512

/// @def   KLUA_KICE_RECV_MAX
/// @brief 单次接收缓冲长度
#define KLUA_KICE_RECV_MAX              2048


/// @struct klua_kice_t
/// @brief  kice agent 的 Lua 包装
typedef struct klua_kice_t_
{
    lua_State*      L;          ///< Lua 状态
    klb_ice_agent_t* p_agent;   ///< ICE 代理
}klua_kice_t;


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_kice(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


/// @brief 统一返回: 小于 0 压入 nil, code; 否则压入 true
static int push_rc_klua_kice(lua_State* L, int code)
{
    if (0 > code)
    {
        return push_err_klua_kice(L, code);
    }

    lua_pushboolean(L, 1);
    return 1;
}


////////////////////////////////////////
static klua_kice_t* new_klua_kice(lua_State* L)
{
    klua_kice_t* p_kice = (klua_kice_t*)lua_newuserdata(L, sizeof(klua_kice_t));
    KLB_MEMSET(p_kice, 0, sizeof(klua_kice_t));
    luaL_setmetatable(L, KLUA_KICE_HANDLE);
    return p_kice;
}


static klua_kice_t* to_klua_kice(lua_State* L, int index)
{
    klua_kice_t* p_kice = (klua_kice_t*)luaL_checkudata(L, index, KLUA_KICE_HANDLE);
    luaL_argcheck(L, NULL != p_kice, index, "'kice' expected");
    return p_kice;
}


static int klua_kice_tostring(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);

    lua_pushfstring(L, "kice:%p", p_kice->p_agent);
    return 1;
}


static int klua_kice_close(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);

    if (NULL != p_kice->p_agent)
    {
        klb_ice_agent_destroy(p_kice->p_agent);
        p_kice->p_agent = NULL;
    }

    return 0;
}


////////////////////////////////////////
static int klua_kice_create(lua_State* L)
{
    int role = (int)luaL_optinteger(L, 1, KLB_ICE_ROLE_CONTROLLED);  ///< @1. 角色: 0.controlled; 1.controlling
    int code = 0;

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = (NULL != p_env) ? klua_netmulti_get(p_env) : NULL;

    if (NULL == p_netmulti)
    {
        return push_err_klua_kice(L, KLB_NETCODE_INVAL);
    }

    klb_ice_agent_t* p_agent = klb_ice_agent_create(p_netmulti, role, &code);
    if (NULL == p_agent)
    {
        return push_err_klua_kice(L, (0 != code) ? code : KLB_NETCODE_ERR);
    }

    klua_kice_t* p_kice = new_klua_kice(L);
    p_kice->L = L;
    p_kice->p_agent = p_agent;

    return 1;
}


static int klua_kice_local_ufrag(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    char buf[KLUA_KICE_TEXT_MAX];
    int ret = klb_ice_agent_local_ufrag(p_kice->p_agent, buf, (int)sizeof(buf));
    if (0 > ret)
    {
        return push_err_klua_kice(L, ret);
    }

    lua_pushlstring(L, buf, (size_t)ret);
    return 1;
}


static int klua_kice_local_pwd(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    char buf[KLUA_KICE_TEXT_MAX];
    int ret = klb_ice_agent_local_pwd(p_kice->p_agent, buf, (int)sizeof(buf));
    if (0 > ret)
    {
        return push_err_klua_kice(L, ret);
    }

    lua_pushlstring(L, buf, (size_t)ret);
    return 1;
}


static int klua_kice_local_count(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    int ret = klb_ice_agent_local_count(p_kice->p_agent);
    if (0 > ret)
    {
        return push_err_klua_kice(L, ret);
    }

    lua_pushinteger(L, ret);
    return 1;
}


static int klua_kice_local_candidate(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    int index = (int)luaL_checkinteger(L, 2);       ///< @2. 候选下标 (从 0 起)
    char buf[KLUA_KICE_TEXT_MAX];
    int ret = klb_ice_agent_local_candidate(p_kice->p_agent, index, buf, (int)sizeof(buf));
    if (0 > ret)
    {
        return push_err_klua_kice(L, ret);
    }

    lua_pushlstring(L, buf, (size_t)ret);
    return 1;
}


static int klua_kice_set_remote_credentials(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    const char* p_ufrag = luaL_checkstring(L, 2);   ///< @2. 远端 ufrag
    const char* p_pwd = luaL_checkstring(L, 3);     ///< @3. 远端 pwd

    return push_rc_klua_kice(L,
        klb_ice_agent_set_remote_credentials(p_kice->p_agent, p_ufrag, p_pwd));
}


static int klua_kice_add_remote_candidate(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    const char* p_candidate = luaL_checkstring(L, 2);   ///< @2. candidate 行

    return push_rc_klua_kice(L,
        klb_ice_agent_add_remote_candidate(p_kice->p_agent, p_candidate));
}


static int klua_kice_gather(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);

    return push_rc_klua_kice(L, klb_ice_agent_gather(p_kice->p_agent));
}


static int klua_kice_start(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);

    return push_rc_klua_kice(L, klb_ice_agent_start(p_kice->p_agent));
}


static int klua_kice_state(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    int ret = klb_ice_agent_get_state(p_kice->p_agent);
    if (0 > ret)
    {
        return push_err_klua_kice(L, ret);
    }

    lua_pushinteger(L, ret);
    return 1;
}


static int klua_kice_send(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    size_t len = 0;
    const char* p_data = luaL_checklstring(L, 2, &len);    ///< @2. 应用数据

    return push_rc_klua_kice(L,
        klb_ice_agent_send(p_kice->p_agent, (const uint8_t*)p_data, (int)len));
}


static int klua_kice_recv(lua_State* L)
{
    klua_kice_t* p_kice = to_klua_kice(L, 1);
    int cap = (int)luaL_optinteger(L, 2, KLUA_KICE_RECV_MAX);  ///< @2. 最大字节数
    int code = 0;
    uint8_t buf[KLUA_KICE_RECV_MAX];

    if ((0 >= cap) || (KLUA_KICE_RECV_MAX < cap))
    {
        cap = KLUA_KICE_RECV_MAX;
    }

    int ret = klb_ice_agent_recv(p_kice->p_agent, buf, cap, &code);
    if (0 > ret)
    {
        return push_err_klua_kice(L, (0 != code) ? code : ret);
    }

    if (0 == ret)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlstring(L, (const char*)buf, (size_t)ret);
    return 1;
}


////////////////////////////////////////
static void klua_kice_createmeta(lua_State* L)
{
    static const luaL_Reg meth[] =
    {
        { "close",                  klua_kice_close },
        { "destroy",                klua_kice_close },

        { "local_ufrag",            klua_kice_local_ufrag },
        { "local_pwd",              klua_kice_local_pwd },
        { "local_count",            klua_kice_local_count },
        { "local_candidate",        klua_kice_local_candidate },
        { "set_remote_credentials", klua_kice_set_remote_credentials },
        { "add_remote_candidate",   klua_kice_add_remote_candidate },

        { "gather",                 klua_kice_gather },
        { "start",                  klua_kice_start },
        { "state",                  klua_kice_state },
        { "send",                   klua_kice_send },
        { "recv",                   klua_kice_recv },

        { NULL,                     NULL }
    };

    static const luaL_Reg metameth[] =
    {
        { "__index",    NULL },     /* place holder */
        { "__gc",       klua_kice_close },
        { "__close",    klua_kice_close },
        { "__tostring", klua_kice_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KICE_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


int klua_open_kice(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "create",     klua_kice_create },
        { NULL,         NULL }
    };

    luaL_newlib(L, lib);
    klua_kice_createmeta(L);

    return 1;
}


// end
