#include "klua/klua.h"
#include "socket/klb_socket.h"
#include "socket/klb_socket_base.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "mem/klb_mem.h"
#include "mem/klb_buf.h"
#include "mem/klb_buffer.h"
#include "parser/http_parser.h"
#include "list/klb_list.h"
#include "log/klb_log.h"
#include "string/sds.h"
#include <assert.h>


#define KLUA_KWEBSOCKET_HANDLE     "KLUA_KWEBSOCKET_HANDLE*"


typedef struct klua_kwebsocket_t_
{
    // Lua相关
    struct
    {
        klua_env_t*             p_env;
        klua_ex_multiplex_t*    p_multiplex;

        int                     reg_on_recv;        ///< Lua脚本函数
    };

    // socket相关
    struct
    {
        klb_socket_t*           p_socket;
        int                     id;
    };
}klua_kwebsocket_t;

//////////////////////////////////////////////////////////////////////////

int klua_open_kwebsocket(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        //{ "connect",    klua_khttp_connect },

        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
