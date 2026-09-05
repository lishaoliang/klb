// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_object.h"
#include "klua/klua_object.h"
#include "klbutil/klb_map.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


typedef struct klua_ex_object_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    lua_State*          L;              ///< L

    klb_map_t           obj_map;        ///< object map
}klua_ex_object_t;

//////////////////////////////////////////////////////////////////////////

static void* klua_ex_object_create(klua_env_t* p_env)
{
    klua_ex_object_t* p_ex = KLB_MALLOCZ(klua_ex_object_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    klb_map_init(&p_ex->obj_map);

    return p_ex;
}

static void klua_ex_object_destroy(void* ptr)
{
    klua_ex_object_t* p_ex = (klua_ex_object_t*)ptr;

    klb_map_quit(&p_ex->obj_map);

    KLB_FREE(p_ex);
}


static int klua_ex_object_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_object_t* p_ex = (klua_ex_object_t*)ptr;


    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_object_register_ops(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops)
{
    assert(NULL != p_ex);
    assert(NULL != p_ops);

    klb_map_t* p_map = klb_map_to_map(&p_ex->obj_map, p_ops->p_module);
    if (NULL == p_map)
    {
        klb_map_set_map(&p_ex->obj_map, p_ops->p_module, NULL);
        p_map = klb_map_to_map(&p_ex->obj_map, p_ops->p_module);
    }

    assert(NULL != p_map);
    klb_map_set_lstring(p_map, p_ops->p_name, (const char*)p_ops, sizeof(klb_obj_ops_t));

    return 0;
}

int klua_ex_object_register_ops_array(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops)
{
    for (; NULL != p_ops->p_name; p_ops++)
    {
        klua_ex_object_register_ops(p_ex, p_ops);
    }

    return 0;
}

const klb_obj_ops_t* klua_ex_object_get_ops(klua_ex_object_t* p_ex, const char* p_module, const char* p_name)
{
    klb_map_t* p_map = klb_map_to_map(&p_ex->obj_map, p_module);
    if (NULL == p_map)
    {
        return NULL;
    }

    int len = 0;
    klb_obj_ops_t* p_ops = (klb_obj_ops_t*)klb_map_to_lstring(p_map, p_name, &len);
    if (0 < len)
    {
        assert(len == sizeof(klb_obj_ops_t));
        return p_ops;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// 

klua_ex_object_t* klua_ex_get_object(klua_env_t* p_env)
{
    klua_ex_object_t* p_ex = (klua_ex_object_t*)klua_env_get_extension(p_env, KLUAEX_object);
    return p_ex;
}

klua_ex_object_t* klua_ex_get_object_by_L(lua_State* L)
{
    return klua_ex_get_object(klua_env_get_by_L(L));
}

int klua_ex_register_object(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_object_create;
    ex.cb_destroy = klua_ex_object_destroy;
    ex.cb_msg = NULL;
    ex.cb_loop_once = klua_ex_object_loop_once;

    klua_env_register_extension(p_env, KLUAEX_object, &ex);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出函数

klua_ex_object_t* klua_object_get(klua_env_t* p_env)
{
    return klua_ex_get_object(p_env);
}

klua_ex_object_t* klua_object_get_by_L(lua_State* L)
{
    return klua_ex_get_object_by_L(L);
}

int klua_object_register_ops(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops)
{
    return klua_ex_object_register_ops(p_ex, p_ops);
}

int klua_object_register_ops_array(klua_ex_object_t* p_ex, const klb_obj_ops_t* p_ops)
{
    return klua_ex_object_register_ops_array(p_ex, p_ops);
}

const klb_obj_ops_t* klua_object_get_ops(klua_ex_object_t* p_ex, const char* p_module, const char* p_name)
{
    return klua_ex_object_get_ops(p_ex, p_module, p_name);
}
