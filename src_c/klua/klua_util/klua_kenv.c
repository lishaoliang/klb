// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_env.h"
#include "klua/klua_util/klua_seri_map.h"
#include <stdlib.h>
#include <assert.h>


/// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
static int klua_kenv_get_args(lua_State* L)
{
    const klb_buf_t* p_arg = klua_env_get_args(klua_env_get_by_L(L));

    if (NULL != p_arg)
    {
        int n = luaseri_map_binary_unpack(L, 1, (char*)p_arg->p_buf + p_arg->start, p_arg->end - p_arg->start);
        return n;
    }

    return 0;
}


// 获取 lua 环境名称
static int klua_kenv_get_name(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    const sds name = klua_env_get_name(p_env);

    if (NULL != name)
    {
        lua_pushstring(L, name);
    }
    else
    {
        lua_pushstring(L, "");
    } 
    return 1;
}


// 退出; 同 ksys.exit
static int klua_kenv_exit(lua_State* L)
{
    klua_env_exit(klua_env_get_by_L(L));
    return 0;
}


// 获取是否已退出; 同 ksys.is_exit
static int klua_kenv_is_exit(lua_State* L)
{
    bool exit = klua_env_is_exit(klua_env_get_by_L(L));
    lua_pushboolean(L, exit);
    return 1;
}


// 获取 当前 lua 环境的滴答数
//  与 当前系统滴答 不完全一致, 可能会存在小量偏差
static int klua_kenv_get_tick_count(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    int64_t tc = klua_env_get_tick_count(p_env);

    lua_pushinteger(L, tc);
    return 1;
}


// 立即更新 lua 环境的滴答数
static int klua_kenv_update_tick_count(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    klua_env_update_tick_count(p_env);

    return 0;
}


// 设置 lua 环境 loop 休眠时间(单位:毫秒)
static int klua_kenv_set_loop_sleep(lua_State* L)
{
    int sleep_time = (int)luaL_checkinteger(L, 1);

    klua_env_t* p_env = klua_env_get_by_L(L);
    klua_env_set_loop_sleep(p_env, sleep_time);

    return 0;
}


// 获取 lua 环境 loop 休眠时间(单位:毫秒)
static int klua_kenv_get_loop_sleep(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    int sleep_time = klua_env_get_loop_sleep(p_env);

    lua_pushinteger(L, sleep_time);
    return 1;
}


//////////////////////////////////////////////////////////////////////////


// lua 运行环境
int klua_open_kenv(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "get_args",               klua_kenv_get_args },           // 获取入口参数 args; 同 ksys.get_args
        { "get_name",               klua_kenv_get_name },           // 获取 lua 环境名称

        { "exit",                   klua_kenv_exit },               // 退出; 同 ksys.exit
        { "is_exit",                klua_kenv_is_exit },            // 获取是否已退出; 同 ksys.is_exit

        { "tick_count",             klua_kenv_get_tick_count },     // 获取 当前 lua 环境的滴答数
        { "update_tick_count",      klua_kenv_update_tick_count },  // 立即更新 lua 环境的滴答数

        { "set_loop_sleep",         klua_kenv_set_loop_sleep },     // 设置 loop 循环 休眠时间(单位:毫秒)
        { "get_loop_sleep",         klua_kenv_get_loop_sleep },     // 获取 loop 循环 休眠时间(单位:毫秒)

        { NULL,                     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}

//end
