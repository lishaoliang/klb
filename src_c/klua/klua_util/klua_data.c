#include "klua/klua_data.h"
#include "klbmem/klb_mem.h"
#include <assert.h>

void klua_emptydata(klua_data_t* p_data)
{
    assert(NULL != p_data);
    if (KLUA_DATA_TSTRING == p_data->type)
    {
        KLB_FREE_BY(p_data->str, sdsfree);
    }

    memset(p_data, 0, sizeof(klua_data_t));
}

int klua_checkdata(lua_State* L, klua_data_t* p_data, int idx)
{
    klua_emptydata(p_data);

    int t = lua_type(L, idx);
    switch (t)
    {
    case LUA_TBOOLEAN:
        {
            p_data->integer = lua_toboolean(L, idx);
            p_data->type = KLUA_DATA_TBOOLEAN;
        }
        break;
    case LUA_TLIGHTUSERDATA:
        {
            p_data->ptr = lua_touserdata(L, idx);
            p_data->type = KLUA_DATA_TLIGHTUSERDATA;
        }
        break;
    case LUA_TNUMBER:
        {
            if (lua_isinteger(L, idx))
            {
                p_data->integer = lua_tointeger(L, idx);
                p_data->type = KLUA_DATA_TINTEGER;
            }
            else
            {
                p_data->number = lua_tonumber(L, idx);
                p_data->type = KLUA_DATA_TNUMBER;
            }
        }
        break;
    case LUA_TSTRING:
        {
            size_t str_len = 0;
            const char* p_str = luaL_checklstring(L, idx, &str_len);

            p_data->str = sdsnewlen(p_str, str_len);
            p_data->type = KLUA_DATA_TSTRING;
        }
        break;
    default:
        {
            //无处理
        }
        break;
    }

    return p_data->type;
}

void klua_pushdata(lua_State* L, const klua_data_t* p_data)
{
    switch (p_data->type)
    {
    case KLUA_DATA_TBOOLEAN: ///< boolean 布尔
        {
            lua_pushboolean(L, (int)p_data->integer);
        }
        break;
    case KLUA_DATA_TLIGHTUSERDATA: ///< void* 指针
        {
            lua_pushlightuserdata(L, p_data->ptr);
        }
        break;
    case KLUA_DATA_TNUMBER: ///< lua_Number 浮点类型
        {
            lua_pushnumber(L, p_data->number);
        }
        break;
    case KLUA_DATA_TSTRING: ///< char* 字符串
        {
            lua_pushlstring(L, p_data->str, sdslen(p_data->str));
        }
        break;
    case KLUA_DATA_TINTEGER: ///< lua_Integer 数值型
        {
            lua_pushinteger(L, p_data->integer);
        }
        break;
    default:
        {
            lua_pushnil(L);
        }
        break;
    }
}

void klua_ctrlex_msg_destroy(klua_msg_t* p_msg)
{
    klua_ctrlex_msg_t* p_ctrlex_msg = (klua_ctrlex_msg_t*)p_msg;

    KLB_FREE_BY(p_ctrlex_msg->ex_name, sdsfree);

    for (int i = 0; i < p_ctrlex_msg->data_num; i++)
    {
        klua_emptydata(&p_ctrlex_msg->data[i]);
    }

    KLB_FREE(p_ctrlex_msg);
}

void klua_lpc_msg_destroy(klua_msg_t* p_msg)
{
    klua_lpc_msg_t* p_lpc_msg = (klua_lpc_msg_t*)p_msg;

    KLB_FREE_BY(p_lpc_msg->ex_name, sdsfree);
    KLB_FREE_BY(p_lpc_msg->src_name, sdsfree);
    KLB_FREE_BY(p_lpc_msg->func_name, sdsfree);
    KLB_FREE_BY(p_lpc_msg->sequence, sdsfree);

    for (int i = 0; i < p_lpc_msg->data_num; i++)
    {
        klua_emptydata(&p_lpc_msg->data[i]);
    }

    KLB_FREE(p_lpc_msg);
}
