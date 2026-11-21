// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klb_type.h"
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"

#include "lstate.h"


static lua_State *getco(lua_State *L) {
    lua_State *co = lua_tothread(L, 1);
    luaL_argexpected(L, co, 1, "thread");
    return co;
}


/*
** Resumes a coroutine. Returns the number of results for non-error
** cases or -1 for errors.
*/
static int auxresume(lua_State *L, lua_State *co, int narg) {
    int status, nres;
    if (!lua_checkstack(co, narg)) {
        lua_pushliteral(L, "too many arguments to resume");
        return -1;  /* error flag */
    }
    lua_xmove(L, co, narg);
    status = lua_resume(co, L, narg, &nres);
    if (status == LUA_OK || status == LUA_YIELD) {
        if (!lua_checkstack(L, nres + 1)) {
            lua_pop(co, nres);  /* remove results anyway */
            lua_pushliteral(L, "too many results to resume");
            return -1;  /* error flag */
        }
        lua_xmove(co, L, nres);  /* move yielded values */
        return nres;
    }
    else {
        lua_xmove(co, L, 1);  /* move error message */
        return -1;  /* error flag */
    }
}


static int luaB_coresume(lua_State *L) {
    lua_State *co = getco(L);
    int r;
    r = auxresume(L, co, lua_gettop(L) - 1);
    if (r < 0) {
        lua_pushboolean(L, 0);
        lua_insert(L, -2);
        return 2;  /* return false + error message */
    }
    else {
        lua_pushboolean(L, 1);
        lua_insert(L, -(r + 1));
        return r + 1;  /* return true + 'resume' returns */
    }
}


static int luaB_auxwrap(lua_State *L) {
    lua_State *co = lua_tothread(L, lua_upvalueindex(1));
    int r = auxresume(L, co, lua_gettop(L));
    if (r < 0) {  /* error? */
        int stat = lua_status(co);
        if (stat != LUA_OK && stat != LUA_YIELD)  /* error in the coroutine? */
            lua_resetthread(co);  /* close its tbc variables */
        if (stat != LUA_ERRMEM &&  /* not a memory error and ... */
            lua_type(L, -1) == LUA_TSTRING) {  /* ... error object is a string? */
            luaL_where(L, 1);  /* add extra info, if available */
            lua_insert(L, -2);
            lua_concat(L, 2);
        }
        return lua_error(L);  /* propagate error */
    }
    return r;
}


static int luaB_cocreate(lua_State *L) {
    lua_State *NL;
    luaL_checktype(L, 1, LUA_TFUNCTION);
    NL = lua_newthread(L);
    lua_pushvalue(L, 1);  /* move function to top */
    lua_xmove(L, NL, 1);  /* move function from L to NL */
    return 1;
}


static int luaB_cowrap(lua_State *L) {
    luaB_cocreate(L);
    lua_pushcclosure(L, luaB_auxwrap, 1);
    return 1;
}


static int luaB_yield(lua_State *L) {
    return lua_yield(L, lua_gettop(L));
}


#define COS_RUN		0
#define COS_DEAD	1
#define COS_YIELD	2
#define COS_NORM	3
#define COS_ERR		4


static const char *const statname[] =
{ "running", "dead", "suspended", "normal", "error" };


static int auxstatus(lua_State *L, lua_State *co) {
    if (L == co) return COS_RUN;
    else {
        switch (lua_status(co)) {
        case LUA_YIELD:
            return COS_YIELD;
        case LUA_OK: {
            lua_Debug ar;
            if (lua_getstack(co, 0, &ar))  /* does it have frames? */
                return COS_NORM;  /* it is running */
            else if (lua_gettop(co) == 0)
                return COS_DEAD;
            else
                return COS_YIELD;  /* initial state */
        }
        default:  /* some error occurred */
            return COS_DEAD;
        }
    }
}


static int luaB_costatus(lua_State *L) {
    lua_State *co = getco(L);
    lua_pushstring(L, statname[auxstatus(L, co)]);
    return 1;
}


static int luaB_yieldable(lua_State *L) {
    lua_State *co = lua_isnone(L, 1) ? L : getco(L);
    lua_pushboolean(L, lua_isyieldable(co));
    return 1;
}


static int luaB_corunning(lua_State *L) {
    int ismain = lua_pushthread(L);
    lua_pushboolean(L, ismain);
    return 2;
}


static int luaB_close(lua_State *L) {
    lua_State *co = getco(L);
    int status = auxstatus(L, co);
    switch (status) {
    case COS_DEAD: case COS_YIELD: {
        status = lua_resetthread(co);
        if (status == LUA_OK) {
            lua_pushboolean(L, 1);
            return 1;
        }
        else {
            lua_pushboolean(L, 0);
            lua_xmove(co, L, 1);  /* copy error message */
            return 2;
        }
    }
    default:  /* normal or running coroutine */
        return luaL_error(L, "cannot close a %s coroutine", statname[status]);
    }
}


static const luaL_Reg co_funcs[] = {
    { "create", luaB_cocreate },
    { "resume", luaB_coresume },
    { "running", luaB_corunning },
    { "status", luaB_costatus },
    { "wrap", luaB_cowrap },
    { "yield", luaB_yield },
    { "isyieldable", luaB_yieldable },
    { "close", luaB_close },
    { NULL, NULL }
};

#define KLUA_KCO_HANDLE     "KLUA_KCO_HANDLE*"


static klua_coroutine_handle_t* to_klua_kco_h(lua_State* L, int index)
{
    klua_coroutine_handle_t* p_h = (klua_coroutine_handle_t*)luaL_checkudata(L, index, KLUA_KCO_HANDLE);
    luaL_argcheck(L, NULL != p_h, index, "'kco' handle expected");
    return p_h;
}


static klua_coroutine_handle_t* new_klua_kco_h(lua_State* L, klua_coroutine_env_t* p_co_env)
{
    klua_coroutine_handle_t* p_h = (klua_coroutine_handle_t*)lua_newuserdata(L, sizeof(klua_coroutine_handle_t));

    KLB_MEMSET(p_h, 0, sizeof(*p_h));
    p_h->p_co_env = p_co_env;
    p_h->done = false;
    p_h->stat = COS_YIELD;
    p_co_env->p_handle = p_h;
    luaL_setmetatable(L, KLUA_KCO_HANDLE);

    return p_h;
}


static void mark_done_klua_kco_h(klua_coroutine_env_t* p_co_env, bool is_err)
{
    klua_coroutine_handle_t* p_h = NULL;

    if (NULL == p_co_env->p_handle)
    {
        return;
    }

    p_h = (klua_coroutine_handle_t*)p_co_env->p_handle;
    p_h->done = true;
    p_h->stat = is_err ? COS_ERR : COS_DEAD;
    p_h->p_co_env = NULL;
    p_co_env->p_handle = NULL;
}


static int status_of_klua_kco_h(lua_State* L, klua_coroutine_handle_t* p_h)
{
    if (p_h->done)
    {
        return p_h->stat;
    }

    if (NULL == p_h->p_co_env)
    {
        return COS_DEAD;
    }

    return auxstatus(L, p_h->p_co_env->p_co);
}


static bool is_done_stat_klua_kco(int st)
{
    return (COS_DEAD == st || COS_ERR == st);
}


static int klua_kco_h_status(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);

    lua_pushstring(L, statname[status_of_klua_kco_h(L, p_h)]);
    return 1;
}


static int klua_kco_h_is_done(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);

    lua_pushboolean(L, is_done_stat_klua_kco(status_of_klua_kco_h(L, p_h)));
    return 1;
}


static int klua_kco_h_is_error(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);

    lua_pushboolean(L, COS_ERR == status_of_klua_kco_h(L, p_h));
    return 1;
}


static int klua_kco_h_gc(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);

    if (NULL != p_h->p_co_env)
    {
        p_h->p_co_env->p_handle = NULL;
        p_h->p_co_env = NULL;
    }

    return 0;
}


static int klua_kco_h_tostring(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);

    lua_pushfstring(L, "kco handle (%s)", statname[status_of_klua_kco_h(L, p_h)]);
    return 1;
}


static int klua_kco_wrap(lua_State* L)
{
    // 创建
    lua_State *NL;
    luaL_checktype(L, 1, LUA_TFUNCTION);
    NL = lua_newthread(L);
    lua_pushvalue(L, 1);  /* move function to top */
    lua_xmove(L, NL, 1);  /* move function from L to NL */

    // 返回
    lua_pushcclosure(L, luaB_auxwrap, 1);
    return 1;
}

static void free_klua_coroutine_env(klua_coroutine_env_t* p_co_env, bool is_err)
{
    mark_done_klua_kco_h(p_co_env, is_err);

    klua_ex_coroutine_remove(klua_ex_get_coroutine_by_L(p_co_env->p_main), p_co_env); 
    klua_unref_registryindex(p_co_env->p_main, p_co_env->co_reg);

    KLB_FREE(p_co_env);
}

static int klua_kco_auxwrap(lua_State *L)
{
    lua_State *co = lua_tothread(L, lua_upvalueindex(1));
    klua_coroutine_env_t* p_co_env = (klua_coroutine_env_t*)lua_touserdata(L, lua_upvalueindex(2));

    int narg = 0;
    if (p_co_env->is_first)
    {
        p_co_env->is_first = false;

        // 附加参数
        for (int i = 0; i < p_co_env->param_num; i++)
        {
            lua_pushvalue(L, lua_upvalueindex(i + 3));
        }

        narg = lua_gettop(L);
    }
    else
    {
        narg = lua_gettop(L);
    }

    int r = auxresume(L, co, narg/*lua_gettop(L)*/);
    if (r < 0) {  /* error? */
        // 异常结束
        free_klua_coroutine_env(p_co_env, true);

        int stat = lua_status(co);
        if (stat != LUA_OK && stat != LUA_YIELD)  /* error in the coroutine? */
            lua_resetthread(co);  /* close its tbc variables */
        if (stat != LUA_ERRMEM &&  /* not a memory error and ... */
            lua_type(L, -1) == LUA_TSTRING) {  /* ... error object is a string? */
            luaL_where(L, 1);  /* add extra info, if available */
            lua_insert(L, -2);
            lua_concat(L, 2);
        }
        return lua_error(L);  /* propagate error */
    }
    else
    {
        if (LUA_OK == lua_status(co))
        {
            // 协程正常结束
            free_klua_coroutine_env(p_co_env, false);
        }
    }
    return r;
}


static int klua_kco_h_close(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);
    klua_coroutine_env_t* p_co_env = NULL;
    lua_State* p_co = NULL;
    klua_ex_coroutine_t* p_ex = NULL;
    int st = 0;
    int reset_st = LUA_OK;

    st = status_of_klua_kco_h(L, p_h);
    if (is_done_stat_klua_kco(st))
    {
        lua_pushboolean(L, 1);
        return 1;
    }

    if (COS_RUN == st || COS_NORM == st)
    {
        return luaL_error(L, "cannot close a %s coroutine", statname[st]);
    }

    p_co_env = p_h->p_co_env;
    p_co = p_co_env->p_co;
    p_ex = klua_ex_get_coroutine_by_L(p_co_env->p_main);

    klua_ex_coroutine_close_one(p_ex, p_co);
    reset_st = lua_resetthread(p_co);

    if (LUA_OK != reset_st)
    {
        lua_xmove(p_co, L, 1);
        free_klua_coroutine_env(p_co_env, true);
        lua_pushboolean(L, 0);
        lua_insert(L, -2);
        return 2;
    }

    free_klua_coroutine_env(p_co_env, false);
    lua_pushboolean(L, 1);
    return 1;
}


/// @brief wait continuation; deadline in ctx (0=infinite), unit same as co_sleep
static int cont_klua_kco_co_wait(lua_State* L, int status, lua_KContext ctx)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);
    int64_t deadline = (int64_t)ctx;
    int64_t now = 0;
    lua_Integer slice = 10;
    int st = 0;

    (void)status;

    st = status_of_klua_kco_h(L, p_h);
    if (is_done_stat_klua_kco(st))
    {
        lua_pushboolean(L, 1);
        lua_pushboolean(L, COS_ERR == st);
        return 2;
    }

    now = klua_env_get_tick_count(klua_env_get_by_L(L));
    if (0 != deadline && now >= deadline)
    {
        lua_pushboolean(L, 0);
        lua_pushboolean(L, 0);
        return 2;
    }

    if (0 != deadline)
    {
        int64_t remain = deadline - now;

        if (remain < (int64_t)slice)
        {
            slice = (lua_Integer)remain;
        }

        if (slice <= 0)
        {
            slice = 1;
        }
    }

    klua_ex_coroutine_wakeup_timeout(
        klua_ex_get_coroutine_by_L(G(L)->mainthread), L, slice);

    return lua_yieldk(L, 0, ctx, cont_klua_kco_co_wait);
}


static int klua_kco_co_wait(lua_State* L)
{
    klua_coroutine_handle_t* p_h = to_klua_kco_h(L, 1);
    lua_Integer tc = 0;
    int64_t deadline = 0;
    int st = 0;

    if (!lua_isnoneornil(L, 2))
    {
        tc = luaL_checkinteger(L, 2);
    }

    st = status_of_klua_kco_h(L, p_h);
    if (is_done_stat_klua_kco(st))
    {
        lua_pushboolean(L, 1);
        lua_pushboolean(L, COS_ERR == st);
        return 2;
    }

    if (0 < tc)
    {
        deadline = klua_env_get_tick_count(klua_env_get_by_L(L)) + (int64_t)tc;
    }

    return cont_klua_kco_co_wait(L, LUA_OK, (lua_KContext)deadline);
}


static int klua_kco_running(lua_State* L)
{
    return luaB_corunning(L);
}


static int klua_kco_isyieldable(lua_State* L)
{
    return luaB_yieldable(L);
}


static void createmeta_klua_kco(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "status",     klua_kco_h_status },
        { "is_done",    klua_kco_h_is_done },
        { "is_error",   klua_kco_h_is_error },
        { "close",      klua_kco_h_close },
        { "co_wait",    klua_kco_co_wait },
        { NULL,         NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",    NULL },
        { "__gc",       klua_kco_h_gc },
        { "__tostring", klua_kco_h_tostring },
        { NULL,         NULL }
    };

    luaL_newmetatable(L, KLUA_KCO_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}

static klua_coroutine_env_t* new_cowrap_klua_kco(lua_State* L, int idx, int from, int to)
{
    klua_coroutine_env_t* ptr = KLB_MALLOCZ(klua_coroutine_env_t, 1, 0);

    ptr->p_main = G(L)->mainthread;

    lua_State *NL;
    luaL_checktype(L, idx, LUA_TFUNCTION);
    NL = lua_newthread(L);
    lua_pushvalue(L, idx);  /* move function to top */
    lua_xmove(L, NL, 1);    /* move function from L to NL */

    lua_pushlightuserdata(L, ptr); // 参数2

    for (int i = from; i <= to; i++)
    {
        lua_pushvalue(L, i);
        ptr->param_num++;
    }

    lua_pushcclosure(L, klua_kco_auxwrap, 2 + ptr->param_num);

    // 在主程中记录(ref)
    lua_xmove(L, ptr->p_main, 1);  /* move function from L to NL */
    ptr->co_reg = luaL_ref(ptr->p_main, LUA_REGISTRYINDEX);
    ptr->p_co = NL;
    ptr->is_first = true;

    // 初始化
    {
        memset(NL->uname, 0, sizeof(NL->uname));
        NL->udata = NULL;
    }

    //
    klua_ex_coroutine_t* p_ex_co = klua_ex_get_coroutine_by_L(G(L)->mainthread);
    klua_ex_coroutine_push(p_ex_co, ptr);

    return ptr;
}

static int klua_kco_fork(lua_State* L)
{
    klua_coroutine_env_t* ptr = new_cowrap_klua_kco(L, 1, 2, lua_gettop(L));

    klua_ex_coroutine_wakeup(klua_ex_get_coroutine_by_L(G(L)->mainthread), ptr->p_co);
    new_klua_kco_h(L, ptr);

    return 1;
}

static int klua_kco_timeout(lua_State* L)
{
    lua_Integer tc = luaL_checkinteger(L, 1);
    klua_coroutine_env_t* ptr = new_cowrap_klua_kco(L, 2, 3, lua_gettop(L));

    if (0 < tc)
    {
        klua_ex_coroutine_wakeup_timeout(klua_ex_get_coroutine_by_L(G(L)->mainthread), ptr->p_co, tc);
    }
    else
    {
        klua_ex_coroutine_wakeup(klua_ex_get_coroutine_by_L(G(L)->mainthread), ptr->p_co);
    }

    new_klua_kco_h(L, ptr);
    return 1;
}

static int klua_kco_co_sleep(lua_State* L)
{
    lua_Integer tc = 0;

    if (lua_isinteger(L, 1))
    {
        tc = lua_tointeger(L, 1);
    }

    if (0 < tc)
    {
        klua_ex_coroutine_wakeup_timeout(klua_ex_get_coroutine_by_L(G(L)->mainthread), L, tc);
    }
    else
    {
        klua_ex_coroutine_wakeup(klua_ex_get_coroutine_by_L(G(L)->mainthread), L);
    }

    return lua_yield(L, lua_gettop(L));
}

/// @brief 获取当前已登记协程数目
/// @return 1 栈顶为 count[number]
static int klua_kco_count(lua_State* L)
{
    klua_ex_coroutine_t* p_ex = klua_ex_get_coroutine_by_L(G(L)->mainthread);
    int n = klua_ex_coroutine_count(p_ex);

    lua_pushinteger(L, n);
    return 1;
}

/// @brief 关闭所有协程 (quit + 清队列 + 强制收尸残留, count 可归零)
/// @return 0
static int klua_kco_close_all(lua_State* L)
{
    klua_ex_coroutine_t* p_ex = klua_ex_get_coroutine_by_L(G(L)->mainthread);

    klua_ex_coroutine_close_all(p_ex);
    return 0;
}

/// @brief 协程快照 (诊断: count + list[id/status/in_timeout/in_wakeup/has_cb])
/// @return 1 栈顶 table
static int klua_kco_snapshot(lua_State* L)
{
    klua_ex_coroutine_t* p_ex = klua_ex_get_coroutine_by_L(G(L)->mainthread);

    return klua_ex_coroutine_snapshot(p_ex, L);
}

// 协程 coroutine
int klua_open_kco(lua_State* L)
{
    static luaL_Reg kco_lib[] =
    {
        { "fork",           klua_kco_fork },
        { "timeout",        klua_kco_timeout },

        { "co_sleep",       klua_kco_co_sleep },
        { "co_wait",        klua_kco_co_wait },
        { "count",          klua_kco_count },
        { "close_all",      klua_kco_close_all },
        { "snapshot",       klua_kco_snapshot },

        { "status",         klua_kco_h_status },
        { "is_done",        klua_kco_h_is_done },
        { "is_error",       klua_kco_h_is_error },
        { "close",          klua_kco_h_close },

        { "running",        klua_kco_running },
        { "isyieldable",    klua_kco_isyieldable },

        { NULL,             NULL }
    };

    createmeta_klua_kco(L);

#if 1
    // 创建导出库函数
    luaL_newlib(L, kco_lib);
#else
    luaL_checkversion(L);
    luaL_newlibtable(L, kco_lib);

    klua_env_t* ptr = klua_env_get_by_L(L);
    lua_pushlightuserdata(L, ptr);

    luaL_setfuncs(L, kco_lib, 1);
#endif

    return 1;
}

// end
