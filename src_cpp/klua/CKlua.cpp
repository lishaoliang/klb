#include "klua/CKlua.hpp"


namespace klb {

    CKlua::CKlua()
    {

    }

    CKlua::~CKlua()
    {

    }

    bool CKlua::ParseToMap(lua_State* L, int idx, CMap& map)
    {
        if (!lua_istable(L, idx))
        {
            return false;
        }

        CMap& r = map;

        int absindex = lua_absindex(L, idx);
        lua_pushnil(L);

        while (lua_next(L, absindex) != 0)
        {
            int t2 = lua_type(L, -2);
            const char* p_key = lua_tostring(L, -2);

            int t1 = lua_type(L, -1);

            switch (t1)
            {
            case LUA_TBOOLEAN:
                {
                    r[p_key] = (0 == lua_toboolean(L, -1)) ? false : true;
                }
                break;
            case LUA_TNUMBER:
                {
                    int isnum = 0;
                    lua_Integer d = lua_tointegerx(L, -1, &isnum);
                    if (isnum)
                    {
                        r[p_key] = d;
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
            default:
                break;
            }

            lua_pop(L, 1);
        }

        return true;
    }
}
