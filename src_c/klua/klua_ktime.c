#include "klua/klua.h"
#include "time/klb_time.h"


static int klua_ktime_sleep(lua_State* L)
{
    return 0;
}

static int klua_ktime_sleep_ns(lua_State* L)
{
    return 0;
}

static int klua_ktime_tick_count(lua_State* L)
{
    uint64_t tc = klb_tick_count64();
    tc = tc & 0x7fffffffffffffff;

    lua_pushinteger(L, tc);     // #1 tick count
    return 1;
}

int klua_open_ktime(lua_State* L)
{
    static luaL_Reg ktime_lib[] =
    {
        { "sleep",      klua_ktime_sleep },
        { "sleep_ns",   klua_ktime_sleep_ns },

        { "tick_count", klua_ktime_tick_count },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, ktime_lib);

    return 1;
}
