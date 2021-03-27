#include "klua/extension_single/klua_extension_single.h"
#include "klua/extension_single/klua_exs_thread.h"
#include "klua/extension_single/klua_exs_mcache.h"


int klua_register_extension_single_std(klua_env_t* p_env)
{
    // 多线程相关
    klua_exs_register_thread(p_env);

    // 线程之间共享缓存
    klua_exs_register_mcache(p_env);

    return 0;
}
