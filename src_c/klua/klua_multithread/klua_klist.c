// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbutil/klb_obj.h"
#include "klbplatform/klb_atomic.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"
#include <assert.h>


#define KLUA_KLIST_HANDLE     "KLUA_KLIST_HANDLE*"


/// @struct klua_klist_item_t
/// @brief  线程间共享缓存数据
typedef struct klua_klist_item_t_
{
    long volatile   lock_list;  ///< p_list 的原子锁
    klb_nlist_t*    p_list;     ///< 存储 klb_obj_t*
}klua_klist_item_t;


/// @struct klua_klist_t
/// @brief  线程间共享缓存数据
///  \n 只能单次存取
///  \n 用于多个线程之间 交换 重量级 数据: eg. 监听到的socket,媒体数据流等
typedef struct klua_klist_t_
{
    long volatile   lock_hlist; ///< p_hlist 的原子锁 
    klb_hlist_t*    p_hlist;    ///< 存储 klua_klist_item_t*
}klua_klist_t;


#define lock_klist(PTR_)        { klb_atomic_lock(&((PTR_)->lock_hlist)); }
#define unlock_klist(PTR_)      { klb_atomic_unlock(&((PTR_)->lock_hlist)); }

#define lock_klist_item(PTR_)   { klb_atomic_lock(&((PTR_)->lock_list)); }
#define unlock_klist_item(PTR_) { klb_atomic_unlock(&((PTR_)->lock_list)); }


static klua_klist_t* g_klua_klist = NULL;


//////////////////////////////////////////////////////////////////////////
int klua_klist_init()
{
    assert(NULL == g_klua_klist);

    g_klua_klist = KLB_MALLOCZ(klua_klist_t, 1, 0);    
    klb_atomic_set_zero(&g_klua_klist->lock_hlist);

    g_klua_klist->p_hlist = klb_hlist_create(0);

    return 0;
}

void klua_klist_quit()
{
    assert(NULL != g_klua_klist);

    // 清空
    while (0 < klb_hlist_size(g_klua_klist->p_hlist))
    {
        klua_klist_item_t* p_item = (klua_klist_item_t*)klb_hlist_pop_head(g_klua_klist->p_hlist);

        while (0 < klb_nlist_size(p_item->p_list))
        {
            klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_item->p_list);
            
            klb_obj_destroy_cb cb_destroy = p_obj->ops.cb_destroy;
            assert(NULL != cb_destroy);
            cb_destroy(p_obj);
        }

        KLB_FREE_BY(p_item->p_list, klb_nlist_destroy);
        KLB_FREE(p_item);
    }

    KLB_FREE_BY(g_klua_klist->p_hlist, klb_hlist_destroy);
    KLB_FREE(g_klua_klist);
}

//////////////////////////////////////////////////////////////////////////

static klua_klist_item_t* get_insert_item_klua_klist(klua_klist_t* ptr, const char* p_key, int key_len)
{
    klua_klist_item_t* p_item = (klua_klist_item_t*)klb_hlist_find(ptr->p_hlist, p_key, key_len);
    if (NULL == p_item)
    {
        p_item = KLB_MALLOCZ(klua_klist_item_t, 1, 0);
        klb_atomic_set_zero(&p_item->lock_list);

        p_item->p_list = klb_nlist_create();

        klb_hlist_push_tail(ptr->p_hlist, p_key, key_len, p_item);
    }

    return p_item;
}

////////////////////////////////////////

typedef struct klua_klist_userdata_t_
{
    klua_klist_item_t*  p_item;         ///< 
    klb_nlist_t*        p_tmp_list;     ///< 临时list
}klua_klist_userdata_t;


static klua_klist_userdata_t* new_klua_klist(lua_State* L)
{
    klua_klist_userdata_t* p_item = (klua_klist_userdata_t*)lua_newuserdata(L, sizeof(klua_klist_userdata_t));
    KLB_MEMSET(p_item, 0, sizeof(klua_klist_userdata_t));
    luaL_setmetatable(L, KLUA_KLIST_HANDLE);
    return p_item;
}

static klua_klist_userdata_t* to_klua_klist(lua_State* L, int idx)
{
    klua_klist_userdata_t* p_item = (klua_klist_userdata_t*)luaL_checkudata(L, idx, KLUA_KLIST_HANDLE);
    luaL_argcheck(L, NULL != p_item, idx, "'klist' expected");
    return p_item;
}

static int klua_klist_tostring(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);
    lua_pushfstring(L, "klist:%p", p_ud);

    return 0;
}

static int klua_klist_gc(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);

    p_ud->p_item = NULL;
    KLB_FREE_BY(p_ud->p_tmp_list, klb_nlist_destroy);

    return 0;
}

static int klua_klist_push(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);
    klb_obj_t* p_obj = (klb_obj_t*)luaL_checklightuserdata(L, 2);
    assert(NULL != p_obj);
    assert(NULL != p_obj->ops.cb_destroy);

    lock_klist_item(p_ud->p_item);
    klb_nlist_push_tail(p_ud->p_item->p_list, p_obj);
    unlock_klist_item(p_ud->p_item);

    return 0;
}

static int klua_klist_pop(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);

    klb_obj_t* p_obj = NULL;
    lock_klist_item(p_ud->p_item);
    if (0 < klb_nlist_size(p_ud->p_item->p_list))
    {
        p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_item->p_list);
    }
    unlock_klist_item(p_ud->p_item);

    if (NULL != p_obj)
    {
        lua_pushlightuserdata(L, p_obj);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

static int klua_klist_size(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);

    lock_klist_item(p_ud->p_item);
    uint32_t size = klb_nlist_size(p_ud->p_item->p_list);
    unlock_klist_item(p_ud->p_item);

    lua_pushinteger(L, size);
    return 1;
}

static int klua_klist_clear(lua_State* L)
{
    klua_klist_userdata_t* p_ud = to_klua_klist(L, 1);

    lock_klist_item(p_ud->p_item);

    while (0 < klb_nlist_size(p_ud->p_item->p_list))
    {
        klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_item->p_list);

        // 放入临时表, 稍后释放, 以免释放时耗时过长 
        klb_nlist_push_tail(p_ud->p_tmp_list, p_obj);
    }

    unlock_klist_item(p_ud->p_item);

    // 释放
    while (0 < klb_nlist_size(p_ud->p_tmp_list))
    {
        klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_tmp_list);
        klb_obj_destroy_cb cb_destroy = p_obj->ops.cb_destroy;
        assert(NULL != cb_destroy);

        cb_destroy(p_obj);
    }

    return 0;
}

static void klua_klist_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "push",           klua_klist_push },
        { "pop",            klua_klist_pop },

        { "size",           klua_klist_size },
        { "clear",          klua_klist_clear },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },  /* place holder */
        { "__gc",           klua_klist_gc },
        { "__close",        klua_klist_gc },
        { "__tostring",     klua_klist_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KLIST_HANDLE);/* metatable for KLUA_KLIST_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}

//////////////////////////////////////////////////////////////////////////

static int lib_klua_klist_new(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);

    lock_klist(g_klua_klist);
    klua_klist_item_t* p_item = get_insert_item_klua_klist(g_klua_klist, p_path, path_len);
    unlock_klist(g_klua_klist);

    klua_klist_userdata_t* p_ud = new_klua_klist(L);
    p_ud->p_item = p_item;
    p_ud->p_tmp_list = klb_nlist_create();

    return 0;
}

static int lib_klua_klist_push(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);
    klb_obj_t* p_obj = (klb_obj_t*)luaL_checklightuserdata(L, 2);
    assert(NULL != p_obj);
    assert(NULL != p_obj->ops.cb_destroy);

    lock_klist(g_klua_klist);

    klua_klist_item_t* p_item = get_insert_item_klua_klist(g_klua_klist, p_path, path_len);

    lock_klist_item(p_item);
    klb_nlist_push_tail(p_item->p_list, p_obj);
    unlock_klist_item(p_item);

    unlock_klist(g_klua_klist);

    return 0;
}

static int lib_klua_klist_pop(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);

    klb_obj_t* p_obj = NULL;

    lock_klist(g_klua_klist);

    klua_klist_item_t* p_item = (klua_klist_item_t*)klb_hlist_find(g_klua_klist->p_hlist, p_path, path_len);
    if (NULL != p_item)
    {
        lock_klist_item(p_item);
        if (0 < klb_nlist_size(p_item->p_list))
        {
            p_obj = (klb_obj_t*)klb_nlist_pop_head(p_item->p_list);
        }
        unlock_klist_item(p_item);
    }

    unlock_klist(g_klua_klist);

    if (NULL != p_obj)
    {
        lua_pushlightuserdata(L, p_obj);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

int klua_open_klist(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new",            lib_klua_klist_new },

        { "push",           lib_klua_klist_push },
        { "pop",            lib_klua_klist_pop },

        { NULL,             NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // createmeta
    klua_klist_createmeta(L);

    return 1;
}
