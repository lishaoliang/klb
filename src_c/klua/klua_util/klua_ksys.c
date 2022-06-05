// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbmem/klb_mem.h"
#include "klua/lua-skynet/lua-seri.h"
#include <stdlib.h>
#include <assert.h>


static int klua_ksys_exit(lua_State* L)
{
    klua_env_exit(klua_env_get_by_L(L));
    return 0;
}

static int klua_ksys_pack_string(lua_State* L)
{
    luaseri_pack(L);

    char * str = (char *)lua_touserdata(L, -2);
    int sz = lua_tointeger(L, -1);
    lua_pushlstring(L, str, sz);
    KLB_FREE(str);
    return 1;
}

/// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
static int klua_ksys_get_arg(lua_State* L)
{
    const klb_buf_t* p_arg = klua_env_get_arg(klua_env_get_by_L(L));

    if (NULL != p_arg)
    {
        int n =luaseri_unpack_by_buffer(L, 1, (char*)p_arg->p_buf + p_arg->start, p_arg->end - p_arg->start);
        return n;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_ksys(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "exit",           klua_ksys_exit },

        { "pack_string",    klua_ksys_pack_string },
        { "unpack",         luaseri_unpack },

        { "get_arg",        klua_ksys_get_arg },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
