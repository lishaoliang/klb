///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_ncm.h
/// @brief   net connect manage: ./klb/src/knet/ncm.go
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NCM_H__
#define __KLB_NCM_H__

#include "klb_type.h"
#include "socket/klb_socket.h"
#include "mem/klb_buf.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum klb_protocol_e_
{
    KLB_PROTOCOL_UNKOWN     = 0,    ///< unkown
    KLB_PROTOCOL_MNP        = 1,    ///< mnp 协议
    KLB_PROTOCOL_MNPS       = 2,    ///< mnp 协议: TLS
    KLB_PROTOCOL_RTMP       = 3,    ///< rtmp 协议
    KLB_PROTOCOL_RTSP       = 4,    ///< rtsp 协议
    KLB_PROTOCOL_HTTP       = 5,    ///< http 协议
    KLB_PROTOCOL_HTTPS      = 6,    ///< https 协议: TLS
    KLB_PROTOCOL_WS         = 7,    ///< websocket 协议
    KLB_PROTOCOL_WSS        = 8,    ///< websocket 协议: TLS

    KLB_PROTOCOL_HTTP_MNP   = 9,    ///< http mnp 协议
    KLB_PROTOCOL_HTTP_FLV   = 10,   ///< http flv 协议
    KLB_PROTOCOL_WS_MNP     = 11,   ///< websocket mnp 协议
    KLB_PROTOCOL_WS_FLV     = 12    ///< websocket flv 协议
}klb_protocol_e;


typedef struct klb_ncm_t_ klb_ncm_t;


klb_ncm_t* klb_ncm_create();
void klb_ncm_destroy(klb_ncm_t* p_ncm);


int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, int id, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);
int klb_ncm_close(klb_ncm_t* p_ncm, int id);


int klb_ncm_send(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len);
int klb_ncm_recv(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, int* p_code, uint32_t* p_sequence, uint32_t* p_uid, klb_buf_t** p_extra, klb_buf_t** p_data);


int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data);
int klb_ncm_recv_media(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, klb_buf_t** p_data);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NCM_H__
//end
