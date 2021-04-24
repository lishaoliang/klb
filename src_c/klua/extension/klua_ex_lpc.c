#include "klua/extension/klua_ex_lpc.h"
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include "klbthird/sds.h"
#include "klbutil/klb_rand.h"
#include <assert.h>


/// @struct klua_ex_lpc_function_t
/// @brief  注册的Lua脚本函数
typedef struct klua_ex_lpc_function_t_
{
    int     reg_on_callback;        ///< Lua函数引用
}klua_ex_lpc_function_t;


/// @struct klua_ex_lpc_response_t
/// @brief  响应处理
typedef struct klua_ex_lpc_response_t_
{
    void*                   ptr;    ///< 用户指针
    klua_ex_lpc_response_cb cb;     ///< 响应函数
}klua_ex_lpc_response_t;


/// @struct klua_ex_lpc_t
/// @brief  lpc
typedef struct klua_ex_lpc_t_
{
    klua_env_t*         p_env;          ///< Lua环境
    lua_State*          L;              ///< L

    klb_hlist_t*        p_func_hlist;   ///< 注册的函数列表: klua_ex_lpc_function_t*

    klb_hlist_t*        p_response_list;///< 响应列表: klua_ex_lpc_response_t*
}klua_ex_lpc_t;


//////////////////////////////////////////////////////////////////////////
static int call_klua_ex_lpc_function(klua_ex_lpc_t* p_ex, klua_ex_lpc_function_t* p_func, klua_lpc_msg_t* p_msg);


static void* klua_ex_lpc_create(klua_env_t* p_env)
{
    klua_ex_lpc_t* p_ex = KLB_MALLOC(klua_ex_lpc_t, 1, 0);
    KLB_MEMSET(p_ex, 0, sizeof(klua_ex_lpc_t));

    p_ex->p_env = p_env;
    p_ex->L = klua_env_get_L(p_env);

    p_ex->p_func_hlist = klb_hlist_create(0);
    p_ex->p_response_list = klb_hlist_create(0);

    return p_ex;
}

static void klua_ex_lpc_destroy(void* ptr)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;
    klua_env_t* p_env = p_ex->p_env;

    // 清空
    while (0 < klb_hlist_size(p_ex->p_func_hlist))
    {
        klua_ex_lpc_function_t* p_func = (klua_ex_lpc_function_t*)klb_hlist_pop_head(p_ex->p_func_hlist);
        klua_unref_registryindex(p_ex->L, p_func->reg_on_callback);

        KLB_FREE(p_func);
    }

    KLB_FREE_BY(p_ex->p_response_list, klb_hlist_destroy);
    KLB_FREE_BY(p_ex->p_func_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex);
}

static int klua_ex_lpc_msg(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;
    klua_lpc_msg_t* p_lpc_msg = (klua_lpc_msg_t*)p_msg;

    if (KLUA_LPC_MSG_POST == p_lpc_msg->msg)
    {
        klua_ex_lpc_function_t* p_func = klb_hlist_find(p_ex->p_func_hlist, p_lpc_msg->func_name, sdslen(p_lpc_msg->func_name));
        if (NULL != p_func)
        {
            call_klua_ex_lpc_function(p_ex, p_func, p_lpc_msg);
        }
    }
    else if (KLUA_LPC_MSG_REQUEST == p_lpc_msg->msg)
    {
        klua_ex_lpc_function_t* p_func = klb_hlist_find(p_ex->p_func_hlist, p_lpc_msg->func_name, sdslen(p_lpc_msg->func_name));
        if (NULL != p_func)
        {
            call_klua_ex_lpc_function(p_ex, p_func, p_lpc_msg);
        }
    }
    else if (KLUA_LPC_MSG_RESPONSE == p_lpc_msg->msg)
    {
        klua_ex_lpc_response_t* p_response = klb_hlist_remove_bykey(p_ex->p_response_list, p_lpc_msg->sequence, sdslen(p_lpc_msg->sequence));
        if (NULL != p_response)
        {
            p_response->cb(p_response->ptr, p_ex, p_lpc_msg);
            KLB_FREE(p_response);
        }
    }

    return 0;
}

static int klua_ex_lpc_loop_once(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now)
{
    klua_ex_lpc_t* p_ex = (klua_ex_lpc_t*)ptr;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static int call_klua_ex_lpc_function(klua_ex_lpc_t* p_ex, klua_ex_lpc_function_t* p_func, klua_lpc_msg_t* p_msg)
{
    assert(NULL != p_ex);
    if (p_func->reg_on_callback <= 0) return EXIT_FAILURE;

    lua_State* L = p_ex->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_func->reg_on_callback);     /* to call reg_on_callback in protected mode */
    lua_pushlstring(L, p_msg->src_name, sdslen(p_msg->src_name));   /* 1st argument */
    lua_pushlstring(L, p_msg->sequence, p_msg->sequence ? sdslen(p_msg->sequence) : 0); /* 2st argument */

    // 数据
    for (int i = 0; i < p_msg->data_num; i++)
    {
        klua_pushdata(L, &p_msg->data[i]);
    }

    int status = lua_pcall(L, 2 + p_msg->data_num, 0, 0);           /* do the call */
    klua_env_report(p_ex->p_env, status);

    KLUA_HELP_TOP_E(L);
    return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void klua_ex_lpc_register_function(klua_ex_lpc_t* p_ex, const char* p_func_name, size_t name_len, int func_idx)
{
    klua_ex_lpc_function_t* p_func = (klua_ex_lpc_function_t*)klb_hlist_find(p_ex->p_func_hlist, p_func_name, name_len);
    if (NULL != p_func)
    {
        // 释放原来的函数
        klua_unref_registryindex(p_ex->L, p_func->reg_on_callback);

        // 引用新函数
        p_func->reg_on_callback = klua_ref_registryindex(p_ex->L, func_idx);
    }
    else
    {
        // 新建
        klua_ex_lpc_function_t* p_new_func = KLB_MALLOC(klua_ex_lpc_function_t, 1, 0);
        KLB_MEMSET(p_new_func, 0, sizeof(klua_ex_lpc_function_t));

        // 引用
        p_new_func->reg_on_callback = klua_ref_registryindex(p_ex->L, func_idx);

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_func_hlist, p_func_name, name_len, p_new_func);
        assert(NULL != p_iter);
    }
}

static sds klua_ex_lpc_new_sequence(klua_ex_lpc_t* p_ex)
{
    while (true)
    {
        int len = 12;
        char r[16] = { 0 };
        klb_rand_string(r, len, true);

        if (NULL == klb_hlist_find_iter(p_ex->p_response_list, r, len))
        {
            return sdsnewlen(r, len);
        }
    }

    return NULL;
}

sds klua_ex_lpc_new_wait_response(klua_ex_lpc_t* p_ex, klua_ex_lpc_response_cb cb, void* ptr)
{
    sds sequence = klua_ex_lpc_new_sequence(p_ex);

    klua_ex_lpc_response_t* p_response = KLB_MALLOC(klua_ex_lpc_response_t, 1, 0);
    KLB_MEMSET(p_response, 0, sizeof(klua_ex_lpc_response_t));

    p_response->cb = cb;
    p_response->ptr = ptr;

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_response_list, sequence, sdslen(sequence), p_response);
    assert(NULL != p_iter);

    return sequence;
}

void klua_ex_lpc_remove_response(klua_ex_lpc_t* p_ex, sds sequence)
{
    klua_ex_lpc_response_t* p_response = klb_hlist_remove_bykey(p_ex->p_response_list, sequence, sdslen(sequence));
    if (NULL != p_response)
    {
        KLB_FREE(p_response);
    }
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
