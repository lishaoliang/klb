// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_util/klua_kobject.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_obj.h"
#include "klua/klua_util/klua_seri_map.h"
#include "klua/klua_object.h"
#include <stdlib.h>
#include <assert.h>


static int klua_kobject_tostring(lua_State* L)
{
    klb_obj_t* p_obj = to_klb_obj(L, 1);

    lua_pushfstring(L, "object:[%p],module:[%s],name:[%s]", p_obj, p_obj->ops.p_module, p_obj->ops.p_name);
    return 1;
}

static int klua_kobject_close(lua_State* L)
{
    klb_obj_t* p_obj = to_klb_obj(L, 1);

    if (NULL != p_obj->ops.cb_destroy)
    {
        p_obj->ops.cb_destroy(p_obj);
    }

    return 1;
}

static int klua_kobject_set(lua_State* L)
{
    klb_obj_t* p_obj = to_klb_obj(L, 1);

    if (NULL == p_obj->ops.cb_set)
    {
        return 0;
    }

    klb_map_t* p_map = luaseri_map_pack(L, 1);

    p_obj->ops.cb_set(p_obj, p_map);

    KLB_FREE_BY(p_map, klb_map_destroy);

    return 0;
}

static int klua_kobject_get(lua_State* L)
{
    klb_obj_t* p_obj = to_klb_obj(L, 1);

    if (NULL == p_obj->ops.cb_get)
    {
        return 0;
    }

    klb_map_t* p_map = klb_map_create();

    p_obj->ops.cb_get(p_obj, p_map);

    int n = luaseri_map_unpack(L, 1, p_map);

    KLB_FREE_BY(p_map, klb_map_destroy);
    return n;
}

static int klua_kobject_ctrl(lua_State* L)
{
    klb_obj_t* p_obj = to_klb_obj(L, 1);

    if (NULL == p_obj->ops.cb_ctrl)
    {
        return 0;
    }

    klb_map_t* p_in_map = luaseri_map_pack(L, 1);
    klb_map_t* p_out_map = klb_map_create();

    p_obj->ops.cb_ctrl(p_obj, p_in_map, p_out_map);

    int n = luaseri_map_unpack(L, 1, p_out_map);

    KLB_FREE_BY(p_in_map, klb_map_destroy);
    KLB_FREE_BY(p_out_map, klb_map_destroy);
    return n;
}

void createmeta_kobject_handle(lua_State *L)
{
    static luaL_Reg meth[] =
    {
        { "set",            klua_kobject_set },
        { "get",            klua_kobject_get },

        { "ctrl",           klua_kobject_ctrl },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_kobject_close },
        { "__close",         klua_kobject_close },
        { "__tostring",      klua_kobject_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KOBJECT_HANDLE);      /* metatable for x handles */
    luaL_setfuncs(L, metameth, 0);                  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);                      /* create method table */
    luaL_setfuncs(L, meth, 0);                      /* add file methods to method table */
    lua_setfield(L, -2, "__index");                 /* metatable.__index = method table */
    lua_pop(L, 1);                                  /* pop metatable */
}
