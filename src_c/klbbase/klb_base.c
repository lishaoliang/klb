#include "klbbase/klb_base.h"
#include "klbnet/klb_socket.h"
#include "klbplatform/klb_thread.h"
#include "klbplatform/klb_time.h"
#include <stdlib.h>
#include <assert.h>


int klb_base_init(const char* p_cfg)
{
    // socket
    klb_socket_init();

    // 初始化随机值
    srand(klb_tick_count() + klb_thread_tid());

    return 0;
}

void klb_base_quit()
{
    klb_socket_quit();
}
