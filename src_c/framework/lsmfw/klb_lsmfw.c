#include "framework/klb_lsmfw.h"
#include "klb_lsm.h"
#include "mem/klb_mem.h"
#include <assert.h>


static klb_lsm_t* g_klb_lsm = NULL;


int klb_lsmfw_init(const char* p_cfg)
{
    assert(NULL == g_klb_lsm);

    g_klb_lsm = klb_lsm_create();

    return 0;
}

void klb_lsmfw_quit()
{
    KLB_FREE_BY(g_klb_lsm, klb_lsm_destroy);
}

int klb_lsmfw_start()
{
    assert(NULL != g_klb_lsm);

    return klb_lsm_start(g_klb_lsm);
}

void klb_lsmfw_stop()
{
    assert(NULL != g_klb_lsm);

    klb_lsm_stop(g_klb_lsm);
}

int klb_lsmfw_listen()
{
    return 0;
}

int klb_lsmfw_listen_tls()
{
    return 0;
}

int klb_lsmfw_close_listen()
{
    return 0;
}

int klb_lsmfw_set_new_conn_cb()
{
    return 0;
}

int klb_lsmfw_set_receiver_cb()
{
    return 0;
}

int klb_lsmfw_set_disconnect_cb()
{
    return 0;
}

int klb_lsmfw_switch_protocol()
{
    return 0;
}

int klb_lsmfw_close_conn(int id)
{
    return 0;
}

int klb_lsmfw_send_txt(int id)
{
    return 0;
}

int klb_lsmfw_send_file(int id)
{
    return 0;
}

int klb_lsmfw_open_stream(int id)
{
    return 0;
}

int klb_lsmfw_close_stream(int id)
{
    return 0;
}

int klb_lsmfw_push_media(uint32_t chnn, uint32_t sidx)
{
    return 0;
}
