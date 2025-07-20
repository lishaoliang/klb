// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbmem/klb_mem.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klua/klua_util/klua_seri_json.h"
#include "klbthird/cJSON.h"
#include "klbbase/klb_base.h"
#include <stdlib.h>
#include <assert.h>


// KLB库版本
static int klua_ksys_version(lua_State* L)
{
    int ver = klb_version_number();
    int date = klb_version_date();

    lua_pushstring(L, klb_version());
    lua_pushinteger(L, ver);
    lua_pushinteger(L, date);
    return 3;
}


static int klua_ksys_exit(lua_State* L)
{
    klua_env_exit(klua_env_get_by_L(L));
    return 0;
}

static int klua_ksys_is_exit(lua_State* L)
{
    bool exit = klua_env_is_exit(klua_env_get_by_L(L));
    lua_pushboolean(L, exit);
    return 1;
}

static int klua_ksys_pack_string(lua_State* L)
{
    klb_buf_t* p_buf = luaseri_map_binary_pack(L, 0);

    lua_pushlstring(L, p_buf->p_buf + p_buf->start, p_buf->end - p_buf->start);

    KLB_FREE(p_buf);
    return 1;
}

static int klua_ksys_unpack(lua_State* L)
{
    size_t str_len = 0;
    const char* p_str = luaL_checklstring(L, 1, &str_len);

    int n = luaseri_map_binary_unpack(L, 1, p_str, str_len);

    return n;
}

static int klua_ksys_pack_json(lua_State* L)
{
    cJSON* p_json = luaseri_json_pack(L, 0);
    char* p_str = cJSON_PrintUnformatted(p_json);

    lua_pushstring(L, p_str);
    
    KLB_FREE(p_str);
    KLB_FREE_BY(p_json, cJSON_Delete);
    return 1;
}

static int klua_ksys_unpack_json(lua_State* L)
{
    const char* p_str = luaL_checkstring(L, 1);

    const char* p_ep = NULL;
    cJSON* p_json = cJSON_Parse(p_str, &p_ep);

    int n = luaseri_json_unpack(L, 1, p_json);

    KLB_FREE_BY(p_json, cJSON_Delete);
    return n;
}

/// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
static int klua_ksys_get_args(lua_State* L)
{
    const klb_buf_t* p_arg = klua_env_get_args(klua_env_get_by_L(L));

    if (NULL != p_arg)
    {
        int n = luaseri_map_binary_unpack(L, 1, (char*)p_arg->p_buf + p_arg->start, p_arg->end - p_arg->start);
        return n;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_open_ksys(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "version",        klua_ksys_version },

        { "exit",           klua_ksys_exit },
        { "is_exit",        klua_ksys_is_exit },

        { "pack_string",    klua_ksys_pack_string },
        { "unpack",         klua_ksys_unpack },

        { "pack_json",      klua_ksys_pack_json },
        { "unpack_json",    klua_ksys_unpack_json },

        { "get_args",       klua_ksys_get_args },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}

//end
