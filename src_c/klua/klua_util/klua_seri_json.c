#include "klua/klua_util/klua_seri_json.h"
#include "klua/klua_seri.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>


#define MAX_DEPTH 32


///////////////////////////////////////

static cJSON* luaseri_json_pack_one_object(lua_State *L, int index, int depth);

static cJSON* luaseri_json_pack_table_array(lua_State *L, int index, int depth) 
{
    int array_size = (int)lua_rawlen(L,index);

    cJSON* p_array = cJSON_CreateArray();

    for (int i = 1; i <= array_size; i++) 
    {
        lua_rawgeti(L,index,i);

        cJSON* p_item = luaseri_json_pack_one_object(L, -1, depth);
        cJSON_AddItemToArray(p_array, p_item);

        lua_pop(L,1);
    }

    return p_array;
}

static cJSON* luaseri_json_pack_table_hash(lua_State *L, int index, int depth, int array_size) 
{
    cJSON* p_obj = cJSON_CreateObject();

    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
        int t = lua_type(L, -2);
        if (LUA_TNUMBER == t)
        {
            if (lua_isinteger(L, -2))
            {
                lua_Integer x = lua_tointeger(L,-2);
                if (x > 0 && x <= array_size)
                {
                    lua_pop(L,1);
                    continue;
                }
            }
        }
        else if (LUA_TSTRING == t)
        {
            const char* p_key = lua_tostring(L, -2);

#if 1
            cJSON* p_v = luaseri_json_pack_one_object(L, -1, depth);
            cJSON_AddItemToObject(p_obj, p_key, p_v);
#else
            if (0 != strcmp(p_key, "_G") &&
                0 != strcmp(p_key, "package"))
            {
                cJSON* p_v = luaseri_json_pack_one_object(L, -1, depth);
                cJSON_AddItemToObject(p_obj, p_key, p_v);
            }
#endif
        }

        lua_pop(L, 1);
    }

    return p_obj;
}

static cJSON* luaseri_json_pack_table(lua_State *L, int index, int depth) 
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    if (index < 0) 
    {
        index = lua_gettop(L) + index + 1;
    }

    int array_size = (int)lua_rawlen(L, index);
    if (0 < array_size)
    {
        return luaseri_json_pack_table_array(L, index, depth);
    }
    else
    {
        return luaseri_json_pack_table_hash(L, index, depth, array_size);
    }
}

static cJSON* luaseri_json_pack_one_object(lua_State *L, int index, int depth)
{
    if (depth > MAX_DEPTH)
    {
        return cJSON_CreateString("unsupport:MAX_DEPTH");
    }

    cJSON* p_obj = NULL;
    int type = lua_type(L, index);
    switch (type) 
    {
    case LUA_TNIL:
        p_obj = cJSON_CreateNull(); // nil => false
        break;
    case LUA_TNUMBER: 
        {
            if (lua_isinteger(L, index)) {
                lua_Integer x = lua_tointeger(L, index);
                p_obj = cJSON_CreateInt(x, -1);
            }
            else
            {
                lua_Number n = lua_tonumber(L, index);
                p_obj = cJSON_CreateDouble(n, -1);
            }
            break;
        }
    case LUA_TBOOLEAN:
        p_obj = cJSON_CreateBool(lua_toboolean(L, index));
        break;
    case LUA_TSTRING: 
        {
            size_t sz = 0;
            const char *str = lua_tolstring(L, index, &sz);
            p_obj = cJSON_CreateString(str);
        }
        break;
    case LUA_TTABLE:
        {
            if (index < 0)
            {
                index = lua_gettop(L) + index + 1;
            }
            p_obj = luaseri_json_pack_table(L, index, depth + 1);
        }
        break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
        {
            char str[64] = { 0 };
            snprintf(str, sizeof(str), "userdata:%p", lua_topointer(L, index));
            str[sizeof(str) - 1] = '\0';
            p_obj = cJSON_CreateString(str);    // 不支持, 直接转化为 string
        }
        break;
    case LUA_TFUNCTION:
        {
            char str[64] = { 0 };
            snprintf(str, sizeof(str), "function:%p", lua_topointer(L, index));
            str[sizeof(str) - 1] = '\0';
            p_obj = cJSON_CreateString(str);    // 不支持, 直接转化为 string
        }
        break;
    case LUA_TTHREAD:
        {
            char str[64] = { 0 };
            snprintf(str, sizeof(str), "thread:%p", lua_topointer(L, index));
            str[sizeof(str) - 1] = '\0';
            p_obj = cJSON_CreateString(str);    // 不支持, 直接转化为 string
        }
        break;
    default:
        {
            char str[64] = { 0 };
            snprintf(str, sizeof(str), "unknown:%p", lua_topointer(L, index));
            str[sizeof(str) - 1] = '\0';
            p_obj = cJSON_CreateString(str);    // 不支持, 直接转化为 string
        }
        break;
    }

    return p_obj;
}

static cJSON* luaseri_json_pack_from(lua_State *L, int from)
{
    cJSON* p_array = cJSON_CreateArray();

    int n = lua_gettop(L) - from;
    for (int i = 1; i <= n; i++)
    {
        cJSON* p_item = luaseri_json_pack_one_object(L, from + i, 0);
        cJSON_AddItemToArray(p_array, p_item);
    }

    return p_array;
}

///////////////////////////////////
static void luaseri_json_unpack_one(lua_State *L, cJSON* p_item);

static void luaseri_json_unpack_array(lua_State *L, cJSON* p_array)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    int array_size = cJSON_GetArraySize(p_array);
    lua_createtable(L, array_size, 0);
    for (int i = 0; i < array_size; i++)
    {
        cJSON* p_item = cJSON_GetArrayItem(p_array, i);
        luaseri_json_unpack_one(L, p_item);
        lua_rawseti(L, -2, i + 1);
    }
}

static void luaseri_json_unpack_object(lua_State *L, cJSON* p_object)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);
    lua_newtable(L);

    cJSON* p_next = p_object->child;
    while (NULL != p_next)
    {
        lua_pushstring(L, p_next->string);  // key
        luaseri_json_unpack_one(L, p_next);              // value
        lua_rawset(L, -3);

        p_next = p_next->next;
    }
}

static void luaseri_json_unpack_one(lua_State *L, cJSON* p_item)
{
    switch (p_item->type)
    {
    case cJSON_False:
        lua_pushboolean(L, false);
        break;
    case cJSON_True:
        lua_pushboolean(L, true);
        break;
    case cJSON_NULL:
        lua_pushnil(L);
        break;
    case cJSON_Int:
        lua_pushinteger(L, p_item->valueint);
        break;
    case cJSON_Double:
        lua_pushnumber(L, p_item->valuedouble);
        break;
    case cJSON_String:
        lua_pushstring(L, p_item->valuestring);
        break;
    case cJSON_Array:
        luaseri_json_unpack_array(L, p_item);
        break;
    case cJSON_Object:
        luaseri_json_unpack_object(L, p_item);
        break; 
    default:
        break;
    }
}

//////////////////////////////////

cJSON* luaseri_json_pack(lua_State *L, int base_idx)
{
    cJSON* p_json = luaseri_json_pack_from(L, base_idx/*0*/);
    //char* p_str = cJSON_PrintUnformatted(p_json);

    return p_json;
}

int luaseri_json_unpack(lua_State *L, int base_idx, const cJSON* ptr)
{
    cJSON* p_json = (cJSON*)ptr;

    if (NULL == p_json)
    {
        return 0;
    }

    lua_settop(L, base_idx/*1*/);

    if (cJSON_Array == p_json->type)
    {
        int n = cJSON_GetArraySize(p_json);

        for (int i = 0; i < n; i++)
        {
            cJSON* p_item = cJSON_GetArrayItem(p_json, i);
            luaseri_json_unpack_one(L, p_item);
        }
    }
    else
    {
        // 单个参数
        luaseri_json_unpack_one(L, p_json);
    }

    return lua_gettop(L) - base_idx/*1*/;
}

//////////////////////////////////////////////////////////////////////////
// 导出

cJSON* klua_seri_json_pack(lua_State* L, int base_idx)
{
    return luaseri_json_pack(L, base_idx);
}

int klua_seri_json_unpack(lua_State* L, int base_idx, const cJSON* p_json)
{
    return luaseri_json_unpack(L, base_idx, p_json);
}
