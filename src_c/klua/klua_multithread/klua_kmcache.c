#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbplatform/klb_mutex.h"
#include "klbplatform/klb_atomic.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"
#include "klbthird/sds.h"
#include <assert.h>


/// @struct klua_kmcache_item_t
/// @brief  共享缓存元素项
typedef struct klua_kmcache_item_t_
{
#define KLUA_KMCACHE_TNIL               LUA_TNIL                ///< nil
#define KLUA_KMCACHE_TBOOLEAN           LUA_TBOOLEAN            ///< boolean
#define KLUA_KMCACHE_TLIGHTUSERDATA     LUA_TLIGHTUSERDATA      ///< void*
#define KLUA_KMCACHE_TNUMBER            LUA_TNUMBER             ///< lua_Number 浮点类型
#define KLUA_KMCACHE_TSTRING            LUA_TSTRING             ///< char*
#define KLUA_KMCACHE_TINTEGER           LUA_NUMTYPES + 10       ///< lua_Integer
    int                 type;   ///< 类型: LUA_TBOOLEAN, LUA_TNUMBER, LUA_TSTRING

    union
    {
        lua_Integer     i;      ///< 整数
        lua_Number      num;    ///< 浮点数
        sds             str;    ///< 字符串
        void*           ptr;    ///< C指针
    };
}klua_kmcache_item_t;


/// @struct klua_kmcache_t
/// @brief  线程间共享缓存数据
typedef struct klua_kmcache_t_
{
    klb_hlist_t*    p_hlist;        ///< hlist容器: klua_kmcache_item_t*
    klb_mutex_t*    p_mutex;        ///< 锁
}klua_kmcache_t;


static klua_kmcache_t* g_klua_kmcache = NULL;


//////////////////////////////////////////////////////////////////////////
static void klua_kmcache_clear_all(klua_kmcache_t* p_kmcache);


int klua_kmcache_init()
{
    assert(NULL == g_klua_kmcache);
    g_klua_kmcache = KLB_MALLOC(klua_kmcache_t, 1, 0);
    KLB_MEMSET(g_klua_kmcache, 0, sizeof(klua_kmcache_t));

    g_klua_kmcache->p_hlist = klb_hlist_create(0);
    g_klua_kmcache->p_mutex = klb_mutex_create();

    return 0;
}

void klua_kmcache_quit()
{
    assert(NULL != g_klua_kmcache);

    // 清空
    klua_kmcache_clear_all(g_klua_kmcache);

    KLB_FREE_BY(g_klua_kmcache->p_hlist, klb_hlist_destroy);
    KLB_FREE_BY(g_klua_kmcache->p_mutex, klb_mutex_destroy);
    KLB_FREE(g_klua_kmcache);
}


//////////////////////////////////////////////////////////////////////////
static void empty_klua_kmcache_item(klua_kmcache_item_t* p_item)
{
    switch (p_item->type)
    {
    case KLUA_KMCACHE_TSTRING:
        KLB_FREE_BY(p_item->str, sdsfree);
        break;
    }

    KLB_MEMSET(p_item, 0, sizeof(klua_kmcache_item_t));
}

static void free_klua_kmcache_item(klua_kmcache_item_t* p_item)
{
    empty_klua_kmcache_item(p_item);
    KLB_FREE(p_item);
}

static void klua_kmcache_clear_all(klua_kmcache_t* p_kmcache)
{
    while (0 < klb_hlist_size(p_kmcache->p_hlist))
    {
        klua_kmcache_item_t* p_item = (klua_kmcache_item_t*)klb_hlist_pop_head(p_kmcache->p_hlist);
        KLB_FREE_BY(p_item, free_klua_kmcache_item);
    }
}

static klua_kmcache_item_t* get_klua_kmcache_item(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len)
{
    klua_kmcache_item_t* p_item = (klua_kmcache_item_t*)klb_hlist_find(p_kmcache->p_hlist, p_key, key_len);
    if (NULL == p_item)
    {
        p_item = KLB_MALLOC(klua_kmcache_item_t, 1, 0);
        KLB_MEMSET(p_item, 0, sizeof(klua_kmcache_item_t));

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_kmcache->p_hlist, p_key, key_len, p_item);
        assert(NULL != p_iter);
    }

    assert(NULL != p_item);
    return p_item;
}

static void klua_kmcache_set_nil(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len)
{
    klua_kmcache_item_t* p_item = (klua_kmcache_item_t*)klb_hlist_remove_bykey(p_kmcache->p_hlist, p_key, key_len);
    KLB_FREE_BY(p_item, free_klua_kmcache_item);
}

static void klua_kmcache_set_boolean(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len, int b)
{
    klua_kmcache_item_t* p_item = get_klua_kmcache_item(p_kmcache, p_key, key_len);

    // 清空原来的数据
    empty_klua_kmcache_item(p_item);

    p_item->i = b;
    p_item->type = KLUA_KMCACHE_TBOOLEAN;
}

static void klua_kmcache_set_lightuserdata(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len, void* ptr)
{
    klua_kmcache_item_t* p_item = get_klua_kmcache_item(p_kmcache, p_key, key_len);

    // 清空原来的数据
    empty_klua_kmcache_item(p_item);

    p_item->ptr = ptr;
    p_item->type = KLUA_KMCACHE_TLIGHTUSERDATA;
}

static void klua_kmcache_set_number(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len, lua_Number num)
{
    klua_kmcache_item_t* p_item = get_klua_kmcache_item(p_kmcache, p_key, key_len);

    // 清空原来的数据
    empty_klua_kmcache_item(p_item);

    p_item->num = num;
    p_item->type = KLUA_KMCACHE_TNUMBER;
}

static void klua_kmcache_set_string(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len, const char* p_str, size_t str_len)
{
    klua_kmcache_item_t* p_item = get_klua_kmcache_item(p_kmcache, p_key, key_len);

    // 清空原来的数据
    empty_klua_kmcache_item(p_item);

    p_item->str = sdsnewlen(p_str, str_len);
    p_item->type = KLUA_KMCACHE_TSTRING;
}

static void klua_kmcache_set_integer(klua_kmcache_t* p_kmcache, const char* p_key, size_t key_len, lua_Integer i)
{
    klua_kmcache_item_t* p_item = get_klua_kmcache_item(p_kmcache, p_key, key_len);

    // 清空原来的数据
    empty_klua_kmcache_item(p_item);

    p_item->i = i;
    p_item->type = KLUA_KMCACHE_TINTEGER;
}


//////////////////////////////////////////////////////////////////////////


/// @brief 设置值
static int klua_kmcache_set(lua_State* L)
{
    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 1, &key_len);

    int type = lua_type(L, 2);
    switch (type)
    {
    case LUA_TBOOLEAN:
        {
            int b = lua_toboolean(L, 2);
            klb_mutex_lock(g_klua_kmcache->p_mutex);
            klua_kmcache_set_boolean(g_klua_kmcache, p_key, key_len, b);
            klb_mutex_unlock(g_klua_kmcache->p_mutex);
        }
        break;
    case LUA_TLIGHTUSERDATA:
        {
            void* ptr = lua_touserdata(L, 2);
            klb_mutex_lock(g_klua_kmcache->p_mutex);
            klua_kmcache_set_lightuserdata(g_klua_kmcache, p_key, key_len, ptr);
            klb_mutex_unlock(g_klua_kmcache->p_mutex);
        }
        break;
    case LUA_TNUMBER:
        {
            if (lua_isinteger(L, 2))
            {
                lua_Integer i = lua_tointeger(L, 2);
                klb_mutex_lock(g_klua_kmcache->p_mutex);
                klua_kmcache_set_integer(g_klua_kmcache, p_key, key_len, i);
                klb_mutex_unlock(g_klua_kmcache->p_mutex);
            }
            else
            {
                lua_Number num = lua_tonumber(L, 2);
                klb_mutex_lock(g_klua_kmcache->p_mutex);
                klua_kmcache_set_number(g_klua_kmcache, p_key, key_len, num);
                klb_mutex_unlock(g_klua_kmcache->p_mutex);
            }
        }
        break;
    case LUA_TSTRING:
        {
            size_t str_len = 0;
            const char* p_str = luaL_checklstring(L, 2, &str_len);
            klb_mutex_lock(g_klua_kmcache->p_mutex);
            klua_kmcache_set_string(g_klua_kmcache, p_key, key_len, p_str, str_len);
            klb_mutex_unlock(g_klua_kmcache->p_mutex);
        }
        break;
    default:
        {
            klb_mutex_lock(g_klua_kmcache->p_mutex);
            klua_kmcache_set_nil(g_klua_kmcache, p_key, key_len);
            klb_mutex_unlock(g_klua_kmcache->p_mutex);
        }
        break;
    }

    return 0;
}

/// @brief 获取值
static int klua_kmcache_get(lua_State* L)
{
    size_t key_len = 0;
    const char* p_key = luaL_checklstring(L, 1, &key_len);

    klb_mutex_lock(g_klua_kmcache->p_mutex);
    klua_kmcache_item_t* p_item = (klua_kmcache_item_t*)klb_hlist_find(g_klua_kmcache->p_hlist, p_key, key_len);

    if (NULL != p_item)
    {
        switch (p_item->type)
        {
        case KLUA_KMCACHE_TBOOLEAN:
            lua_pushboolean(L, (int)p_item->i);
            break;
        case KLUA_KMCACHE_TLIGHTUSERDATA:
            lua_pushlightuserdata(L, p_item->ptr);
            break;
        case KLUA_KMCACHE_TNUMBER:
            lua_pushnumber(L, p_item->num);
            break;
        case KLUA_KMCACHE_TSTRING:
            lua_pushlstring(L, p_item->str, sdslen(p_item->str));
            break;
        case KLUA_KMCACHE_TINTEGER:
            lua_pushinteger(L, p_item->i);
            break;
        default:
            lua_pushnil(L);
            break;
        }
    }
    else
    {
        lua_pushnil(L);
    }

    klb_mutex_unlock(g_klua_kmcache->p_mutex);
    return 1;
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
static int klua_kmcache_clear(lua_State* L)
{
    klb_mutex_lock(g_klua_kmcache->p_mutex);
    klua_kmcache_clear_all(g_klua_kmcache);
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
        { "clear",      klua_kmcache_clear },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
