// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_mutex.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_mem.h"
#include "klua/lua-skynet/lua-seri.h"
#include <assert.h>


/// @struct klua_kmcache_t
/// @brief  线程间共享缓存数据
typedef struct klua_kmcache_t_
{
    klb_hlist_t*    p_hlist;        ///< hlist容器: klb_buf_t*
    klb_mutex_t*    p_mutex;        ///< 锁
}klua_kmcache_t;


static klua_kmcache_t* g_klua_kmcache = NULL;


//////////////////////////////////////////////////////////////////////////
static void clear_all_klua_kmcache(klua_kmcache_t* p_kmcache)
{
    while (0 < klb_hlist_size(p_kmcache->p_hlist))
    {
        klb_buf_t* p_tmp = klb_hlist_pop_head(p_kmcache->p_hlist);

        KLB_FREE(p_tmp->p_buf);
        KLB_FREE(p_tmp);
    }
}

int klua_kmcache_init()
{
    assert(NULL == g_klua_kmcache);
    g_klua_kmcache = KLB_MALLOCZ(klua_kmcache_t, 1, 0);

    g_klua_kmcache->p_hlist = klb_hlist_create(0);
    g_klua_kmcache->p_mutex = klb_mutex_create();

    return 0;
}

void klua_kmcache_quit()
{
    assert(NULL != g_klua_kmcache);

    // 清空
    clear_all_klua_kmcache(g_klua_kmcache);

    KLB_FREE_BY(g_klua_kmcache->p_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kmcache->p_mutex, klb_mutex_destroy);
    KLB_FREE(g_klua_kmcache);
}


//////////////////////////////////////////////////////////////////////////


/// @brief 设置值
static int klua_kmcache_set(lua_State* L)
{
    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 1, &key_len);

    luaseri_pack_from(L, 1);

    char* ptr = (char*)lua_topointer(L, -2);
    int size = (int)lua_tointeger(L, -1);

    klb_buf_t* p_buf = KLB_MALLOCZ(klb_buf_t, 1, 0);
    p_buf->p_buf = ptr;
    p_buf->end = size;
    p_buf->buf_len = size;

    klb_mutex_lock(g_klua_kmcache->p_mutex);

    klb_buf_t* p_old = klb_hlist_update(g_klua_kmcache->p_hlist, p_key, key_len, p_buf);
    if (NULL != p_old)
    {
        // 更新了, 删除旧数据
        KLB_FREE(p_old->p_buf);
        KLB_FREE(p_old);
    }
    else
    {
        // 未找到, 插入新的
        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(g_klua_kmcache->p_hlist, p_key, key_len, p_buf);
        assert(NULL != p_iter);
    }

    klb_mutex_unlock(g_klua_kmcache->p_mutex);

    return 0;
}

/// @brief 获取值
static int klua_kmcache_get(lua_State* L)
{
    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 1, &key_len);

    int n = 0;
    klb_mutex_lock(g_klua_kmcache->p_mutex);

    klb_buf_t* p_buf = klb_hlist_find(g_klua_kmcache->p_hlist, p_key, key_len);
    if (NULL != p_buf)
    {
        n = luaseri_unpack_by_buffer(L, 1, p_buf->p_buf, p_buf->end);
    }

    klb_mutex_unlock(g_klua_kmcache->p_mutex);
    
    return n;
}


/// @brief 当前数据条目
static int klua_kmcache_size(lua_State* L)
{
    int64_t size = 0;

    klb_mutex_lock(g_klua_kmcache->p_mutex);
    size = klb_hlist_size(g_klua_kmcache->p_hlist);
    klb_mutex_unlock(g_klua_kmcache->p_mutex);

    lua_pushinteger(L, size);
    return 1;
}


/// @brief 清除所有
static int klua_kmcache_clear_all(lua_State* L)
{
    klb_mutex_lock(g_klua_kmcache->p_mutex);
    clear_all_klua_kmcache(g_klua_kmcache);
    klb_mutex_unlock(g_klua_kmcache->p_mutex);

    return 0;
}


int klua_open_kmcache(lua_State* L)
{
    static luaL_Reg lib [] =
    {
        { "set",        klua_kmcache_set },
        { "get",        klua_kmcache_get },

        { "size",       klua_kmcache_size },
        { "clear_all",  klua_kmcache_clear_all },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
