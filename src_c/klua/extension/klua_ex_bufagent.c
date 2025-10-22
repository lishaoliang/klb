// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_bufagent.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define KLUA_EX_BUFAGENT_NAME               "_KLUAEX_BUFAGENT_"


typedef struct klua_ex_bufagent_t_
{
    klua_env_t*             p_env;          ///< Lua环境

    klb_bufpoolfix_t*       p_bufpool;      ///< 固定内存池
}klua_ex_bufagent_t;


//////////////////////////////////////////////////////////////////////////

static void* klua_ex_bufagent_create(klua_env_t* p_env)
{
    klua_ex_bufagent_t* p_ex = KLB_MALLOCZ(klua_ex_bufagent_t, 1, 0);

    p_ex->p_env = p_env;

    p_ex->p_bufpool = klb_bufpoolfix_create();

    return p_ex;
}

static void klua_ex_bufagent_destroy(void* ptr)
{
    klua_ex_bufagent_t* p_ex = (klua_ex_bufagent_t*)ptr;

    KLB_FREE_BY(p_ex->p_bufpool, klb_bufpoolfix_destroy);
    KLB_FREE(p_ex);
}

//////////////////////////////////////////////////////////////////////////


klb_bufpoolfix_t* klua_ex_bufagent_get_bufpoolfix(klua_ex_bufagent_t* p_ex)
{
    return p_ex->p_bufpool;
}

int klua_ex_bufagent_get_bufpoolfix2(klua_ex_bufagent_t* p_ex, klb_bufagent_t* p_out)
{
    p_out->cb_malloc = klb_bufpoolfix_malloc;
    p_out->p_pool = p_ex->p_bufpool;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_bufagent(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_bufagent_create;
    ex.cb_destroy = klua_ex_bufagent_destroy;
    ex.cb_loop_once = NULL;

    klua_env_register_extension(p_env, KLUA_EX_BUFAGENT_NAME, &ex);

    return 0;
}

klua_ex_bufagent_t* klua_ex_get_bufagent(klua_env_t* p_env)
{
    klua_ex_bufagent_t* p_ex = (klua_ex_bufagent_t*)klua_env_get_extension(p_env, KLUA_EX_BUFAGENT_NAME);
    return p_ex;
}

//////////////////////////////////////////////////////////////////////////
// export


/// @brief 获取固定内存池
klb_bufpoolfix_t* klua_bufpoolfix_get(klua_env_t* p_env)
{
    klua_ex_bufagent_t* p_ex = klua_ex_get_bufagent(p_env);
    return klua_ex_bufagent_get_bufpoolfix(p_ex);
}

int klua_bufpoolfix_get2(klua_env_t* p_env, klb_bufagent_t* p_out)
{
    klua_ex_bufagent_t* p_ex = klua_ex_get_bufagent(p_env);
    return klua_ex_bufagent_get_bufpoolfix2(p_ex, p_out);
}

/// @brief 获取固定内存池
klb_bufpoolfix_t* klua_bufpoolfix_get_by_L(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    return klua_bufpoolfix_get(p_env);
}

int klua_bufpoolfix_get2_by_L(lua_State* L, klb_bufagent_t* p_out)
{
    klua_env_t* p_env = klua_env_get_by_L(L);
    return klua_bufpoolfix_get2(p_env, p_out);
}

//end
