// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua_env.h"
#include "lstate.h"
#include "klua/klua.h"
#include "klua_help.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbplatform/klb_time.h"
#include "klbutil/klb_hlist.h"
#include "klbutil/klb_nlist.h"
#include "klbthird/sds.h"
#include "klua/klua_thread.h"
#include "klbplatform/klb_atomic.h"
#include "klua/extension/klua_extension.h"
#include "klua/extension/klua_ex_lpc.h"
#include "klua/klua_util/klua_kobject.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "lstate.h"
#include <assert.h>


#if 0
#define KLUA_ENV_PTR            "_KLUA_ENV_PTR_"
#endif

#define KLUA_ENV_GC_TIME_OUT    30000   ///< 30 * 1000 = 30秒


/// @struct klua_env_extension_activate_t
/// @brief  激活的扩展
typedef struct klua_env_extension_activate_t_
{
    void*                ptr;           ///< 扩展的指针
    sds                  name;          ///< 扩展的名称
    klua_env_extension_t ex;            ///< 扩展的接口函数
}klua_env_extension_activate_t;


/// @struct klua_env_t
/// @brief  Lua环境
typedef struct klua_env_t_
{
    struct
    {
        klb_atomic_t volatile   is_get_lpc_msg; ///< 是否 取 LPC 消息
        klb_atomic_t volatile   lpc_msg_lock;   ///< p_lpc_msg_list锁

        klb_nlist_t*     p_lpc_msg_list; ///< LPC 消息列表
        klb_nlist_t*     p_msg_list;     ///< 待处理消息列表
    };

    struct
    {
        lua_State*      L;              ///< Lua环境

        int             G;              ///< "G"
        int             kexit;          ///< "kexit"
    };

    struct
    {
        klb_hlist_t* p_extension_hlist;             ///< 注册的扩展; klua_env_extension_t*
        klb_hlist_t* p_extension_activate_hlist;    ///< 激活的扩展; klua_env_extension_activate_t*
    };

    struct
    {
        klb_buf_t*      p_arg;          ///< 全局参数(启动参数)

        sds             name;           ///< 线程名称
        int             is_exit;        ///< 是否退出

        int64_t         tc;             ///< 当前时间

        int64_t         gc_tc;          ///< 上次gc时间
        int64_t         gc_interval;    ///< gc间隔
    };

    struct
    {
        void*           p_udata;        ///< user data
    };
}klua_env_t;

static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load);
static int klua_pquit(lua_State *L);
static void klua_pref(klua_env_t* p_env, lua_State* L);


klua_env_t* klua_env_create(lua_CFunction cb_pre_load)
{
    klua_env_t* p_env = KLB_MALLOCZ(klua_env_t, 1, 0);

    p_env->p_extension_hlist = klb_hlist_create(0);
    p_env->p_extension_activate_hlist = klb_hlist_create(0);

    klb_atomic_set_zero(&p_env->is_get_lpc_msg);
    klb_atomic_set_zero(&p_env->lpc_msg_lock);

    p_env->p_lpc_msg_list = klb_nlist_create();
    p_env->p_msg_list = klb_nlist_create();
    
    p_env->is_exit = false;

    p_env->tc = klb_tick_counti64();

    p_env->gc_interval = KLUA_ENV_GC_TIME_OUT;
    p_env->gc_tc = p_env->tc;

    p_env->L = luaL_newstate(); // Lua运行环境
    p_env->L->udata = p_env;

    // 注册标准扩展 C
    klua_register_extension_std(p_env);

    // 注册标准扩展 CPP
    klua_register_extension_std_cpp(p_env);

    // lua环境初始化
    klua_env_init(p_env, cb_pre_load);

    return p_env;
}

void klua_env_destroy(klua_env_t* p_env)
{
    assert(NULL != p_env);

    // 注销激活的扩展
    while (0 < klb_hlist_size(p_env->p_extension_activate_hlist))
    {
        klua_env_extension_activate_t* p_tmp = (klua_env_extension_activate_t*)klb_hlist_pop_head(p_env->p_extension_activate_hlist);
        
        p_tmp->ex.cb_destroy(p_tmp->ptr);
        
        KLB_FREE_BY(p_tmp->name, sdsfree);
        KLB_FREE(p_tmp);
    }

    // 销毁注册的扩展
    while (0 < klb_hlist_size(p_env->p_extension_hlist))
    {
        klua_env_extension_t* p_extension = (klua_env_extension_t*)klb_hlist_pop_head(p_env->p_extension_hlist);
        KLB_FREE(p_extension);
    }

    // 销毁Lua环境
    KLB_FREE_BY(p_env->L, lua_close);

    KLB_FREE(p_env->p_arg);
    KLB_FREE_BY(p_env->name, sdsfree);
    KLB_FREE_BY(p_env->p_msg_list, klb_nlist_destroy);
    KLB_FREE_BY(p_env->p_lpc_msg_list, klb_nlist_destroy);
    KLB_FREE_BY(p_env->p_extension_activate_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_env->p_extension_hlist, klb_hlist_destroy);
    KLB_FREE(p_env);
}

void klua_env_set_udata(klua_env_t* p_env, void* p_udata)
{
    assert(NULL != p_env);
    p_env->p_udata = p_udata;
}

void* klua_env_get_udata(klua_env_t* p_env)
{
    assert(NULL != p_env);

    return p_env->p_udata;
}

int64_t klua_env_get_tick_count(klua_env_t* p_env)
{
    assert(NULL != p_env);
    return p_env->tc;
}

void klua_env_update_tick_count(klua_env_t* p_env)
{
    assert(NULL != p_env);
    p_env->tc = klb_tick_counti64();
}

static int klua_pdofile(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dofile(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        lua_pushboolean(L, true);
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
        lua_pushboolean(L, false);
    }

    return 1;
}

static int klua_pdolibrary(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    char* p_loader = (char*)lua_tostring(L, 2);

    if (LUA_OK == klua_help_dolibrary(p_env->L, p_loader))
    {
        klua_pref(p_env, L);        // 保存地址
        lua_pushboolean(L, true);
    }
    else
    {
        KLB_LOG_E("klua_env dofile error!path:%s\n", p_loader);
        lua_pushboolean(L, false);
    }

    return 1;
}

int klua_env_dofile(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);

    // 更新时间
    p_env->tc = klb_tick_counti64();
    p_env->gc_tc = p_env->tc;

    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdofile);    /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    int ret = (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    if (EXIT_SUCCESS != ret)
    {
        p_env->is_exit = true;
    }

    return ret;
}

int klua_env_dolibrary(klua_env_t* p_env, const char* p_loader)
{
    assert(NULL != p_env);

    // 更新时间
    p_env->tc = klb_tick_counti64();
    p_env->gc_tc = p_env->tc;

    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pdolibrary); /* to call 'klua_pdofile' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushstring(L, p_loader);            /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    int ret = (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
    if (EXIT_SUCCESS != ret)
    {
        p_env->is_exit = true;
    }

    return ret;
}

int klua_env_doend(klua_env_t* p_env)
{
    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_pushcfunction(L, &klua_pquit);      /* to call 'klua_pquit' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    int status = lua_pcall(L, 1, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

klua_env_t* klua_env_get_by_L(lua_State* L)
{
    assert(NULL != L);

    return (klua_env_t*)G(L)->mainthread->udata;

#if 0
    lua_getglobal(L, KLUA_ENV_PTR);

    if (LUA_TLIGHTUSERDATA == lua_type(L, -1))
    {
        return (klua_env_t*)lua_touserdata(L, -1);
    }

    assert(false);
    return NULL;
#endif
}

lua_State* klua_env_get_L(klua_env_t* p_env)
{
    assert(NULL != p_env);
    return p_env->L;
}

int klua_env_report(klua_env_t* p_env, int status)
{
    assert(NULL != p_env);
    return klua_help_report(p_env->L, status);
}

int klua_env_report_by_L(lua_State* L, int status)
{
    return klua_help_report(L, status);
}

static int klua_env_call_kexit(klua_env_t* p_env)
{
    assert(NULL != p_env);
    if (p_env->kexit <= 0) return EXIT_FAILURE;

    lua_State* L = p_env->L;
    KLUA_HELP_TOP_B(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_env->kexit);/* to call 'kexit' in protected mode */
    int status = lua_pcall(L, 0, 1, 0);             /* do the call */
    int result = lua_toboolean(L, -1);              /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    KLUA_HELP_TOP_E(L);
    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

static void klua_pref(klua_env_t* p_env, lua_State* L)
{
    assert(NULL != p_env);
    assert(NULL != L);

    int id = lua_getglobal(L, "G");
    if (0 < id) { p_env->G = luaL_ref(L, LUA_REGISTRYINDEX); }

    id = lua_getglobal(L, "kexit");
    if (0 < id) { p_env->kexit = luaL_ref(L, LUA_REGISTRYINDEX); }
}

static void klua_punref(klua_env_t* p_env, lua_State* L)
{
    assert(NULL != p_env);
    assert(NULL != L);

    if (0 < p_env->kexit)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->kexit);
        p_env->kexit = 0;
    }

    if (0 < p_env->G)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, p_env->G);
        p_env->G = 0;
    }
}

static int klua_exit(lua_State* L)
{
    klua_env_t* p_env = klua_env_get_by_L(L);

    p_env->is_exit = true;

    return 0;
}

static int klua_pmain(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);
    lua_CFunction cb_pre_load = (lua_CFunction)lua_touserdata(L, 2);

#if 0
    // 将环境指针保存到全局, 如果有更好的方法则可换掉
    lua_pushlightuserdata(L, p_env);
    lua_setglobal(L, KLUA_ENV_PTR);
#endif

    // 加载标准库
    luaL_openlibs(L);

    // 加载"k*"系列额外库
    //klua_loadlib(L, klua_open_kos, "kos");
    //klua_loadlib(L, klua_open_ktime, "ktime");
    //klua_loadlib(L, klua_open_kthread, "kthread");

    // object
    createmeta_kobject_handle(L);

    // 加载自定义库
    if (cb_pre_load) { cb_pre_load(L); }

    lua_pushboolean(L, true);
    return 1;
}

static void klua_pquit_extension_opt(klua_env_t* p_env)
{
    // 向所有扩展调用退出消息
    // klb_hlist_t* p_extension_activate_hlist
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_env->p_extension_activate_hlist);
    
    while (NULL != p_iter)
    {
        klua_env_extension_activate_t* p_activate = (klua_env_extension_activate_t*)klb_hlist_data(p_iter);

        if (p_activate && p_activate->ex.cb_ctrl)
        {
            p_activate->ex.cb_ctrl(p_activate->ptr, p_env, KLUA_ENV_EX_quit, NULL, 0);
        }

        p_iter = klb_hlist_next(p_iter);
    }
}

static int klua_pquit(lua_State *L)
{
    klua_env_t* p_env = (klua_env_t*)lua_touserdata(L, 1);

    // call kexit
    klua_env_call_kexit(p_env);

    // exit extension opt
    klua_pquit_extension_opt(p_env);

    // exit coroutine
    klua_ex_coroutine_exit(p_env, p_env->tc);

    // unref
    klua_punref(p_env, L);

    // gc
    lua_gc(L, LUA_GCCOLLECT, 0);

    p_env->is_exit = true;
    lua_pushboolean(L, true);
    return 1;
}

static int klua_env_init(klua_env_t* p_env, lua_CFunction cb_pre_load)
{
    assert(NULL != p_env);
    lua_State* L = p_env->L;
    lua_pushcfunction(L, &klua_pmain);      /* to call 'klua_pmain' in protected mode */
    lua_pushlightuserdata(L, p_env);        /* 1st argument */
    lua_pushlightuserdata(L, (void*)cb_pre_load);  /* 2st argument */
    int status = lua_pcall(L, 2, 1, 0);     /* do the call */
    int result = lua_toboolean(L, -1);      /* get result */
    klua_help_report(L, status);
    if (LUA_OK == status) { lua_pop(L, 1); }

    return (result && status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

void klua_msg_free(klua_msg_t* p_msg)
{
    assert(NULL != p_msg);

    switch (p_msg->type)
    {
    case KLUA_LPC_POST:
    case KLUA_LPC_REQUEST:
    case KLUA_LPC_RESPONSE:
        {
            KLB_FREE(p_msg->p_data);
        }
        break;
    default:
        break;
    }

    KLB_FREE(p_msg);
}

int klua_env_register_extension(klua_env_t* p_env, const char* p_name, const klua_env_extension_t* p_extension)
{
    assert(NULL != p_env);
    assert(NULL != p_name);
    assert(NULL != p_extension);
    assert(NULL != p_extension->cb_create);
    assert(NULL != p_extension->cb_destroy);

    size_t name_len = strlen(p_name);

    klua_env_extension_t* p_tmp = KLB_MALLOC(klua_env_extension_t, 1, 0);
    KLB_MEMSET(p_tmp, 0, sizeof(klua_env_extension_t));

    memcpy(p_tmp, p_extension, sizeof(klua_env_extension_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_env->p_extension_hlist, p_name, name_len, p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG_E("register lua env extension error!name:[%s]\n", p_name);
        return 1; // 放入失败, 名称重复
    }

    return 0;
}

static klua_env_extension_activate_t* klua_env_get_extension_activate(klua_env_t* p_env, const char* p_name)
{
    assert(NULL != p_env);
    assert(NULL != p_name);

    size_t name_len = strlen(p_name);

    klua_env_extension_activate_t* p_activate = (klua_env_extension_activate_t*)klb_hlist_find(p_env->p_extension_activate_hlist, p_name, name_len);
    if (NULL != p_activate)
    {
        return p_activate;
    }

    klua_env_extension_t* p_extension = (klua_env_extension_t*)klb_hlist_find(p_env->p_extension_hlist, p_name, name_len);
    if (NULL != p_extension)
    {
        klua_env_extension_activate_t* p_tmp = KLB_MALLOCZ(klua_env_extension_activate_t, 1, 0);

        memcpy(&p_tmp->ex, p_extension, sizeof(klua_env_extension_t));
        p_tmp->name = sdsnewlen(p_name, name_len);

        p_tmp->ptr = p_tmp->ex.cb_create(p_env);
        assert(NULL != p_tmp->ptr);

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_env->p_extension_activate_hlist, p_name, name_len, p_tmp);
        assert(NULL != p_iter);

        return p_tmp;
    }

    return NULL;
}

void* klua_env_get_extension(klua_env_t* p_env, const char* p_name)
{
    klua_env_extension_activate_t* p_activate = klua_env_get_extension_activate(p_env, p_name);

    if (NULL != p_activate)
    {
        return p_activate->ptr;
    }

    return NULL;
}

static int klua_env_loop_msg(klua_env_t* p_env, int64_t now)
{
    while (0 < klb_nlist_size(p_env->p_msg_list))
    {
        klua_msg_t* p_msg = (klua_msg_t*)klb_nlist_pop_head(p_env->p_msg_list);
        assert(NULL != p_msg);

        switch (p_msg->type)
        {
        case KLUA_LPC_POST:
        case KLUA_LPC_REQUEST:
        case KLUA_LPC_RESPONSE:
        case KLUA_LPC_NOTIFY:
            {
                klua_env_extension_activate_t* p_ex_lpc = klua_env_get_extension_activate(p_env, KLUA_EX_LPC_NAME);
                assert(NULL != p_ex_lpc);
                assert(NULL != p_ex_lpc->ex.cb_msg);

                p_ex_lpc->ex.cb_msg(p_ex_lpc->ptr, p_env, now, p_msg);
            }
            break;
        default:
            {
                klua_msg_free(p_msg);
            }
            break;
        }
    }

    return 0;
}

static void klua_env_get_msg(klua_env_t* p_env)
{
    klb_atomic_lock(&p_env->lpc_msg_lock);

    while (0 < klb_nlist_size(p_env->p_lpc_msg_list))
    {
        klua_msg_t* p_msg = (klua_msg_t*)klb_nlist_pop_head(p_env->p_lpc_msg_list);
        klb_nlist_push_tail(p_env->p_msg_list, p_msg);
    }

    klb_atomic_unlock(&p_env->lpc_msg_lock);
}

int klua_env_loop_once(klua_env_t* p_env)
{
    assert(NULL != p_env);
    int64_t now = klb_tick_counti64();

    // 获取消息
    if (!klb_atomic_is_zero(&p_env->is_get_lpc_msg))
    {
        klua_env_get_msg(p_env);
        klb_atomic_set_zero(&p_env->is_get_lpc_msg);
    }

    // bug. 后续流程可能依赖计时器tc, 这里需要先更新当前时钟
    // eg. 若在后面更新tc, 则kco.co_sleep()时会导致计时函数死循环
    p_env->tc = now;

    // 处理消息
    klua_env_loop_msg(p_env, now);

    int n = 0;

    // loop once extension
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_env->p_extension_activate_hlist);
    while (NULL != p_iter)
    {
        klua_env_extension_activate_t* p_activate = (klua_env_extension_activate_t*)klb_hlist_data(p_iter);

        if (NULL != p_activate->ex.cb_loop_once)
        {
            n += p_activate->ex.cb_loop_once(p_activate->ptr, p_env, p_env->tc, now);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    // gc
    if (p_env->gc_interval <= ABS_SUB(now, p_env->gc_tc))
    {
        p_env->gc_tc = now;

        int pre_used_kb = lua_gc(p_env->L, LUA_GCCOUNT, 0);     // 获取lua使用的内存
        lua_gc(p_env->L, LUA_GCCOLLECT, 0);
        int after_used_kb = lua_gc(p_env->L, LUA_GCCOUNT, 0);   // 获取gc之后,lua使用的内存

        //KLB_LOG("klua_env_loop_once gc:[%dKB]\n", pre_used_kb - after_used_kb);
    }

    return 5 - n;
}

bool klua_env_is_exit(klua_env_t* p_env)
{
    return p_env->is_exit;
}

void klua_env_exit(klua_env_t* p_env)
{
    p_env->is_exit = true;
}

/// @brief 设置名称
/// @param [in] *p_env              lua环境
/// @return 无
void klua_env_set_name(klua_env_t* p_env, const char* p_name, size_t name_len)
{
    assert(NULL != p_env);
    KLB_FREE_BY(p_env->name, sdsfree);
    p_env->name = sdsnewlen(p_name, name_len);
}


/// @brief 获取名称
/// @param [in] *p_env              lua环境
/// @return sds 名称
const sds klua_env_get_name(klua_env_t* p_env)
{
    assert(NULL != p_env);
    return p_env->name;
}

/// @brief 设置全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
/// @param [in] *p_env              lua环境
/// @return 无
void klua_env_set_args(klua_env_t* p_env, const char* p_data, int data_len)
{
    assert(NULL != p_env);

    KLB_FREE(p_env->p_arg);

    if (NULL != p_data && 0 < data_len)
    {
        p_env->p_arg = klb_buf_malloc(KLB_PADDING_4(data_len), false);
        memcpy(p_env->p_arg->p_buf, p_data, data_len);
        p_env->p_arg->end = data_len;
    }
}


/// @brief 获取全局参数: 数据格式参考 luaseri_pack/luaseri_pack_from
/// @param [in] *p_env              lua环境
/// @return 无
const klb_buf_t* klua_env_get_args(klua_env_t* p_env)
{
    return p_env->p_arg;
}

void klua_env_push_lpc_msg(klua_env_t* p_env, klua_msg_t* p_msg)
{
    klb_atomic_lock(&p_env->lpc_msg_lock);
    klb_nlist_push_tail(p_env->p_lpc_msg_list, p_msg);
    klb_atomic_unlock(&p_env->lpc_msg_lock);

    klb_atomic_set_value(&p_env->is_get_lpc_msg, 1);
}
