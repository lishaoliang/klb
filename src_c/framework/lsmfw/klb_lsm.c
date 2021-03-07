#include "klb_lsm.h"
#include "klb_type.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include "socket/klb_socket.h"
#include <assert.h>

static int klb_thread_lsm_listen(void* p_obj, int* p_run);
static int klb_thread_lsm_txt(void* p_obj, int* p_run);
static int klb_thread_lsm_media(void* p_obj, int* p_run);

klb_lsm_t* klb_lsm_create()
{
    klb_lsm_t* p_lsm = KLB_MALLOC(klb_lsm_t, 1, 0);
    KLB_MEMSET(p_lsm, 0, sizeof(klb_lsm_t));

    p_lsm->ncm_num = 1;
    for (int i = 0; i < p_lsm->ncm_num; i++)
    {
        p_lsm->p_ncm[i] = klb_ncm_create();
    }

    return p_lsm;
}

void klb_lsm_destroy(klb_lsm_t* p_lsm)
{
    assert(NULL != p_lsm);

    for (int i = 0; i < p_lsm->ncm_num; i++)
    {
        KLB_FREE_BY(p_lsm->p_ncm[i], klb_ncm_destroy);
    }

    KLB_FREE(p_lsm)
}

int klb_lsm_start(klb_lsm_t* p_lsm)
{
    assert(NULL != p_lsm);

    p_lsm->p_thread_media = klb_thread_create(klb_thread_lsm_media, p_lsm, -1, "lsm media");
    assert(NULL != p_lsm->p_thread_media);

    p_lsm->p_thread_txt = klb_thread_create(klb_thread_lsm_txt, p_lsm, -1, "lsm txt");
    assert(NULL != p_lsm->p_thread_txt);

    p_lsm->p_thread_listen = klb_thread_create(klb_thread_lsm_listen, p_lsm, -1, "lsm listen");
    assert(NULL != p_lsm->p_thread_listen);

    return 0;
}

int klb_lsm_stop(klb_lsm_t* p_lsm)
{
    assert(NULL != p_lsm);

    KLB_FREE_BY(p_lsm->p_thread_listen, klb_thread_destroy);
    KLB_FREE_BY(p_lsm->p_thread_txt, klb_thread_destroy);
    KLB_FREE_BY(p_lsm->p_thread_media, klb_thread_destroy);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static int klb_thread_lsm_listen(void* p_obj, int* p_run)
{
    KLB_LOG("start thread,klb_thread_lsm_listen.\n");
    klb_lsm_t* p_lsm = (klb_lsm_t*)p_obj;

    klb_socket_fd fd = klb_socket_listen(1234, 20);

    while (0 != *p_run)
    {
        struct sockaddr_in addr = { 0 };

        klb_socket_fd client = klb_socket_accept(fd, &addr);
        if (INVALID_SOCKET != client)
        {
            KLB_SOCKET_CLOSE(client);
        }


        klb_sleep(10);
    }

    KLB_SOCKET_CLOSE(fd);

    KLB_LOG("stop thread,klb_thread_lsm_listen.\n");
    return 0;
}

static int klb_thread_lsm_txt(void* p_obj, int* p_run)
{
    KLB_LOG("start thread,klb_thread_lsm_txt.\n");
    klb_lsm_t* p_lsm = (klb_lsm_t*)p_obj;

    while (0 != *p_run)
    {

        klb_sleep(10);
    }

    KLB_LOG("stop thread,klb_thread_lsm_txt.\n");
    return 0;
}

static int klb_thread_lsm_media(void* p_obj, int* p_run)
{
    KLB_LOG("start thread,klb_thread_lsm_media.\n");
    klb_lsm_t* p_lsm = (klb_lsm_t*)p_obj;

    while (0 != *p_run)
    {

        klb_sleep(10);
    }

    KLB_LOG("stop thread,klb_thread_lsm_media.\n");
    return 0;
}

