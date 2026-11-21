// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
// kmuxlist: 专用跨线程队列, 仅传递 klb_netmux_socket_t* (内置 wrap/unwrap)
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klbnet/klblisten/klb_netmux.h"
#include "klbnet/klb_socket.h"
#include "klbutil/klb_obj.h"
#include "klbplatform/klb_atomic.h"
#include "klbutil/klb_nlist.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"
#include <assert.h>
#include <string.h>


#define KLUA_KMUXLIST_HANDLE    "KLUA_KMUXLIST_HANDLE*"


/// @struct klua_kmuxlist_sock_payload_t
/// @brief  kmuxlist 队列元素载荷 (对齐 klb_netmux_socket_t 移交字段)
typedef struct klua_kmuxlist_sock_payload_t_
{
    klb_socket_fd               fd;             ///< socket fd
    struct sockaddr_in          addr;           ///< 对端 IPv4 (兼容)
    struct sockaddr_storage     addr_sa;        ///< 完整对端地址
    bool                        tls;            ///< 是否 TLS ClientHello
    int                         protocol;       ///< klb_protocol_e
    int                         listen_port;    ///< 本 mux 监听端口 (主机序)
}klua_kmuxlist_sock_payload_t;


/// @struct klua_kmuxlist_item_t
/// @brief  单 path 队列
typedef struct klua_kmuxlist_item_t_
{
    klb_atomic_t volatile       lock_list;      ///< p_list 锁
    klb_nlist_t*                p_list;         ///< 存 klb_obj_t*
    uint32_t                    max_size;       ///< 最大积压; 0=不限; 满则丢弃队头未消费项
}klua_kmuxlist_item_t;


/// @struct klua_kmuxlist_t
/// @brief  进程内 kmuxlist 总表 (path → 队列)
typedef struct klua_kmuxlist_t_
{
    klb_atomic_t volatile       lock_hlist;     ///< p_hlist 锁
    klb_hlist_t*                p_hlist;        ///< klua_kmuxlist_item_t*
}klua_kmuxlist_t;


#define lock_kmuxlist(PTR_)         { klb_atomic_lock(&((PTR_)->lock_hlist)); }
#define unlock_kmuxlist(PTR_)       { klb_atomic_unlock(&((PTR_)->lock_hlist)); }

#define lock_kmuxlist_item(PTR_)    { klb_atomic_lock(&((PTR_)->lock_list)); }
#define unlock_kmuxlist_item(PTR_)  { klb_atomic_unlock(&((PTR_)->lock_list)); }


static klua_kmuxlist_t* g_klua_kmuxlist = NULL;

static const char* g_kmuxlist_obj_module = "kmuxlist";
static const char* g_kmuxlist_obj_name = "sock";


//////////////////////////////////////////////////////////////////////////
// sock 信封


/// @brief 队列元素销毁: 固定载荷 klua_kmuxlist_sock_payload_t, 必须关 socket fd
/// @note clear / quit / 未 unwrap 的丢弃均走此回调; unwrap 成功后 fd 已置 INVALID_SOCKET, 此处不再关
static void destroy_sock_obj_klua_kmuxlist(klb_obj_t* p_obj)
{
    klua_kmuxlist_sock_payload_t* p_pay = NULL;

    assert(NULL != p_obj);

    p_pay = (klua_kmuxlist_sock_payload_t*)p_obj->extra;

    // 固定类型清理要点: 持有系统 fd 时须关闭, 避免泄漏
    if (INVALID_SOCKET != p_pay->fd)
    {
        KLB_SOCKET_CLOSE(p_pay->fd);
    }

    KLB_FREE(p_obj);
}


static klb_obj_t* wrap_sock_klua_kmuxlist(klb_netmux_socket_t* p_sock)
{
    klb_obj_t* p_obj = NULL;
    klua_kmuxlist_sock_payload_t* p_pay = NULL;

    if (NULL == p_sock)
    {
        return NULL;
    }

    p_obj = KLB_MALLOCZ(klb_obj_t, 1, sizeof(klua_kmuxlist_sock_payload_t));
    p_obj->p_module = g_kmuxlist_obj_module;
    p_obj->p_name = g_kmuxlist_obj_name;
    p_obj->cb_destroy = destroy_sock_obj_klua_kmuxlist;

    p_pay = (klua_kmuxlist_sock_payload_t*)p_obj->extra;
    p_pay->fd = p_sock->fd;
    p_pay->addr = p_sock->addr;
    p_pay->addr_sa = p_sock->addr_sa;
    p_pay->tls = p_sock->tls;
    p_pay->protocol = p_sock->protocol;
    p_pay->listen_port = p_sock->listen_port;

    // 节点释放; fd 已迁入 obj
    p_sock->fd = INVALID_SOCKET;
    KLB_FREE(p_sock);

    return p_obj;
}


static bool is_sock_obj_klua_kmuxlist(klb_obj_t* p_obj)
{
    if (NULL == p_obj)
    {
        return false;
    }

    if (destroy_sock_obj_klua_kmuxlist != p_obj->cb_destroy)
    {
        return false;
    }

    if (NULL == p_obj->p_module || 0 != strcmp(p_obj->p_module, g_kmuxlist_obj_module))
    {
        return false;
    }

    if (NULL == p_obj->p_name || 0 != strcmp(p_obj->p_name, g_kmuxlist_obj_name))
    {
        return false;
    }

    return true;
}


static klb_netmux_socket_t* unwrap_sock_klua_kmuxlist(klb_obj_t* p_obj)
{
    klua_kmuxlist_sock_payload_t* p_pay = NULL;
    klb_netmux_socket_t* p_sock = NULL;

    if (!is_sock_obj_klua_kmuxlist(p_obj))
    {
        return NULL;
    }

    p_pay = (klua_kmuxlist_sock_payload_t*)p_obj->extra;
    if (INVALID_SOCKET == p_pay->fd)
    {
        KLB_OBJ_DESTROY(p_obj);
        return NULL;
    }

    p_sock = KLB_MALLOCZ(klb_netmux_socket_t, 1, 0);
    p_sock->fd = p_pay->fd;
    p_sock->addr = p_pay->addr;
    p_sock->addr_sa = p_pay->addr_sa;
    p_sock->tls = p_pay->tls;
    p_sock->protocol = p_pay->protocol;
    p_sock->err_code = 0;
    p_sock->listen_port = p_pay->listen_port;

    p_pay->fd = INVALID_SOCKET;
    KLB_OBJ_DESTROY(p_obj);

    return p_sock;
}


//////////////////////////////////////////////////////////////////////////
// init / quit


int klua_kmuxlist_init()
{
    assert(NULL == g_klua_kmuxlist);

    g_klua_kmuxlist = KLB_MALLOCZ(klua_kmuxlist_t, 1, 0);
    klb_atomic_set_zero(&g_klua_kmuxlist->lock_hlist);
    g_klua_kmuxlist->p_hlist = klb_hlist_create(0);

    return 0;
}


void klua_kmuxlist_quit()
{
    assert(NULL != g_klua_kmuxlist);

    while (0 < klb_hlist_size(g_klua_kmuxlist->p_hlist))
    {
        klua_kmuxlist_item_t* p_item = (klua_kmuxlist_item_t*)klb_hlist_pop_head(g_klua_kmuxlist->p_hlist);

        while (0 < klb_nlist_size(p_item->p_list))
        {
            klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_item->p_list);
            klb_obj_destroy_cb cb_destroy = p_obj->cb_destroy;

            assert(NULL != cb_destroy);
            cb_destroy(p_obj);
        }

        KLB_FREE_BY(p_item->p_list, klb_nlist_destroy);
        KLB_FREE(p_item);
    }

    KLB_FREE_BY(g_klua_kmuxlist->p_hlist, klb_hlist_destroy);
    KLB_FREE(g_klua_kmuxlist);
}


//////////////////////////////////////////////////////////////////////////


static klua_kmuxlist_item_t* get_insert_item_klua_kmuxlist(klua_kmuxlist_t* ptr, const char* p_key, int key_len)
{
    klua_kmuxlist_item_t* p_item = (klua_kmuxlist_item_t*)klb_hlist_find(ptr->p_hlist, p_key, key_len);

    if (NULL == p_item)
    {
        p_item = KLB_MALLOCZ(klua_kmuxlist_item_t, 1, 0);
        klb_atomic_set_zero(&p_item->lock_list);
        p_item->p_list = klb_nlist_create();
        p_item->max_size = 0;               // 默认不限
        klb_hlist_push_tail(ptr->p_hlist, p_key, key_len, p_item);
    }

    return p_item;
}


/// @brief 解析 new 可选配置表: max / max_size
/// @param [in]  idx            Lua 表下标
/// @param [out] *p_max_size    最大积压; 未给则不改 (*p_set=false)
/// @param [out] *p_set         是否写了 max
static void parse_opts_klua_kmuxlist(lua_State* L, int idx, uint32_t* p_max_size, bool* p_set)
{
    assert(NULL != p_max_size);
    assert(NULL != p_set);

    *p_set = false;

    if (LUA_TTABLE != lua_type(L, idx))
    {
        return;
    }

    lua_getfield(L, idx, "max");
    if (LUA_TNIL == lua_type(L, -1))
    {
        lua_pop(L, 1);
        lua_getfield(L, idx, "max_size");
    }

    if (LUA_TNUMBER == lua_type(L, -1))
    {
        lua_Integer n = lua_tointeger(L, -1);

        if (0 > n)
        {
            n = 0;
        }

        *p_max_size = (uint32_t)n;
        *p_set = true;
    }

    lua_pop(L, 1);
}


/// @brief 在 item 锁内: 超 max 则弹出队头到 p_drop; 再 push 新元素
/// @note 调用方持有 item 锁; destroy 在锁外做
static void push_capped_klua_kmuxlist(klua_kmuxlist_item_t* p_item, klb_obj_t* p_obj, klb_nlist_t* p_drop)
{
    assert(NULL != p_item);
    assert(NULL != p_obj);
    assert(NULL != p_drop);

    if (0 != p_item->max_size)
    {
        while (p_item->max_size <= klb_nlist_size(p_item->p_list))
        {
            klb_obj_t* p_old = (klb_obj_t*)klb_nlist_pop_head(p_item->p_list);

            if (NULL == p_old)
            {
                break;
            }

            klb_nlist_push_tail(p_drop, p_old);
        }
    }

    klb_nlist_push_tail(p_item->p_list, p_obj);
}


/// @brief 销毁 drop 列表中的 sock 信封 (关 fd)
static void destroy_drop_list_klua_kmuxlist(klb_nlist_t* p_drop)
{
    assert(NULL != p_drop);

    while (0 < klb_nlist_size(p_drop))
    {
        klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_drop);
        klb_obj_destroy_cb cb_destroy = p_obj->cb_destroy;

        assert(NULL != cb_destroy);
        cb_destroy(p_obj);
    }
}


typedef struct klua_kmuxlist_userdata_t_
{
    klua_kmuxlist_item_t*       p_item;         ///< 共享队列项
    klb_nlist_t*                p_tmp_list;     ///< clear 时暂存
}klua_kmuxlist_userdata_t;


static klua_kmuxlist_userdata_t* new_ud_klua_kmuxlist(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = (klua_kmuxlist_userdata_t*)lua_newuserdata(L, sizeof(klua_kmuxlist_userdata_t));

    KLB_MEMSET(p_ud, 0, sizeof(klua_kmuxlist_userdata_t));
    luaL_setmetatable(L, KLUA_KMUXLIST_HANDLE);

    return p_ud;
}


static klua_kmuxlist_userdata_t* to_ud_klua_kmuxlist(lua_State* L, int idx)
{
    klua_kmuxlist_userdata_t* p_ud = (klua_kmuxlist_userdata_t*)luaL_checkudata(L, idx, KLUA_KMUXLIST_HANDLE);

    luaL_argcheck(L, NULL != p_ud, idx, "'kmuxlist' expected");

    return p_ud;
}


static int klua_kmuxlist_tostring(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);

    lua_pushfstring(L, "kmuxlist:%p", p_ud);

    return 1;
}


static int klua_kmuxlist_gc(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);

    p_ud->p_item = NULL;
    KLB_FREE_BY(p_ud->p_tmp_list, klb_nlist_destroy);

    return 0;
}


/// @brief :push(sock)  sock=klb_netmux_socket_t* lightuserdata; 内置 wrap
/// @note 若配置了 max 且已满: 先销毁队头未消费项再入队
static int klua_kmuxlist_push(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)luaL_checklightuserdata(L, 2);
    klb_obj_t* p_obj = NULL;

    p_obj = wrap_sock_klua_kmuxlist(p_sock);
    if (NULL == p_obj)
    {
        return 0;
    }

    lock_kmuxlist_item(p_ud->p_item);
    push_capped_klua_kmuxlist(p_ud->p_item, p_obj, p_ud->p_tmp_list);
    unlock_kmuxlist_item(p_ud->p_item);

    destroy_drop_list_klua_kmuxlist(p_ud->p_tmp_list);

    return 0;
}


/// @brief :pop() → sock lightuserdata | nil; 内置 unwrap
static int klua_kmuxlist_pop(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);
    klb_obj_t* p_obj = NULL;
    klb_netmux_socket_t* p_sock = NULL;

    lock_kmuxlist_item(p_ud->p_item);
    if (0 < klb_nlist_size(p_ud->p_item->p_list))
    {
        p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_item->p_list);
    }
    unlock_kmuxlist_item(p_ud->p_item);

    p_sock = unwrap_sock_klua_kmuxlist(p_obj);
    if (NULL != p_sock)
    {
        lua_pushlightuserdata(L, p_sock);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}


static int klua_kmuxlist_size(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);
    uint32_t size = 0;

    lock_kmuxlist_item(p_ud->p_item);
    size = klb_nlist_size(p_ud->p_item->p_list);
    unlock_kmuxlist_item(p_ud->p_item);

    lua_pushinteger(L, (lua_Integer)size);

    return 1;
}


static int klua_kmuxlist_clear(lua_State* L)
{
    klua_kmuxlist_userdata_t* p_ud = to_ud_klua_kmuxlist(L, 1);

    lock_kmuxlist_item(p_ud->p_item);

    while (0 < klb_nlist_size(p_ud->p_item->p_list))
    {
        klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_item->p_list);

        klb_nlist_push_tail(p_ud->p_tmp_list, p_obj);
    }

    unlock_kmuxlist_item(p_ud->p_item);

    while (0 < klb_nlist_size(p_ud->p_tmp_list))
    {
        klb_obj_t* p_obj = (klb_obj_t*)klb_nlist_pop_head(p_ud->p_tmp_list);
        klb_obj_destroy_cb cb_destroy = p_obj->cb_destroy;

        assert(NULL != cb_destroy);
        cb_destroy(p_obj);
    }

    return 0;
}


static void klua_kmuxlist_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "push",           klua_kmuxlist_push },
        { "pop",            klua_kmuxlist_pop },
        { "size",           klua_kmuxlist_size },
        { "clear",          klua_kmuxlist_clear },
        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",        NULL },
        { "__gc",           klua_kmuxlist_gc },
        { "__close",        klua_kmuxlist_gc },
        { "__tostring",     klua_kmuxlist_tostring },
        { NULL,             NULL }
    };

    luaL_newmetatable(L, KLUA_KMUXLIST_HANDLE);
    luaL_setfuncs(L, metameth, 0);
    luaL_newlibtable(L, meth);
    luaL_setfuncs(L, meth, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
}


//////////////////////////////////////////////////////////////////////////
// lib


/// @brief kmuxlist.new(path [, opts])
/// @param opts[table]  [可选] { max=N } 或 { max_size=N }; 0/缺省=不限
/// @note 满时 push 自动销毁队头未消费 sock (关 fd)
static int lib_klua_kmuxlist_new(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);
    klua_kmuxlist_item_t* p_item = NULL;
    klua_kmuxlist_userdata_t* p_ud = NULL;
    uint32_t max_size = 0;
    bool max_set = false;

    parse_opts_klua_kmuxlist(L, 2, &max_size, &max_set);

    lock_kmuxlist(g_klua_kmuxlist);
    p_item = get_insert_item_klua_kmuxlist(g_klua_kmuxlist, p_path, (int)path_len);
    if (max_set)
    {
        p_item->max_size = max_size;
    }
    unlock_kmuxlist(g_klua_kmuxlist);

    p_ud = new_ud_klua_kmuxlist(L);
    p_ud->p_item = p_item;
    p_ud->p_tmp_list = klb_nlist_create();

    return 1;
}


/// @brief kmuxlist.push(path, sock)  sock=klb_netmux_socket_t*
static int lib_klua_kmuxlist_push(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);
    klb_netmux_socket_t* p_sock = (klb_netmux_socket_t*)luaL_checklightuserdata(L, 2);
    klb_obj_t* p_obj = NULL;
    klua_kmuxlist_item_t* p_item = NULL;
    klb_nlist_t* p_drop = NULL;

    p_obj = wrap_sock_klua_kmuxlist(p_sock);
    if (NULL == p_obj)
    {
        return 0;
    }

    p_drop = klb_nlist_create();

    lock_kmuxlist(g_klua_kmuxlist);
    p_item = get_insert_item_klua_kmuxlist(g_klua_kmuxlist, p_path, (int)path_len);
    lock_kmuxlist_item(p_item);
    push_capped_klua_kmuxlist(p_item, p_obj, p_drop);
    unlock_kmuxlist_item(p_item);
    unlock_kmuxlist(g_klua_kmuxlist);

    destroy_drop_list_klua_kmuxlist(p_drop);
    KLB_FREE_BY(p_drop, klb_nlist_destroy);

    return 0;
}


/// @brief kmuxlist.pop(path) → sock | nil
static int lib_klua_kmuxlist_pop(lua_State* L)
{
    size_t path_len = 0;
    const char* p_path = luaL_checklstring(L, 1, &path_len);
    klb_obj_t* p_obj = NULL;
    klb_netmux_socket_t* p_sock = NULL;
    klua_kmuxlist_item_t* p_item = NULL;

    lock_kmuxlist(g_klua_kmuxlist);
    p_item = (klua_kmuxlist_item_t*)klb_hlist_find(g_klua_kmuxlist->p_hlist, p_path, (int)path_len);
    if (NULL != p_item)
    {
        lock_kmuxlist_item(p_item);
        if (0 < klb_nlist_size(p_item->p_list))
        {
            p_obj = (klb_obj_t*)klb_nlist_pop_head(p_item->p_list);
        }
        unlock_kmuxlist_item(p_item);
    }
    unlock_kmuxlist(g_klua_kmuxlist);

    p_sock = unwrap_sock_klua_kmuxlist(p_obj);
    if (NULL != p_sock)
    {
        lua_pushlightuserdata(L, p_sock);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

// "kmuxlist"
int klua_open_kmuxlist(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "new",            lib_klua_kmuxlist_new },
        { "push",           lib_klua_kmuxlist_push },
        { "pop",            lib_klua_kmuxlist_pop },
        { NULL,             NULL }
    };

    luaL_newlib(L, lib);
    klua_kmuxlist_createmeta(L);

    return 1;
}

// end
