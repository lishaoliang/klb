#include "socket/klb_socket_transmit.h"
#include "mem/klb_mem.h"
#include "platform/klb_thread.h"
#include <assert.h>


typedef struct klb_socket_transmit_t_
{
    klb_thread_t*   p_thread;
}klb_socket_transmit_t;


static int klb_socket_transmit_thread(void* p_obj, int* p_run);


klb_socket_transmit_t* klb_socket_transmit_create()
{
    klb_socket_transmit_t* p_transmit = KLB_MALLOC(klb_socket_transmit_t, 1, 0);
    KLB_MEMSET(p_transmit, 0, sizeof(klb_socket_transmit_t));


    return p_transmit;
}

void klb_socket_transmit_destroy(klb_socket_transmit_t* p_transmit)
{
    assert(NULL != p_transmit);


    KLB_FREE(p_transmit);
}


int klb_socket_transmit_start(klb_socket_transmit_t* p_transmit, int cpu_idx, const char* p_name)
{
    assert(NULL != p_transmit);
    assert(NULL == p_transmit->p_thread);

    p_transmit->p_thread = klb_thread_create(klb_socket_transmit_thread, p_transmit, cpu_idx, p_name);

    return (NULL != p_transmit->p_thread) ? 0 : 1;
}


void klb_socket_transmit_stop(klb_socket_transmit_t* p_transmit)
{
    assert(NULL != p_transmit);

    KLB_FREE_BY(p_transmit->p_thread, klb_thread_destroy);
}


//////////////////////////////////////////////////////////////////////////

static int klb_socket_transmit_thread(void* p_obj, int* p_run)
{
    klb_socket_transmit_t* p_transmit = (klb_socket_transmit_t*)p_obj;

    while (0 != *p_run)
    {

        klb_sleep(10);
    }

    return 0;
}
