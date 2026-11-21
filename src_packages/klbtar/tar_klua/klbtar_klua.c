// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbtar/klbtar.h"
#include "klua/klua.h"


#define KLUA_KTAR_HANDLE    "KLUA_KTAR_HANDLE*"


/// @struct klua_ktar_t
/// @brief  tar Lua 包装 (桩)
typedef struct klua_ktar_t_
{
    klbtar_t*   p_tar;      ///< C 句柄
}klua_ktar_t;


//////////////////////////////////////////////////////////////////////////


static klua_ktar_t* new_klua_ktar(lua_State* L)
{
    klua_ktar_t* p_obj = (klua_ktar_t*)lua_newuserdata(L, sizeof(klua_ktar_t));

    p_obj->p_tar = NULL;
    luaL_setmetatable(L, KLUA_KTAR_HANDLE);

    return p_obj;
}


static klua_ktar_t* to_klua_ktar(lua_State* L, int index)
{
    klua_ktar_t* p_obj = (klua_ktar_t*)luaL_checkudata(L, index, KLUA_KTAR_HANDLE);

    luaL_argcheck(L, NULL != p_obj, index, "'ktar' expected");

    return p_obj;
}


static void close_klua_ktar(klua_ktar_t* p_obj)
{
    if (NULL == p_obj || NULL == p_obj->p_tar)
    {
        return;
    }

    klbtar_close(p_obj->p_tar);
    p_obj->p_tar = NULL;
}


//////////////////////////////////////////////////////////////////////////


static int klua_ktar_available(lua_State* L)
{
    lua_pushboolean(L, 1);
    return 1;
}


static int klua_ktar_probe(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    int fmt = KLBTAR_FMT_UNKNOWN;
    int rc = 0;

    rc = klbtar_probe(p_path, &fmt);

    if (0 == rc)
    {
        lua_pushinteger(L, fmt);
        lua_pushinteger(L, KLBTAR_OK);
        lua_pushstring(L, "");
        return 3;
    }

    lua_pushnil(L);
    lua_pushinteger(L, rc);
    lua_pushstring(L, "ktar probe stub");
    return 3;
}


static int klua_ktar_gc(lua_State* L)
{
    klua_ktar_t* p_obj = to_klua_ktar(L, 1);

    close_klua_ktar(p_obj);
    return 0;
}


static int klua_ktar_close(lua_State* L)
{
    klua_ktar_t* p_obj = to_klua_ktar(L, 1);

    close_klua_ktar(p_obj);
    return 0;
}


static int klua_ktar_tostring(lua_State* L)
{
    klua_ktar_t* p_obj = to_klua_ktar(L, 1);

    if (NULL != p_obj->p_tar)
    {
        lua_pushfstring(L, "ktar: %p", (void*)p_obj);
    }
    else
    {
        lua_pushstring(L, "ktar: closed");
    }

    return 1;
}


static int klua_ktar_open(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    int err = KLBTAR_OK;
    klbtar_t* p_tar = NULL;

    p_tar = klbtar_open_ex(p_path, &err);

    if (NULL == p_tar)
    {
        lua_pushnil(L);
        lua_pushinteger(L, err);
        lua_pushfstring(L, "ktar open failed: path=[%s]", p_path);
        return 3;
    }

    {
        klua_ktar_t* p_obj = new_klua_ktar(L);

        p_obj->p_tar = p_tar;
    }

    return 1;
}


static int klua_ktar_extract(lua_State* L)
{
    const char* p_path = luaL_checkstring(L, 1);
    const char* p_dest = luaL_checkstring(L, 2);
    int rc = 0;

    rc = klbtar_extract(p_path, p_dest);

    if (0 == rc)
    {
        lua_pushboolean(L, 1);
        lua_pushinteger(L, KLBTAR_OK);
        lua_pushstring(L, "");
        return 3;
    }

    lua_pushboolean(L, 0);
    lua_pushinteger(L, rc);
    lua_pushstring(L, "ktar extract stub");
    return 3;
}


static void klua_ktar_createmeta(lua_State* L)
{
    static luaL_Reg meta[] =
    {
        { "close",      klua_ktar_close },
        { "__gc",       klua_ktar_gc },
        { "__tostring", klua_ktar_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KTAR_HANDLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, meta, 0);
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////


int klua_open_ktar(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "available",  klua_ktar_available },
        { "probe",      klua_ktar_probe },
        { "open",       klua_ktar_open },
        { "extract",    klua_ktar_extract },
        { NULL,         NULL }
    };

    klua_ktar_createmeta(L);
    luaL_newlib(L, lib);
    return 1;
}


// end
