// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/CKlua.hpp"
#include "klbutil/klb_log.h"
#include <assert.h>

namespace klb {

    CKlua::CKlua()
    {

    }

    CKlua::~CKlua()
    {

    }

    static void cklua_parse_to_map(lua_State* L, int ud, CMap& map)
    {
        CMap& r = map;

        bool is_array = false;
        bool is_map = false;
        bool is_first = true;

        int absindex = lua_absindex(L, ud);
        lua_pushnil(L);
        while (lua_next(L, absindex) != 0)
        {
            int t1 = lua_type(L, -1);
            int t2 = lua_type(L, -2);

            if (is_first)
            {
                if (LUA_TNUMBER == t2)
                {
                    is_array = true;
                    r = CArray();
                }
                else if (LUA_TSTRING == t2)
                {
                    is_map = true;
                }

                is_first = false;
            }

            if (is_array && LUA_TNUMBER == t2)
            {
                int index = lua_tointeger(L, -2);

                switch (t1)
                {
                case LUA_TBOOLEAN:
                    r.Array().Append((0 == lua_toboolean(L, -1)) ? false : true);
                    break;
                case LUA_TLIGHTUSERDATA:
                    r.Array().Append(lua_topointer(L, -1));
                    break;
                case LUA_TNUMBER:
                {
                    int isnum = 0;
                    lua_Integer d = lua_tointegerx(L, -1, &isnum);
                    if (isnum)
                    {
                        r.Array().Append(int64_t(d));
                    }
                    else
                    {
                        r.Array().Append(lua_tonumber(L, -1));
                    }
                }
                break;
                case LUA_TSTRING:
                {
                    r.Array().Append(lua_tostring(L, -1));
                }
                break;
                case LUA_TTABLE:
                {
                    CMap sub;
                    cklua_parse_to_map(L, -1, sub);

                    if (sub.IsArray())
                    {
                        r.Array().Append(sub.Array());
                    }
                    else
                    {
                        r.Array().Append(sub);
                    }
                }
                break;
                case LUA_TFUNCTION:
                {
                    r.Array().Append((void*)lua_tocfunction(L, -1), NULL);
                }
                break;
                default:
                    break;
                }
            }
            else if (is_map && LUA_TSTRING == t2)
            {
                const char* p_key = lua_tostring(L, -2);

                switch (t1)
                {
                case LUA_TBOOLEAN:
                    r[p_key] = (0 == lua_toboolean(L, -1)) ? false : true;
                    break;
                case LUA_TLIGHTUSERDATA:
                    r[p_key] = lua_topointer(L, -1);
                    break;
                case LUA_TNUMBER:
                {
                    int isnum = 0;
                    lua_Integer d = lua_tointegerx(L, -1, &isnum);
                    if (isnum)
                    {
                        r[p_key] = (int64_t)d;
                    }
                    else
                    {
                        r[p_key] = lua_tonumber(L, -1);
                    }
                }
                break;
                case LUA_TSTRING:
                {
                    r[p_key] = lua_tostring(L, -1);
                }
                break;
                case LUA_TTABLE:
                {
                    CMap sub;
                    cklua_parse_to_map(L, -1, sub);

                    if (sub.IsArray())
                    {
                        r[p_key] = sub.Array();
                    }
                    else
                    {
                        r[p_key] = sub;
                    }
                }
                break;
                case LUA_TFUNCTION:
                {
                    r[p_key].Set((void*)lua_tocfunction(L, -1), NULL);
                }
                break;
                default:
                    break;
                }
            }

            lua_pop(L, 1);
        }
    }

    static void cklua_push_map_to_lua(lua_State* L, CMap& map)
    {
        CMap& r = map;

        lua_newtable(L);

        if (r.IsArray())
        {
            CArray& a1 = r.Array();

            for (int i = 0; i < a1.Size(); i++)
            {
                CMapObj& item = a1[i];

                switch (item.Type())
                {
                case CMAP_CArray:
                    {
                        CMap sub(item.GetArray());
                        cklua_push_map_to_lua(L, sub);
                        lua_rawseti(L, -2, i + 1);
                    }
                    break;
                case CMAP_CMap:
                    {
                        CMap sub = item.GetMap();
                        cklua_push_map_to_lua(L, sub);
                        lua_rawseti(L, -2, i + 1);
                    }
                    break;
                case CMAP_String:
                    lua_pushstring(L, item.GetString().c_str());
                    lua_rawseti(L, -2, i + 1);
                    break;
                case CMAP_Bool:
                    lua_pushboolean(L, item.GetBool());
                    lua_rawseti(L, -2, i + 1);
                    break;
                case CMAP_Double:
                    lua_pushnumber(L, item.GetDouble());
                    lua_rawseti(L, -2, i + 1);
                    break;
                case CMAP_Int64:
                    lua_pushinteger(L, item.GetInt64());
                    lua_rawseti(L, -2, i + 1);
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            CMapIter iter = r.Begin();
            while (!iter.IsEnd())
            {
                CMapObj& item = iter.Data();

                switch (item.Type())
                {
                case CMAP_CArray:
                    {
                        CMap sub(item.GetArray());
                        cklua_push_map_to_lua(L, sub);
                        lua_pushstring(L, item.GetKey().c_str());
                        lua_insert(L, -2);
                        lua_settable(L, -3);
                    }
                    break;
                case CMAP_CMap:
                    {
                        CMap sub = item.GetMap();
                        cklua_push_map_to_lua(L, sub);
                        lua_pushstring(L, item.GetKey().c_str());
                        lua_insert(L, -2);
                        lua_settable(L, -3);
                    }
                    break;
                case CMAP_String:
                    lua_pushstring(L, item.GetString().c_str());
                    lua_setfield(L, -2, item.GetKey().c_str());
                    break;
                case CMAP_Bool:
                    lua_pushboolean(L, item.GetBool());
                    lua_setfield(L, -2, item.GetKey().c_str());
                    break;
                case CMAP_Double:
                    lua_pushnumber(L, item.GetDouble());
                    lua_setfield(L, -2, item.GetKey().c_str());
                    break;
                case CMAP_Int64:
                    lua_pushinteger(L, item.GetInt64());
                    lua_setfield(L, -2, item.GetKey().c_str());
                    break;
                default:
                    break;
                }

                iter = iter.Next();
            }
        }
    }

    bool CKlua::ParseToMap(lua_State* L, int idx, CMap& map)
    {
        if (!lua_istable(L, idx))
        {
            return false;
        }

        cklua_parse_to_map(L, idx, map);

        //std::string s = map.Dump();
        //KLB_LOG("map dump", s.c_str());

        return true;
    }

    void CKlua::PushMapToLua(lua_State* L, CMap& map)
    {

#if 0
        lua_newtable(L);
        for (int i = 0; i < 10; i++)
        {
            //lua_pushinteger(L, i + 1);
            //lua_pushstring(L, "aaa");

            lua_newtable(L);
                klua_setfield_string(L, "a", "a");
                klua_setfield_string(L, "b", "b");

                lua_newtable(L);
                    klua_setfield_string(L, "c", "c");

                lua_pushstring(L, "d");
                lua_insert(L, -2);
                lua_settable(L, -3);

            lua_rawseti(L, -2, i + 1);
        }
#endif

        cklua_push_map_to_lua(L, map);
    }
}
