// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_netmulti.h"
#include "klua/klua_netmulti.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define KLUAEX_netmulti           "_KLUAEX-netmulti_"

typedef struct klua_ex_netmulti_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    klb_netmulti_t*     p_multi;        ///< 复用指针
}klua_ex_netmulti_t;


//////////////////////////////////////////////////////////////////////////

static void* klua_ex_netmulti_create(klua_env_t* p_env)
{
    klua_ex_netmulti_t* p_ex = KLB_MALLOCZ(klua_ex_netmulti_t, 1, 0);

    p_ex->p_env = p_env;

    p_ex->p_multi = klb_netmulti_create(klua_env_get_tick_count(p_env));

    return p_ex;
}

static void klua_ex_netmulti_destroy(void* ptr)
{
    klua_ex_netmulti_t* p_ex = (klua_ex_netmulti_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    KLB_FREE_BY(p_ex->p_multi, klb_netmulti_destroy);

    KLB_FREE(p_ex);
}

static int klua_ex_netmulti_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_netmulti_t* p_ex = (klua_ex_netmulti_t*)ptr;

    int ret = klb_netmulti_loop_once(p_ex->p_multi, now);

    return ret;
}

//////////////////////////////////////////////////////////////////////////

klb_netmulti_t* klua_ex_netmulti_get(klua_ex_netmulti_t* p_ex)
{
    return p_ex->p_multi;
}

klb_netmulti_t* klua_ex_netmulti_get_by_L(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    assert(NULL != p_env);

    klua_ex_netmulti_t* p_ex = klua_ex_get_netmulti(p_env);
    assert(NULL != p_ex);

    return p_ex->p_multi;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_netmulti(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_netmulti_create;
    ex.cb_destroy = klua_ex_netmulti_destroy;
    ex.cb_loop_once = klua_ex_netmulti_loop_once;

    klua_env_register_extension(p_env, KLUAEX_netmulti, &ex);

    return 0;
}

klua_ex_netmulti_t* klua_ex_get_netmulti(klua_env_t* p_env)
{
    klua_ex_netmulti_t* p_ex = (klua_ex_netmulti_t*)klua_env_get_extension(p_env, KLUAEX_netmulti);
    return p_ex;
}


//////////////////////////////////////////////////////////////////////////
// "klua/klua_netmulti.h"

/// @brief 获取复用
/// @param [in]  *p_env         Lua环境 
/// @return klb_netmulti_t* 
klb_netmulti_t* klua_netmulti_get(klua_env_t* p_env)
{
    return klua_ex_netmulti_get(klua_ex_get_netmulti(p_env));
}


/// @brief 获取复用
/// @param [in]  *L             lua_State 
/// @return klb_netmulti_t* 
klb_netmulti_t* klua_netmulti_get_by_L(lua_State* L)
{
    return klua_ex_netmulti_get_by_L(L);
}
