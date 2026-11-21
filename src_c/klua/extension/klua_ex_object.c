// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_object.h"
#include "klbmem/klb_mem.h"


typedef struct klua_ex_object_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    lua_State*          L;              ///< L
}klua_ex_object_t;

//////////////////////////////////////////////////////////////////////////

static void* klua_ex_object_create(klua_env_t* p_env)
{
    klua_ex_object_t* p_ex = KLB_MALLOCZ(klua_ex_object_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    return p_ex;
}

static void klua_ex_object_destroy(void* ptr)
{
    klua_ex_object_t* p_ex = (klua_ex_object_t*)ptr;

    KLB_FREE(p_ex);
}


static int klua_ex_object_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    (void)ptr;
    (void)p_env;
    (void)last_tc;
    (void)now;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

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

// end
