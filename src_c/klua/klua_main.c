#include "klua/klua.h"
#include "klbnet/klb_socket.h"
#include "klua/klua_env.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_time.h"
#include "klbplatform/klb_thread.h"
#include <assert.h>


static int klua_openlibs_std(lua_State* L)
{
    klua_loadlib_all(L);

    return 0;
}

int klua_main(int argc, char** argv, klua_openlibs_cb cb)
{
    if (argc < 2)
    {
        KLB_LOG("argc:[%d]\n", argc);
        return 0;
    }

    klb_socket_init();
    srand(klb_tick_count() + klb_thread_tid());     // 初始随机值

    klua_env_t* p_env = klua_env_create(klua_openlibs_std);

    if (0 != klua_env_dofile(p_env, argv[1]))
    {
        goto end;
    }

    if (!klua_env_is_exit(p_env))
    {
        while (true)
        {
            klua_env_loop_once(p_env);

            if (klua_env_is_exit(p_env))
            {
                break;
            }
            else
            {
                klb_sleep(10);
            }
        }
    }

    klua_env_doend(p_env);

end:
    KLB_FREE_BY(p_env, klua_env_destroy);
    klb_socket_quit();

    return 0;
}
