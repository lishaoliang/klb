#include "klua/klua_util/klua_seri_map.h"
#include "klbutil/klb_map_serialize.h"
#include "klua/klua_seri.h"
#include "klbmem/klb_buffer.h"
#include "klbmem/klb_rbuf.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <assert.h>


#define MAX_DEPTH 32


//////////////////////////////////////////////////////////////////////////
// Lua => map

static klb_adt_t* luaseri_map_pack_one_object(lua_State *L, int idx, int depth);


static klb_map_t* luaseri_map_pack_table_array(lua_State* L, int idx, int depth, klb_map_t* p_array)
{
    int array_size = (int)lua_rawlen(L, idx);

    for (int i = 1; i <= array_size; i++)
    {
        lua_rawgeti(L, idx, i);

        klb_adt_t* p_adt = luaseri_map_pack_one_object(L, -1, depth);
        klb_map_append_adt(p_array, p_adt);

        lua_pop(L, 1);
    }

    return p_array;
}

static klb_map_t* luaseri_map_pack_table_hash(lua_State* L, int idx, int depth, int array_size, klb_map_t* p_map)
{
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        int t = lua_type(L, -2);
        if (LUA_TNUMBER == t)
        {
            if (lua_isinteger(L, -2))
            {
                lua_Integer x = lua_tointeger(L, -2);
                if (x > 0 && x <= array_size)
                {
                    lua_pop(L, 1);
                    continue;
                }
            }
        }
        else if (LUA_TSTRING == t)
        {
            const char* p_key = lua_tostring(L, -2);

            klb_adt_t* p_adt = luaseri_map_pack_one_object(L, -1, depth);
            klb_map_set_adt(p_map, p_key, p_adt);
        }

        lua_pop(L, 1);
    }

    return p_map;
}

static klb_map_t* luaseri_map_pack_table(lua_State* L, int idx, int depth)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    if (idx < 0)
    {
        idx = lua_gettop(L) + idx + 1;
    }

    klb_map_t* p_map = klb_map_create();

    int array_size = (int)lua_rawlen(L, idx);
    if (0 < array_size)
    {
        luaseri_map_pack_table_array(L, idx, depth, p_map);
    }

    luaseri_map_pack_table_hash(L, idx, depth, array_size, p_map);

    return p_map;
}

static klb_adt_t* luaseri_map_pack_one_object(lua_State* L, int idx, int depth)
{
    klb_adt_t* p_adt = klb_adt_create();

    if (depth > MAX_DEPTH)
    {
        return p_adt;
    }

    int type = lua_type(L, idx);
    switch (type)
    {
    case LUA_TNIL:
        klb_adt_set_null(p_adt);
        break;
    case LUA_TNUMBER:
        {
            if (lua_isinteger(L, idx)) 
            {
                lua_Integer x = lua_tointeger(L, idx);
                klb_adt_set_int64(p_adt, x);
            }
            else
            {
                lua_Number n = lua_tonumber(L, idx);
                klb_adt_set_double(p_adt, n);
            }
        }
        break;
    case LUA_TBOOLEAN:
        klb_adt_set_bool(p_adt, lua_toboolean(L, idx));
        break;
    case LUA_TSTRING:
        {
            size_t len = 0;
            const char* p_str = lua_tolstring(L, idx, &len);
            klb_adt_set_lstring(p_adt, p_str, (int)len);
        }
        break;
    case LUA_TTABLE:
        {
            if (idx < 0)
            {
                idx = lua_gettop(L) + idx + 1;
            }

            klb_map_t* p_sub = luaseri_map_pack_table(L, idx, depth + 1);
            klb_adt_set_map(p_adt, p_sub);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        {
            const void* ptr = lua_topointer(L, idx);
            klb_adt_set_ptr(p_adt, ptr, NULL);
        }
        break;
    case LUA_TUSERDATA:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
        // Note. 只许可基本类型序列化
        break;
    default:
        break;
    }

    return p_adt;
}

static klb_map_t* luaseri_map_pack_from(lua_State* L, int from)
{
    klb_map_t* p_map = klb_map_create();

    int n = lua_gettop(L) - from;
    for (int i = 1; i <= n; i++)
    {
        klb_adt_t* p_adt = luaseri_map_pack_one_object(L, from + i, 0);
        klb_map_append_adt(p_map, p_adt);
    }

    return p_map;
}

klb_map_t* luaseri_map_pack(lua_State* L, int base_idx)
{
    return luaseri_map_pack_from(L, base_idx/*0*/);
}

//////////////////////////////////////////////////////////////////////////
// map => Lua

static void luaseri_map_unpack_one(lua_State *L, klb_adt_t* p_adt);

static void luaseri_map_unpack_array(lua_State *L, klb_map_t* p_array)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    int array_size = klb_map_array_size(p_array);
    for (int i = 0; i < array_size; i++)
    {
        luaseri_map_unpack_one(L, (klb_adt_t*)klb_map_idx_to_adt(p_array, i));
        lua_rawseti(L, -2, i + 1);
    }
}

static void luaseri_map_unpack_object(lua_State *L, klb_map_t* p_object)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    klb_map_iter_t* p_iter = klb_map_begin(p_object);
    while (NULL != p_iter)
    {
        lua_pushstring(L, klb_map_key(p_iter));             // key
        luaseri_map_unpack_one(L, klb_map_data(p_iter));    // value
        lua_rawset(L, -3);

        p_iter = klb_map_next(p_iter);
    }
}

static void luaseri_map_unpack_one(lua_State *L, klb_adt_t* p_adt)
{
    switch (klb_adt_type(p_adt))
    {
    case KLB_ADT_null:
        lua_pushnil(L);
        break;
    case KLB_ADT_bool:
        lua_pushboolean(L, klb_adt_to_bool(p_adt));
        break;
    case KLB_ADT_string:
        {
            int len = 0;
            const char* p_str = klb_adt_to_lstring(p_adt, &len);
            lua_pushlstring(L, p_str, len);
        }     
        break;
    case KLB_ADT_double:
        lua_pushnumber(L, klb_adt_to_double(p_adt));
        break;
    case KLB_ADT_uint64:
        lua_pushinteger(L, klb_adt_to_uint64(p_adt));
        break;
    case KLB_ADT_int64:
        lua_pushinteger(L, klb_adt_to_int64(p_adt));
        break;
    case KLB_ADT_map: 
        {
            lua_newtable(L);

            klb_map_t* p_map = klb_adt_to_map(p_adt);

            if (0 < klb_map_array_size(p_map))
            {
                luaseri_map_unpack_array(L, p_map);
            }

            if(0 < klb_map_key_value_size(p_map))
            {
                luaseri_map_unpack_object(L, p_map);
            }
        }
        break;
    case KLB_ADT_ptr:
        lua_pushlightuserdata(L, (void*)klb_adt_to_ptr(p_adt, NULL));
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

int luaseri_map_unpack(lua_State* L, int base_idx, const klb_map_t* ptr)
{
    klb_map_t* p_map = (klb_map_t*)ptr;

    if (NULL == p_map)
    {
        return 0;
    }

    lua_settop(L, base_idx/*1*/);

    int array_size = klb_map_array_size(p_map);
    if (0 < array_size)
    {
        for (int i = 0; i < array_size; i++)
        {
            luaseri_map_unpack_one(L, (klb_adt_t*)klb_map_idx_to_adt(p_map, i));
        }
    }
    else
    {
        luaseri_map_unpack_object(L, p_map);
    }

    return lua_gettop(L) - base_idx/*1*/;
}

//////////////////////////////////////////////////////////////////////////
// Lua => map binary

#define KLUASERI_MAP_WRITE_RBUF(RBUF_, KEY_, ADT_TYPE_, PAYLOAD_, VALUE_) \
{ \
    klbmapseri_head_t h_ = { 0 }; \
    h_.key = (KEY_) & 0x1; \
    h_.adt_type = (ADT_TYPE_) & 0x7; \
    h_.payload = (PAYLOAD_) & 0xF; \
    klb_rbuf_write(p_rbuf, &h_, sizeof(h_)); \
    klb_rbuf_write(p_rbuf, &(VALUE_), sizeof(VALUE_)); \
}

#define KLUASERI_MAP_WRITE_RBUF_HEAD(RBUF_, KEY_, ADT_TYPE_) \
{ \
    klbmapseri_head_t h_ = { 0 }; \
    h_.key = (KEY_) & 0x1; \
    h_.adt_type = (ADT_TYPE_) & 0x7; \
    h_.payload = 0; \
    klb_rbuf_write(p_rbuf, &h_, sizeof(h_)); \
}

static void luaseri_map_binary_write_one_object(lua_State* L, int idx, klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf);


static void luaseri_map_binary_write_int64(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, int64_t v)
{
    // 格式: klbmapseri_head_t + [数值]

    klb_rbuf_reset(p_rbuf);

    int64_t abs_v = ABS(v);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)v;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_int64, sizeof(int8_t), i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)v;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_int64, sizeof(int16_t), i16);
    }
    else if (abs_v < 0x80000000)
    {
        int32_t i32 = (int32_t)v;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_int64, sizeof(int32_t), i32);
    }
    else
    {
        int64_t i64 = (int64_t)v;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_int64, sizeof(int64_t), i64);
    }

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}


static void luaseri_map_binary_write_string(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, const char* p_str, int32_t str_len)
{
    klb_rbuf_reset(p_rbuf);

    // step1. 写 klbmapseri_head_t 头 + 字符串size
    int32_t abs_v = ABS(str_len);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)str_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_1B, i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)str_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_2B, i16);
    }
    else
    {
        int32_t i32 = (int32_t)str_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_string, KLBMAPSERI_payload_size_4B, i32);
    }

    // step2. 写字符串
    klb_rbuf_write(p_rbuf, p_str, str_len);

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void luaseri_map_binary_write_key(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, const char* p_key, int key_len)
{
    klb_rbuf_reset(p_rbuf);

    // step1. 写 klbmapseri_head_t 头 + 字符串size
    int32_t abs_v = ABS(key_len);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)key_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_1B, i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)key_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_2B, i16);
    }
    else
    {
        int32_t i32 = (int32_t)key_len;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_string, KLBMAPSERI_payload_size_4B, i32);
    }

    // step2. 写字符串
    klb_rbuf_write(p_rbuf, p_key, key_len);

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void luaseri_map_binary_write_idx(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf, int idx)
{
    klb_rbuf_reset(p_rbuf);

    int32_t abs_v = ABS(idx);
    if (abs_v < 0x80)
    {
        int8_t i8 = (int8_t)idx;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_int64, sizeof(int8_t), i8);
    }
    else if (abs_v < 0x8000)
    {
        int16_t i16 = (int16_t)idx;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_int64, sizeof(int16_t), i16);
    }
    else
    {
        int32_t i32 = (int32_t)idx;
        KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_key, KLB_ADT_int64, sizeof(int32_t), i32);
    }

    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void luaseri_map_binary_write_end(klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    klb_rbuf_reset(p_rbuf);

    KLUASERI_MAP_WRITE_RBUF_HEAD(p_rbuf, KLBMAPSERI_key, KLB_ADT_null);
    klb_buffer_write_rbuf(p_buffer, p_rbuf);
}

static void luaseri_map_binary_write_table_array(lua_State* L, int idx, klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    int array_size = (int)lua_rawlen(L, idx);

    for (int i = 1; i <= array_size; i++)
    {
        lua_rawgeti(L, idx, i);

        luaseri_map_binary_write_idx(p_buffer, p_rbuf, i - 1);
        luaseri_map_binary_write_one_object(L, -1, p_buffer, p_rbuf);

        lua_pop(L, 1);
    }
}

static void luaseri_map_binary_write_table_hash(lua_State* L, int idx, int array_size, klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        int t = lua_type(L, -2);
        if (LUA_TNUMBER == t)
        {
            if (lua_isinteger(L, -2))
            {
                lua_Integer x = lua_tointeger(L, -2);
                if (x > 0 && x <= array_size)
                {
                    lua_pop(L, 1);
                    continue;
                }
            }
        }
        else if (LUA_TSTRING == t)
        {
            size_t key_len = 0;
            const char* p_key = lua_tolstring(L, -2, &key_len);

            luaseri_map_binary_write_key(p_buffer, p_rbuf, p_key, (int)key_len);
            luaseri_map_binary_write_one_object(L, -1, p_buffer, p_rbuf);
        }

        lua_pop(L, 1);
    }
}

static void luaseri_map_binary_write_table(lua_State* L, int idx, klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    luaL_checkstack(L, LUA_MINSTACK, NULL);

    if (idx < 0)
    {
        idx = lua_gettop(L) + idx + 1;
    }

    int array_size = (int)lua_rawlen(L, idx);
    if (0 < array_size)
    {
        luaseri_map_binary_write_table_array(L, idx, p_buffer, p_rbuf);
    }

    luaseri_map_binary_write_table_hash(L, idx, array_size, p_buffer, p_rbuf);

    luaseri_map_binary_write_end(p_buffer, p_rbuf);
}

static void luaseri_map_binary_write_one_object(lua_State* L, int idx, klb_buffer_t* p_buffer, klb_rbuf_t* p_rbuf)
{
    klb_rbuf_reset(p_rbuf);

    int type = lua_type(L, idx);
    switch (type)
    {
    case LUA_TNIL:
        {
            KLUASERI_MAP_WRITE_RBUF_HEAD(p_rbuf, KLBMAPSERI_value, KLB_ADT_null);
            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case LUA_TNUMBER:
        {
            if (lua_isinteger(L, idx))
            {
                int64_t i64 = (int64_t)lua_tointeger(L, idx);
                luaseri_map_binary_write_int64(p_buffer, p_rbuf, i64);
            }
            else
            {
                double d = (double)lua_tonumber(L, idx);
                KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_double, sizeof(double), d);
                klb_buffer_write_rbuf(p_buffer, p_rbuf);
            }            
        }
        break;
    case LUA_TBOOLEAN:
        {
            int8_t b = (int8_t)lua_toboolean(L, idx);
            KLUASERI_MAP_WRITE_RBUF(p_rbuf, KLBMAPSERI_value, KLB_ADT_bool, sizeof(int8_t), b);
            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    case LUA_TSTRING:
        {
            size_t len = 0;
            const char* p_str = lua_tolstring(L, idx, &len);
            luaseri_map_binary_write_string(p_buffer, p_rbuf, p_str, (int32_t)len);
        }
        break;
    case LUA_TTABLE:
        {
            if (idx < 0)
            {
                idx = lua_gettop(L) + idx + 1;
            }

            KLUASERI_MAP_WRITE_RBUF_HEAD(p_rbuf, KLBMAPSERI_value, KLB_ADT_map);
            klb_buffer_write_rbuf(p_buffer, p_rbuf);

            luaseri_map_binary_write_table(L, idx, p_buffer, p_rbuf);
        }
        break;
    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
        {
            KLUASERI_MAP_WRITE_RBUF_HEAD(p_rbuf, KLBMAPSERI_value, KLB_ADT_null);
            klb_buffer_write_rbuf(p_buffer, p_rbuf);
        }
        break;
    }
}

static klb_buf_t* luaseri_map_binary_write_from(lua_State* L, int from)
{
    klb_buffer_t* p_buffer = klb_buffer_create(0);
    klb_rbuf_t rbuf = { 0 };

    int n = lua_gettop(L) - from;
    for (int i = 1; i <= n; i++)
    {
        luaseri_map_binary_write_idx(p_buffer, &rbuf, i - 1);
        luaseri_map_binary_write_one_object(L, from + i, p_buffer, &rbuf);
    }

    luaseri_map_binary_write_end(p_buffer, &rbuf);
    klb_rbuf_quit(&rbuf);

    klb_buf_t* p_buf = klb_buffer_join(p_buffer, NULL, NULL);
    klb_buffer_destroy(p_buffer);
    return p_buf;
}

klb_buf_t* luaseri_map_binary_pack(lua_State* L, int base_idx)
{
    return luaseri_map_binary_write_from(L, base_idx/*0*/);
}

//////////////////////////////////////////////////////////////////////////
// Lua => map binary

static int luaseri_map_binary_read_value(lua_State* L, klbmapseri_reader_t* p_reader, klbmapseri_head_t* p_h);

static int luaseri_map_binary_read_map(lua_State* L, klbmapseri_reader_t* p_reader)
{
    while (true)
    {
        klbmapseri_head_t h = { 0 };

        if (!klb_map_seri_read_head(p_reader, &h)) { break; }

        if (KLB_ADT_int64 == h.adt_type)
        {
            int idx = 0;
            klb_map_seri_read_idx(p_reader, &idx); // array

            klbmapseri_head_t h_v = { 0 };
            if (!klb_map_seri_read_head(p_reader, &h_v)) { break; }

            luaseri_map_binary_read_value(L, p_reader, &h_v);   // value
            lua_rawseti(L, -2, idx + 1);                        // index:V
        }
        else if(KLB_ADT_string == h.adt_type)
        {
            char* p_key = NULL;
            int key_len = 0;
            klb_map_seri_read_key(p_reader, &p_key, &key_len); // key/value

            klbmapseri_head_t h_v = { 0 };
            if (!klb_map_seri_read_head(p_reader, &h_v)) { break; }

            lua_pushlstring(L, p_key, key_len);                 // key
            luaseri_map_binary_read_value(L, p_reader, &h_v);   // value
            lua_rawset(L, -3);                                  // K:V
        }
        else
        {
            break; // KLB_ADT_null
        }
    }

    return 0;
}

static int luaseri_map_binary_read_value(lua_State* L, klbmapseri_reader_t* p_reader, klbmapseri_head_t* p_h)
{
    switch (p_h->adt_type)
    {
    case KLB_ADT_null:
        {
            lua_pushnil(L);
        }
        break;
    case KLB_ADT_bool:
        {
            bool b = false;
            klb_map_seri_read_bool(p_reader, &b);
            lua_pushboolean(L, b);
        }
        break;
    case KLB_ADT_string:
        {
            char* p_str = NULL;
            int str_len = 0;
            klb_map_seri_read_string(p_reader, &p_str, &str_len);
            lua_pushlstring(L, p_str, str_len);
        }
        break;
    case KLB_ADT_double:
        {
            double d = 0.0;
            klb_map_seri_read_double(p_reader, &d);
            lua_pushnumber(L, d);
        }
        break;
    case KLB_ADT_uint64:
        {
            uint64_t u64 = 0;
            klb_map_seri_read_uint64(p_reader, &u64);
            lua_pushinteger(L, u64);
        }
        break;
    case KLB_ADT_int64:
        {
            int64_t i64 = 0;
            klb_map_seri_read_int64(p_reader, &i64);
            lua_pushinteger(L, i64);
        }
        break;
    case KLB_ADT_map:
        {
            lua_newtable(L);
            luaseri_map_binary_read_map(L, p_reader);
        }
        break;
    default:
        {
            lua_pushnil(L);
        }
        break;
    }

    return 0;
}

static int luaseri_map_binary_read(lua_State* L, klbmapseri_reader_t* p_reader)
{
    while (true)
    {
        int idx = 0;
        klbmapseri_head_t h = { 0 };

        if (!klb_map_seri_read_head(p_reader, &h)) { break; }

        if (KLB_ADT_int64 == h.adt_type)
        {
            klb_map_seri_read_idx(p_reader, &idx);
        }
        else
        {
            break; // KLB_ADT_null
        }

        klbmapseri_head_t h_v = { 0 };

        if (!klb_map_seri_read_head(p_reader, &h_v)) { break; }

        luaseri_map_binary_read_value(L, p_reader, &h_v);
    }

    return 0;
}

int luaseri_map_binary_unpack(lua_State* L, int base_idx, const char* p_data, int data_len)
{
    klbmapseri_reader_t reader = { 0 };
    klb_map_seri_read_init(&reader, p_data, data_len);

    lua_settop(L, base_idx/*1*/);

    luaseri_map_binary_read(L, &reader);

    return lua_gettop(L) - base_idx/*1*/;
}

//////////////////////////////////////////////////////////////////////////
// 导出

klb_map_t* klua_seri_map_pack(lua_State* L, int base_idx)
{
    return luaseri_map_pack(L, base_idx);
}

int klua_seri_map_unpack(lua_State* L, int base_idx, const klb_map_t* p_map)
{
    return luaseri_map_unpack(L, base_idx, p_map);
}

klb_buf_t* klua_seri_map_binary_pack(lua_State* L, int base_idx)
{
    return luaseri_map_binary_pack(L, base_idx);
}

int klua_seri_map_binary_unpack(lua_State* L, int base_idx, const char* p_data, int data_len)
{
    return luaseri_map_binary_unpack(L, base_idx, p_data, data_len);
}
