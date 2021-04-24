#include "klua/extension/klua_extension.h"
#include "klua/extension/klua_ex_time.h"
#include "klua/extension/klua_ex_gui.h"
#include "klua/extension/klua_ex_multiplex.h"
#include "klua/extension/klua_ex_lpc.h"


int klua_register_extension_std(klua_env_t* p_env)
{
    // 时间/定时器
    klua_ex_register_time(p_env);

    // 复用
    klua_ex_register_multiplex(p_env);

    // LPC: 本地(跨Lua线程)过程调用(Local Procedure Call Protocol)
    klua_ex_register_lpc(p_env);

    // gui
    klua_ex_register_gui(p_env);

    return 0;
}
