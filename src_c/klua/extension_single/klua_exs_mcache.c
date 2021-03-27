#include "klua/extension_single/klua_exs_mcache.h"
#include <assert.h>


#define KLUA_EXS_MCACHE_NAME        "_KLUA_EXS_MCACHE_"


typedef struct klua_exs_mcache_t_
{
    int a;
}klua_exs_mcache_t;

//////////////////////////////////////////////////////////////////////////


static void* klua_exs_mcache_create(klua_env_t* p_env)
{
    return NULL;
}


static void klua_exs_mcache_destroy(void* ptr)
{

}

static int klua_exs_mcache_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_exs_register_mcache(klua_env_t* p_env)
{
    klua_env_extension_t exs = { 0 };

    exs.cb_create = klua_exs_mcache_create;
    exs.cb_destroy = klua_exs_mcache_destroy;
    //exs.cb_loop_once = klua_exs_mcache_loop_once;

    klua_env_register_extension(p_env, KLUA_EXS_MCACHE_NAME, &exs);
    return 0;
}

klua_exs_mcache_t* klua_exs_get_mcache(klua_env_t* p_env)
{
    klua_exs_mcache_t* p_exs = (klua_exs_mcache_t*)klua_env_get_extension(p_env, KLUA_EXS_MCACHE_NAME);
    return p_exs;
}
