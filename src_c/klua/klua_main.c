﻿#include "klua/klua.h"
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

    klua_env_t* p_env = klua_env_create(klua_openlibs_std);
    klua_kthread_register("main", p_env);

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
    klua_kthread_unregister("main");
    KLB_FREE_BY(p_env, klua_env_destroy);

    return 0;
}
