// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbnet/klb_socket.h"                  // Fixed Bug. [2025] MINGW32/MINGW64 必须先引用 <winsock2.h>, 再 引用 <windows.h>
#include "klua/extension/klua_extension.h"
#include "klua/extension/klua_ex_object.h"
#include "klua/extension/klua_ex_bufagent.h"
#include "klua/extension/klua_ex_time.h"
#if !defined(__KLB_NO_GUI__)
#include "klua/extension/klua_ex_gui.h"
#endif
#include "klua/extension/klua_ex_netmulti.h"
#include "klua/extension/klua_ex_lpc.h"
#include "klua/extension/klua_ex_coroutine.h"


int klua_register_extension_std(klua_env_t* p_env)
{
    // ojbect
    klua_ex_register_object(p_env);

    // 缓存代理
    klua_ex_register_bufagent(p_env);

    // 协程
    klua_ex_register_coroutine(p_env);

    // 时间/定时器
    klua_ex_register_time(p_env);

    // 复用 net connect
    klua_ex_register_netmulti(p_env);

    // LPC: 本地(跨Lua线程)过程调用(Local Procedure Call Protocol)
    klua_ex_register_lpc(p_env);

#if !defined(__KLB_NO_GUI__)
    // gui
    klua_ex_register_gui(p_env);
#endif

    return 0;
}

//end
