// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klua/klua.h"
#include "klbnet/klb_socket.h"
#include "klua/klua_env.h"
#include "klbutil/klb_log.h"
#include "klbmem/klb_mem.h"
#include "klbplatform/klb_time.h"
#include "klbplatform/klb_thread.h"
#include "klua/klua_thread.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_map_serialize.h"
#include <assert.h>


static void klua_main_set_args(klua_env_t* p_env, int argc, char** argv)
{
    klb_map_t r = { 0 };
    klb_map_init(&r);

    for (int i = 0; i < argc; i++)
    {
        klb_map_set_idx_string(&r, i, argv[i]);
    }

    klb_buf_t* p_args = klb_map_seri_pack(&r);
    klua_env_set_args(p_env, p_args->p_buf + p_args->start, p_args->end - p_args->start);

    KLB_FREE(p_args);
    klb_map_quit(&r);
}

int klua_main(int argc, char** argv, klua_openlibs_cb cb)
{
    if (argc < 2)
    {
        KLB_LOG("argc:[%d]\n", argc);
        return 0;
    }

    // 设置所有子线程都使用预加载库 cb
    klua_thread_set_preload(cb);

    // env
    klua_env_t* p_env = klua_env_create(cb);    // env
    klua_main_set_args(p_env, argc, argv);      // args
    klua_thread_register("main", p_env);        // main

    if (0 != klua_env_dofile(p_env, argv[1]))
    {
        goto end;
    }

    if (!klua_env_is_exit(p_env))
    {
        while (true)
        {
            int sleep = klua_env_loop_once(p_env);

            if (klua_env_is_exit(p_env))
            {
                break;
            }
            else if(0 < sleep)
            {
                klb_sleep(sleep);
            }
        }
    }

    klua_env_doend(p_env);

end:
    klua_thread_unregister("main");
    KLB_FREE_BY(p_env, klua_env_destroy);

    return 0;
}
