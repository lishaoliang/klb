// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbmem/klb_mem.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/klua_netmulti.h"
#include "klua/klua_coroutine.h"
#include "klbrtsp/klbrtsp.h"
#include "klbrtsp/rtsp_klua/klbrtsp_serve_klua.h"
#include "klbutil/klb_nlist.h"
#include "klbnet/klb_netmulti.h"
#include "klbnet/klb_netcode.h"
#include "klbrtsp/core/klb_rtspclient_conn.h"
#include "klbbase/klb_mnp.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// rtsp client

#define KLUA_KRTSPCLIENT_HANDLE                 "KRTSPCLIENT_HANDLE*"       ///< Lua meta标示


/// @brief 失败: 压入 nil, code; 描述串由脚本 knetcode.strerror 查询
static int push_err_klua_krtsp(lua_State* L, int code)
{
    lua_pushnil(L);
    lua_pushinteger(L, code);
    return 2;
}


typedef struct klua_krtspclient_t_
{
    // Lua相关
    struct
    {
        lua_State*              L;              ///< L
        lua_State*              co_recv;        ///< co_recv协程

        klua_env_t*             p_env;          ///< lua环境
        klua_ex_coroutine_t*    p_coex;         ///< Lua协程扩展
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

        bool                    closed;         ///< 连接已断开或协议错误
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

static int call_co_recv_exit_klua_krtspclient(klua_krtspclient_t* p_client);

static int klua_krtspclient_close(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    klb_netconn_t* p_rtsp_conn = p_client->p_rtsp_conn;

    // 先摘除连接, 防止 co_recv 唤醒过程中重入 disconnect 重复关闭
    p_client->p_rtsp_conn = NULL;
    p_client->closed = true;

    // 唤醒挂起的 co_recv, 避免协程扩展销毁时残留
    call_co_recv_exit_klua_krtspclient(p_client);

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

    // 托管关闭: 交由 netmulti 在事件循环安全点销毁;
    // disconnect 可能在网络回调栈内被调用, 直接销毁会释放仍在使用的读缓存(UAF)
    if (NULL != p_rtsp_conn)
    {
        if (NULL != p_client->p_netmulti)
        {
            klb_netmulti_closing(p_client->p_netmulti, p_rtsp_conn);
        }
        else
        {
            klb_netconn_destroy(p_rtsp_conn);
        }
    }

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


static int call_co_recv_exit_klua_krtspclient(klua_krtspclient_t* p_client)
{
    if (NULL == p_client->co_recv)
    {
        return -1;
    }

    lua_State* L = klua_coroutine_rawgeti(klua_coroutine_get(p_client->p_env), p_client->co_recv);

    if (NULL == L)
    {
        return -1;
    }

    p_client->co_recv = NULL;
    lua_pushstring(L, "exit");
    lua_pushnil(L);

    int status = lua_pcall(L, 2, 0, 0);
    klua_env_report_by_L(L, status);

    return (status == LUA_OK) ? 0 : 1;
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
    else if (KLB_SOCKET_CONNECT == code || KLB_NETCODE_WBUF_EMPTY == code)
    {
        KLB_FREE_BY(p_data, klb_buf_unref_next);
    }
    else
    {
        p_client->closed = true;
        call_co_recv_exit_klua_krtspclient(p_client);
        KLB_FREE_BY(p_data, klb_buf_unref_next);
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

static int klua_krtspclient_send_media(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);   ///< @1. self
    klb_buf_t* p_buf = lua_touserdata(L, 2);                    ///< @2. 媒体数据

    int ret = 1;

    if (NULL != p_client->p_rtsp_conn && NULL != p_buf)
    {
        ret = klb_netconn_send_media(p_client->p_rtsp_conn, p_buf);
    }

    lua_pushinteger(L, ret);
    return 1;
}

static int klua_krtspclient_set_video_pt(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    int pt = (int)luaL_checkinteger(L, 2);

    if (NULL != p_client->p_rtsp_conn)
    {
        klb_rtspclient_conn_set_video_pt(p_client->p_rtsp_conn, pt);
    }

    return 0;
}


static int klua_krtspclient_set_audio_pt(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    int pt = (int)luaL_checkinteger(L, 2);

    if (NULL != p_client->p_rtsp_conn)
    {
        klb_rtspclient_conn_set_audio_pt(p_client->p_rtsp_conn, pt);
    }

    return 0;
}


static int klua_krtspclient_set_audio_info(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    int samples = (int)luaL_checkinteger(L, 2);
    int tracks = (int)luaL_optinteger(L, 3, 2);
    int bits = (int)luaL_optinteger(L, 4, 2);

    if (NULL != p_client->p_rtsp_conn)
    {
        klb_rtspclient_conn_set_audio_info(p_client->p_rtsp_conn, samples, tracks, bits);
    }

    return 0;
}


static int klua_krtspclient_set_timeout(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    int read_ms = (int)luaL_optinteger(L, 2, 0);
    int write_ms = (int)luaL_optinteger(L, 3, 0);
    int idle_ms = (int)luaL_optinteger(L, 4, 0);

    if (NULL != p_client->p_rtsp_conn)
    {
        klb_rtspclient_conn_set_timeout(p_client->p_rtsp_conn, read_ms, write_ms, idle_ms);
    }

    return 0;
}


static int klua_krtspclient_open_udp(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    int port = 0;
    int ret = 1;

    if (NULL != p_client->p_rtsp_conn)
    {
        ret = klb_rtspclient_conn_open_udp(p_client->p_rtsp_conn, &port);
    }

    if (0 != ret)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushinteger(L, port);
    return 1;
}


static int klua_krtspclient_close_udp(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);

    if (NULL != p_client->p_rtsp_conn)
    {
        klb_rtspclient_conn_close_udp(p_client->p_rtsp_conn);
    }

    return 0;
}


static int klua_krtspclient_udp_punch(lua_State* L)
{
    klua_krtspclient_t* p_client = to_klua_krtspclient(L, 1);
    const char* p_host = luaL_checkstring(L, 2);
    int port = (int)luaL_checkinteger(L, 3);
    int ret = 1;

    if (NULL != p_client->p_rtsp_conn)
    {
        ret = klb_rtspclient_conn_udp_punch(p_client->p_rtsp_conn, p_host, port);
    }

    lua_pushboolean(L, 0 == ret);
    return 1;
}


static int on_yield_recv_klua_krtspclient(void* ptr, klua_ex_coroutine_t* p_ex, lua_State* p_co, int opt)
{
    klua_krtspclient_t* p_client = (klua_krtspclient_t*)ptr;

    if (KLUA_ENV_EX_quit == opt)
    {
        call_co_recv_exit_klua_krtspclient(p_client);
    }

    return 0;
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

    if (p_client->closed)
    {
        lua_pushstring(L, "exit");
        lua_pushnil(L);
        return 2;
    }

    p_client->co_recv = L;
    return klua_coroutine_yield(p_client->p_coex, L, on_yield_recv_klua_krtspclient, p_client);
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
        { "send_media",     klua_krtspclient_send_media },
        { "set_video_pt",   klua_krtspclient_set_video_pt },
        { "set_audio_pt",   klua_krtspclient_set_audio_pt },
        { "set_audio_info", klua_krtspclient_set_audio_info },
        { "set_timeout",    klua_krtspclient_set_timeout },    ///< 设置读写/空闲超时(毫秒)
        { "open_udp",       klua_krtspclient_open_udp },
        { "close_udp",      klua_krtspclient_close_udp },
        { "udp_punch",      klua_krtspclient_udp_punch },

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
    int code = 0;
    klua_env_t* p_env = klua_env_get_by_L(L);
    klb_netmulti_t* p_netmulti = klua_netmulti_get(p_env);
    klb_netconn_t* p_rtsp_conn = NULL;
    klua_krtspclient_t* p_client = NULL;

    if (NULL == p_netmulti)
    {
        return push_err_klua_krtsp(L, KLB_NETCODE_EINVAL);
    }

    p_rtsp_conn = klb_rtspclient_connect(p_netmulti, p_host, port, &code);
    if (NULL == p_rtsp_conn)
    {
        return push_err_klua_krtsp(L, (0 != code) ? code : KLB_NETCODE_CONNECT_FAIL);
    }

    p_client = new_klua_krtspclient(L);   ///< #1. meta: klua_krtspclient_createmeta

    p_client->L = L;
    p_client->co_recv = NULL;
    p_client->p_env = p_env;
    p_client->p_coex = klua_coroutine_get(p_env);

    p_client->p_netmulti = p_netmulti;
    p_client->p_rtsp_conn = p_rtsp_conn;

    p_client->p_text_nlist = klb_nlist_create();
    p_client->p_media_nlist = klb_nlist_create();

    // 绑定数据接收
    p_rtsp_conn->p_udata = p_client;
    klb_netconn_bind_recv_data(p_rtsp_conn, on_recv_data_klua_krtspclient);

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
