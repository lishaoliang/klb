// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klua/klua_net/klua_krtsp_serve.h"
#include "klbutil/klb_nlist.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klbrtsp/klb_rtspclient_conn.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// rtsp client

#define KLUA_KRTSPCLIENT_HANDLE                 "KRTSPCLIENT_HANDLE*"       ///< Lua meta标示


typedef struct klua_krtspclient_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
    };

    // 网络相关
    struct
    {
        klb_netmulti_t*         p_netmulti;     ///< 复用; net multiplex
        klb_netconn_t*          p_rtsp_conn;    ///< rtsp 客户端连接
    };

    // 数据
    struct
    {
        klb_nlist_t*            p_text_nlist;   ///< 待读取的 文本数据列表; 存储 klb_buf_t*
        klb_nlist_t*            p_media_nlist;  ///< 待读取的 媒体列表; 存储 klb_buf_t*
    };
}klua_krtspclient_t;


////////////////////////////////////////
static klua_krtspclient_t* new_klua_krtspclient(lua_State* L)
{
    klua_krtspclient_t* p_client = (klua_krtspclient_t*)lua_newuserdata(L, sizeof(klua_krtspclient_t));
    KLB_MEMSET(p_client, 0, sizeof(klua_krtspclient_t));
    luaL_setmetatable(L, KLUA_KRTSPCLIENT_HANDLE);
    return p_client;
}

static klua_krtspclient_t* to_klua_krtspclient(lua_State* L, int index)
{
    klua_krtspclient_t* p_client = (klua_krtspclient_t*)luaL_checkudata(L, index, KLUA_KRTSPCLIENT_HANDLE);
    luaL_argcheck(L, NULL != p_client, index, "'krtspclient' expected");
    return p_client;
}

static int klua_krtspclient_tostring(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);

    lua_pushfstring(L, "krtspclient:%p", p_client);
    return 1;
}

static int klua_krtspclient_close(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);

    // 清空 文本
    if (NULL != p_client->p_text_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_text_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }
    }

    // 清空视频
    if (NULL != p_client->p_media_nlist)
    {
        while (0 < klb_nlist_size(p_client->p_media_nlist))
        {
            klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_media_nlist);
            KLB_FREE_BY(p_txt, klb_buf_unref_next);
        }
    }

    KLB_FREE_BY(p_client->p_text_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_media_nlist, klb_nlist_destroy);
    KLB_FREE_BY(p_client->p_rtsp_conn, klb_netconn_destroy);

    return 0;
}

////////////////////////////////////////

// 调用Lua协程
static int call_co_recv_text_klua_krtspclient(klua_krtspclient_t* p_client)
{
    assert(NULL != p_client);

    if ( NULL != p_client->co_recv && 
        (0 < klb_nlist_size(p_client->p_text_nlist) || 0 < klb_nlist_size(p_client->p_media_nlist)) )
    {
        lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);
        if (NULL == L) return -1; // 未处理

        p_client->co_recv = NULL; // 清空

        // Fixed Bug. [2025] 当调用 lua_pcall 函数之后, 函数执行到 Lua 层
        // 在 Lua 可能会依然调用 co_recv(klua_krtspclient_co_recv) 函数; 这里会存在执行函数的交替执行

        klb_buf_t* p_txt = klb_nlist_pop_head(p_client->p_text_nlist);
        if (NULL != p_txt)
        {
            lua_pushstring(L, "text"); // 
            lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // body
            KLB_FREE_BY(p_txt, klb_buf_unref);
        }

        klb_buf_t* p_media = klb_nlist_pop_head(p_client->p_media_nlist);
        if (NULL != p_media)
        {
            lua_pushstring(L, "media"); // 
            lua_pushlightuserdata(L, p_media);
        }

        int status = lua_pcall(L, 2, 0, 0);                   /* do the call */
        klua_env_report_by_L(L, status);

        return (status == LUA_OK) ? 0 : 1;
    }

    return -1; // 未处理
}

// 数据接收, 当网络上有数据包之后触发
static int on_recv_data_klua_krtspclient(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data)
{
    klua_krtspclient_t* p_client = p_conn->p_udata;

    if (0 == code)
    {
        if (KLB_MNP_TEXT == packtype)
        {
            klb_nlist_push_tail(p_client->p_text_nlist, p_data);

            call_co_recv_text_klua_krtspclient(p_client);
        }
        else if(KLB_MNP_MEDIA == packtype)
        {
            klb_nlist_push_tail(p_client->p_media_nlist, p_data);

            call_co_recv_text_klua_krtspclient(p_client);
        }
        else
        {
            KLB_FREE_BY(p_data, klb_buf_unref_next);
        }
    }

    return 0;
}

////////////////////////////////////////

static int klua_krtspclient_send_text(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);  ///< @1. self

    size_t head_len = 0;
    const char* p_head = luaL_checklstring(L, 2, &head_len);    ///< @2. text head

    int ret = 1;

    if (NULL != p_client->p_rtsp_conn)
    {
        ret = klb_netconn_send_text(p_client->p_rtsp_conn, 0, (const uint8_t*)p_head, head_len, NULL, 0);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_krtspclient_co_recv(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);  ///< @1. self
    klua_check_coroutine(L, "krtspclient:co_recv must in coroutine!");
    assert(NULL == p_client->co_recv);

    klb_buf_t* p_txt = klb_nlist_head(p_client->p_text_nlist);
    if (NULL != p_txt)
    {
        lua_pushstring(L, "text"); // "text"
        lua_pushlstring(L, p_txt->p_buf + p_txt->start, p_txt->end - p_txt->start); // body

        klb_nlist_pop_head(p_client->p_text_nlist);
        KLB_FREE_BY(p_txt, klb_buf_unref_next);

        return 2;
    }

    klb_buf_t* p_media = klb_nlist_head(p_client->p_media_nlist);
    if (NULL != p_media)
    {
        lua_pushstring(L, "media"); // "media"
        lua_pushlightuserdata(L, p_media); // C 指针

        klb_nlist_pop_head(p_client->p_media_nlist);

        return 2;
    }

    p_client->co_recv = L;
    return lua_yield(L, lua_gettop(L));
}

static int klua_krtspclient_free_media(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);  ///< @1. self
    klb_buf_t* p_media = lua_touserdata(L, 2);

    KLB_FREE_BY(p_media, klb_buf_unref_next);

    return 0;
}

static int klua_krtspclient_dump_media(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);  ///< @1. self
    klb_buf_t* p_buf = lua_touserdata(L, 2);

    if (NULL != p_buf)
    {
        klb_mnp_media_t* p_media = (klb_mnp_media_t*)(p_buf->p_buf + p_buf->start);

        printf("media : size[%d], dtype:[%d], vtype:[%d], time:[%lld]\n", p_media->size, p_media->dtype, p_media->vtype,
            p_media->time);
    }
    else
    {
        printf("media : NULL\n");
    }

    return 0;
}

////////////////////////////////////////

static void klua_krtspclient_createmeta(lua_State* L)
{
    static luaL_Reg meth[] = {
        { "disconnect",     klua_krtspclient_close },

        { "send_text",      klua_krtspclient_send_text },

        { "co_recv",        klua_krtspclient_co_recv },

        { "free_media",     klua_krtspclient_free_media },
        { "dump_media",     klua_krtspclient_dump_media },

        { NULL,             NULL }
    };

    static const luaL_Reg metameth[] = {
        { "__index",         NULL },  /* place holder */
        { "__gc",            klua_krtspclient_close },
        { "__close",         klua_krtspclient_close },
        { "__tostring",      klua_krtspclient_tostring },
        { NULL,              NULL }
    };

    luaL_newmetatable(L, KLUA_KRTSPCLIENT_HANDLE);/* metatable for KLUA_KRTSP_HANDLE handles */
    luaL_setfuncs(L, metameth, 0);          /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);              /* create method table */
    luaL_setfuncs(L, meth, 0);              /* add file methods to method table */
    lua_setfield(L, -2, "__index");         /* metatable.__index = method table */
    lua_pop(L, 1);                          /* pop metatable */
}


////////////////////////////////////////
// rtsp client

static int klua_krtspclient_connect(lua_State* L)
{
    const char* p_host = luaL_checkstring(L, 1);                    ///< @1. 目标域名或IP地址
    int port = (int)luaL_checkinteger(L, 2);                        ///< @2. 端口号

    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);

    klb_netconn_t* p_rtsp_conn = klb_rtspclient_connect(p_netmulti, p_host, port);

    if (NULL == p_rtsp_conn)
    {
        lua_pushnil(L);                                             ///< #1. nil
    }
    else
    {
        // 
        klua_krtspclient_t* p_client = new_klua_krtspclient(L);   ///< #1. meta: klua_krtspclient_createmeta

        p_client->L = L;
        p_client->co_recv = NULL;
        p_client->p_env = p_env;

        p_client->p_netmulti = p_netmulti;
        p_client->p_rtsp_conn = p_rtsp_conn;

        p_client->p_text_nlist = klb_nlist_create();
        p_client->p_media_nlist = klb_nlist_create();

        // 绑定数据接收
        p_rtsp_conn->p_udata = p_client;
        klb_netconn_bind_recv_data(p_rtsp_conn, on_recv_data_klua_krtspclient);
    }

    return 1;
}

//////////////////////////////////////////////////////////////////////////

// rtsp
int klua_open_krtsp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "connect",            klua_krtspclient_connect },        ///< 客户端 发起连接

        { "listen",             klua_krtspserve_listen },           ///< 服务端 监听

        { "new_serve",          klua_krtspserve_new_serve },        ///< 新建服务连接

        { NULL,                 NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    // k rtsp client
    klua_krtspclient_createmeta(L);

    // k rtsp serve
    klua_krtspserve_createmeta(L);
    
    // k rtsp serve listen
    klua_krtspserve_listen_createmeta(L);

    return 1;
}

//end
