// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbplatform/klb_time.h"
#include "klbplatform/klb_thread.h"
#include "klua/extension/klua_ex_time.h"


static int klua_ktime_sleep(lua_State* L)
{
    uint32_t ms = (uint32_t)luaL_checkinteger(L, 1);       // @1 休眠时间(毫秒)

    klb_sleep(ms);

    // Fixed Bug. [2025] 休眠后, 主env中依然保持休眠前的时间, 显然不合理
    // 更新计时器
    klua_env_update_tick_count(klua_env_get_by_L(L));

    return 0;
}

static int klua_ktime_sleep_ns(lua_State* L)
{
    return 0;
}

static int klua_ktime_tick_count(lua_State* L)
{
    int64_t tc = klb_tick_counti64();

    lua_pushinteger(L, tc);     // #1 tick count
    return 1;
}

static int klua_ktime_timer(lua_State* L)
{
    lua_Integer wait = luaL_checkinteger(L, 1);         // @1 等待间隔(毫秒)
    luaL_checktype(L, 2, LUA_TFUNCTION);                // @2 回调函数

    klua_env_t* p_env = klua_env_get_by_L(L);
    if (NULL == p_env)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    klua_ex_time_t* p_time = klua_ex_get_time(p_env);
    if (0 == klua_ex_time_new_timer_once(p_time, L, wait, 2))
    {
        lua_pushboolean(L, true);                       // #1 成功
    }
    else
    {
        lua_pushboolean(L, false);
    }

    return 1;
}

static int klua_ktime_ticker(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);    // @1 名称
    lua_Integer interval = luaL_checkinteger(L, 2);             // @2 间隔(毫秒)
    luaL_checktype(L, 3, LUA_TFUNCTION);                        // @3 回调函数

    klua_env_t* p_env = klua_env_get_by_L(L);
    if (NULL == p_env)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    klua_ex_time_t* p_time = klua_ex_get_time(p_env);
    if (0 == klua_ex_time_new_ticker(p_time, L, p_name, name_len, interval, 3))
    {
        lua_pushboolean(L, true);                               // #1 成功
    }
    else
    {
        lua_pushboolean(L, false);
    }

    return 1;
}


static int klua_ktime_stop_ticker(lua_State* L)
{
    size_t name_len = 0;
    const char* p_name = luaL_checklstring(L, 1, &name_len);        // @1 名称

    klua_env_t* p_env = klua_env_get_by_L(L);
    if (NULL == p_env)
    {
        lua_pushboolean(L, false);
        return 1;
    }

    klua_ex_time_t* p_time = klua_ex_get_time(p_env);
    if (0 == klua_ex_time_stop_ticker(p_time, p_name, name_len))
    {
        lua_pushboolean(L, true);                                   // #1 成功
    }
    else
    {
        lua_pushboolean(L, false);
    }

    return 1;
}


int klua_open_ktime(lua_State* L)
{
    static luaL_Reg ktime_lib[] =
    {
        { "sleep",          klua_ktime_sleep },
        { "sleep_ns",       klua_ktime_sleep_ns },

        { "tick_count",     klua_ktime_tick_count },    // 系统滴答数(从开机起的毫秒ms)

        { "timer",          klua_ktime_timer },         // 一次性, 定时器(timer)

        { "ticker",         klua_ktime_ticker },        // 定期执行的定时器(ticker), 直到主动关闭
        { "stop_ticker",    klua_ktime_stop_ticker},    // 关闭(ticker)定时器

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, ktime_lib);

    return 1;
}
