#include "klbbase/klb_base.h"
#include "klbnet/klb_socket.h"
#include "klbplatform/klb_thread.h"
#include "klbplatform/klb_time.h"
#include "klua/klua_multithread/klua_multithread.h"
#include <stdlib.h>
#include <assert.h>


int klb_base_init(const char* p_cfg)
{
    // 初始化伪随机
    srand(klb_tick_count() + klb_thread_tid());

    // socket
    klb_socket_init();

    // klua初始化
    klua_multithread_init();

    return 0;
}

void klb_base_quit()
{
    // klua退出
    klua_multithread_quit();

    // socket退出
    klb_socket_quit();
}
