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
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_multiplex.h"

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
    KLB_PROTOCOL_WS_FLV     = 12,   ///< websocket flv 协议
    KLB_PROTOCOL_MAX        = 13
}klb_protocol_e;


typedef struct klb_ncm_t_ klb_ncm_t;


/// @brief 创建ncm(net connect manage); 网络媒体长链接管理模块
/// @param [in]  *p_json_cfg            json配置
/// @return klb_ncm_t* 管理模块
KLB_API klb_ncm_t* klb_ncm_create(klb_multiplex_t* p_multi);


/// @brief 销毁ncm
/// @param [in]  *p_ncm                 ncm模块
/// @return 无
KLB_API void klb_ncm_destroy(klb_ncm_t* p_ncm);


/// @struct klb_ncm_obj_t
/// @brief  ncm连接
typedef struct klb_ncm_parser_t
{
    /// @brief 创建连接
    /// @param [in] *p_ncm          ncm模块
    /// @return void* 连接的指针
    void* (*cb_create)(klb_ncm_t* p_ncm, int protocol, int id);

    /// @brief 销毁连接
    /// @param [in] *ptr            连接的指针
    /// @return 无
    void  (*cb_destroy)(void* ptr);

    /// @brief 主动发送数据(非媒体数据)
    /// @param [in] *ptr            连接的指针
    /// @return int
    int   (*cb_send)(void* ptr, klb_socket_t* p_socket, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len);
    
    /// @brief 主动发送媒体数据
    /// @param [in] *ptr            连接的指针
    /// @return int
    int   (*cb_send_media)(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data);

    /// @brief 当网络上可以发送数据时
    /// @param [in] *ptr            连接的指针
    /// @return int
    int   (*on_send)(void* ptr, klb_socket_t* p_socket, int64_t now);

    /// @brief 当网络上可以接收数据时
    /// @param [in] *ptr            连接的指针
    /// @return int
    int   (*on_recv)(void* ptr, klb_socket_t* p_socket, int64_t now);
}klb_ncm_parser_t;


/// @brief 向ncm注册一个协议解析器
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_register(klb_ncm_t* p_ncm, int protocol, const klb_ncm_parser_t* p_parser);


/// @brief 向ncm放入一个socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int -1.放入失败; 大于0.为id号
KLB_API int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);


/// @brief 关闭socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_close(klb_ncm_t* p_ncm, int id);


/// @brief 发送数据(非媒体数据)
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_extra, int extra_len, const uint8_t* p_data, int data_len);


/// @brief 接收数据(非媒体数据)
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_recv(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, int* p_code, uint32_t* p_sequence, uint32_t* p_uid, klb_buf_t** p_extra, klb_buf_t** p_data);


/// @brief 发送媒体数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data);


/// @brief 接收媒体数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_recv_media(klb_ncm_t* p_ncm, int* p_protocol, int* p_id, klb_buf_t** p_data);


/// @brief 调用一次ncm; 需要定期调用
/// @param [in] *p_ncm              ncm模块
/// @return int 0
KLB_API int klb_ncm_loop_once(klb_ncm_t* p_ncm, int64_t now);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NCM_H__
//end
