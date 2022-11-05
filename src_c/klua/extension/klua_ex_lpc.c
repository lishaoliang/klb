// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/extension/klua_ex_lpc.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbthird/sds.h"
#include "klbutil/klb_rand.h"
#include <assert.h>


typedef struct klua_ex_lpc_module_t_
{
    int                 module;         ///< 1. module; 0. lpc
    sds                 name;
    klua_ex_lpc_msg_cb  cb_msg;
    void*               ptr;
}klua_ex_lpc_module_t;


/// @struct klua_ex_lpc_t
/// @brief  lpc
typedef struct klua_ex_lpc_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    lua_State*          L;              ///< L

    klb_hlist_t*        p_module_hlist; ///< 模块列表
}klua_ex_lpc_t;

//////////////////////////////////////////////////////////////////////////


int klua_ex_lpc_new_module(klua_ex_lpc_t* p_ex, const sds name, klua_ex_lpc_msg_cb cb_msg, void* ptr)
{
    assert(NULL != cb_msg);

    klua_ex_lpc_module_t* p_mo = KLB_MALLOCZ(klua_ex_lpc_module_t, 1, 0);
    p_mo->module = 1;
    p_mo->name = sdsnew(name);
    p_mo->cb_msg = cb_msg;
    p_mo->ptr = ptr;

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_module_hlist, name, sdslen(name), p_mo);
    assert(NULL != p_iter);

    int ret = klua_thread_register_lpc_module(p_mo->name, p_ex->p_env);
    assert(0 == ret);

    return ret;
}

sds klua_ex_lpc_new_lpc(klua_ex_lpc_t* p_ex, klua_ex_lpc_msg_cb cb_msg, void* ptr)
{
    assert(NULL != cb_msg);

    klua_ex_lpc_module_t* p_mo = KLB_MALLOCZ(klua_ex_lpc_module_t, 1, 0);
    p_mo->module = 0;
    p_mo->name = klua_thread_register_lpc(p_ex->p_env);
    p_mo->cb_msg = cb_msg;
    p_mo->ptr = ptr;

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_module_hlist, p_mo->name, sdslen(p_mo->name), p_mo);
    assert(NULL != p_iter);

    return sdsnew(p_mo->name);
}

int klua_ex_lpc_delete(klua_ex_lpc_t* p_ex, const sds name)
{
    klua_ex_lpc_module_t* p_mo = (klua_ex_lpc_module_t*)klb_hlist_remove_bykey(p_ex->p_module_hlist, name, sdslen(name));
    if (NULL != p_mo)
    {
        if (1 == p_mo->module)
        {
            klua_thread_unregister_lpc_module(name);
        }
        else
        {
            klua_thread_unregister_lpc(name);
        }

        KLB_FREE_BY(p_mo->name, sdsfree);
        KLB_FREE(p_mo);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
static klua_ex_lpc_module_t* get_module_klua_ex_lpc(klua_ex_lpc_t* p_ex, const char* p_module)
{
    return (klua_ex_lpc_module_t*)klb_hlist_find(p_ex->p_module_hlist, p_module, strlen(p_module));
}

//////////////////////////////////////////////////////////////////////////

static void* klua_ex_lpc_create(klua_env_t* p_env)
{
    klua_ex_lpc_t* p_ex = KLB_MALLOCZ(klua_ex_lpc_t, 1, 0);

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    p_ex->p_module_hlist = klb_hlist_create(0);

    return p_ex;
}

static void klua_ex_lpc_destroy(void* ptr)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    KLB_FREE_BY(p_ex->p_module_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_lpc_msg(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;
    klua_msg_t* p_lpc_msg = (klua_msg_t*)p_msg;

    klua_ex_lpc_module_t* p_mo = get_module_klua_ex_lpc(p_ex, p_lpc_msg->dst_name);
    if (NULL != p_mo)
    {
        p_mo->cb_msg(p_mo->ptr, p_msg);
    }
    else
    {
        assert(false);
        KLB_FREE_BY(p_msg, klua_msg_free);
    }

    return 0;
}

static int klua_ex_lpc_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klua_ex_register_lpc(klua_env_t* p_env)
{
    klua_env_extension_t ex = { 0 };

    ex.cb_create = klua_ex_lpc_create;
    ex.cb_destroy = klua_ex_lpc_destroy;
    ex.cb_msg = klua_ex_lpc_msg;
    ex.cb_loop_once = klua_ex_lpc_loop_once;

    klua_env_register_extension(p_env, KLUA_EX_LPC_NAME, &ex);

    return 0;
}

klua_ex_lpc_t* klua_ex_get_lpc(klua_env_t* p_env)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)klua_env_get_extension(p_env, KLUA_EX_LPC_NAME);
    return p_ex;
}
