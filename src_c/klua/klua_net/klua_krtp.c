// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbmem/klb_mem.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_listen.h"
#include "klbutil/klb_list.h"
#include "klbbase/klb_mnp.h"
#include "klbbase/klb_mnp_help.h"
#include "klbmem/klb_buffer.h"
#include "klua/klua.h"
#include "klua/klua_env.h"
#include "klua/extension/klua_ex_coroutine.h"
#include "klua/extension/klua_ex_multiplex.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////

// rtp
int klua_open_krtp(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { NULL,         NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
