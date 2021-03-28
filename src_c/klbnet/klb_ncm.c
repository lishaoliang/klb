#include "klbnet/klb_ncm.h"
#include "klbmem/klb_mem.h"
#include "klbbase/klb_multiplex.h"
#include <assert.h>


typedef struct klb_ncm_t_
{
    klb_multiplex_t*    p_multi;            ///< 多路复用
}klb_ncm_t;


klb_ncm_t* klb_ncm_create(const char* p_json_cfg)
{
    klb_ncm_t* p_ncm = KLB_MALLOC(klb_ncm_t, 1, 0);
    KLB_MEMSET(p_ncm, 0, sizeof(klb_ncm_t));

    p_ncm->p_multi = klb_multiplex_create();

    return p_ncm;
}

void klb_ncm_destroy(klb_ncm_t* p_ncm)
{
    assert(NULL != p_ncm);
    
    KLB_FREE_BY(p_ncm->p_multi, klb_multiplex_destroy);

    KLB_FREE(p_ncm);
}

int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len)
{
    return 0;
}

int klb_ncm_close(klb_ncm_t* p_ncm, int id)
{
    return 0;
}

int klb_ncm_send(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len)
{
    return 0;
}

int klb_ncm_recv(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, int* p_code, uint32_t* p_sequence, uint32_t* p_uid, klb_buf_t** p_extra, klb_buf_t** p_data)
{
    return 0;
}

int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data)
{
    return 0;
}

int klb_ncm_recv_media(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, klb_buf_t** p_data)
{
    return 0;
}

int klb_ncm_loop_once(klb_ncm_t* p_ncm, int64_t now)
{
    return 0;
}
