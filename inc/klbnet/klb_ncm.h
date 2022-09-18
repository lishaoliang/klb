///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_ncm.h
/// @brief   net connect manage: 多个网路连接
///  \n 一般用于服务端
/// @version 0.2
/// @history 修改历史
///  \n 2022 0.2 加入直接支持RPC
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NCM_H__
#define __KLB_NCM_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_multiplex.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @enum  klb_protocol_e
/// @brief 协议类型
typedef enum klb_protocol_e_
{
    KLB_PROTOCOL_UNKOWN         = 0,    ///< unkown
    KLB_PROTOCOL_MNP            = 1,    ///< mnp 协议
    KLB_PROTOCOL_RTMP           = 2,    ///< rtmp 协议
    KLB_PROTOCOL_RTSP           = 3,    ///< rtsp 协议

    KLB_PROTOCOL_HTTP_MNP       = 4,    ///< http mnp 协议
    KLB_PROTOCOL_HTTP_FLV       = 5,   ///< http flv 协议
    KLB_PROTOCOL_WS_MNP         = 6,   ///< websocket mnp 协议
    KLB_PROTOCOL_WS_FLV         = 7,   ///< websocket flv 协议

    KLB_PROTOCOL_HTTP           = 8,    ///< http 协议
    KLB_PROTOCOL_WS             = 9,    ///< websocket 协议

    KLB_PROTOCOL_RPC_MNP_LUA    = 10,   ///< RPC mnp-lua
    KLB_PROTOCOL_RPC_MNP_JSON   = 11,   ///< RPC mnp-json

    KLB_PROTOCOL_RPC_HTTP_LUA   = 12,   ///< RPC http-lua
    KLB_PROTOCOL_RPC_HTTP_JSON  = 13,   ///< RPC http-json

    KLB_PROTOCOL_RPC_WS_LUA     = 14,   ///< RPC ws-lua
    KLB_PROTOCOL_RPC_WS_JSON    = 15,   ///< RPC ws-json

    KLB_PROTOCOL_MAX,
}klb_protocol_e;


typedef enum klb_protocol_preload_e_
{
    KLB_PROTOCOL_LOAD_STD       = 0x0001,
    KLB_PROTOCOL_LOAD_RPC       = 0x0002,
}klb_protocol_preload_e;


typedef struct klb_ncm_t_ klb_ncm_t;


/// @brief 创建ncm(net connect manage); 网络媒体连接管理模块
/// @param [in]  *p_multi               socekt复用模块
/// @param [in]  preload                预加载的解析器: klb_protocol_preload_e
/// @return klb_ncm_t* 管理模块
KLB_API klb_ncm_t* klb_ncm_create(klb_multiplex_t* p_multi, uint32_t preload);


/// @brief 销毁ncm
/// @param [in]  *p_ncm                 ncm模块
/// @return 无
KLB_API void klb_ncm_destroy(klb_ncm_t* p_ncm);


/// @enum  klb_ncm_packtype_e
/// @brief 数据包类型
typedef enum klb_ncm_packtype_e_
{
    KLB_NCM_PACK_TEXT,                  ///< 文本数据
    KLB_NCM_PACK_BINARY,                ///< 二进制数据
    KLB_NCM_PACK_MEDIA,                 ///< 媒体数据
    KLB_NCM_PACK_RPC_LUA,               ///< rpc/lua 二进制数据
    KLB_NCM_PACK_RPC_JSON,              ///< rpc/json json文本数据
}klb_ncm_packtype_e;


/// @brief 当前连接接收到数据/错误等信息,后调用此函数
/// @param [in]  *ptr                   用户指针
/// @param [in]  protocol               协议号: klb_protocol_e
/// @param [in]  id                     连接号: 正整数
/// @param [in]  code                   错误码: 0.成功; klb_socket_status_e
/// @param [in]  packtype               数据类型: klb_ncm_packtype_e
/// @param [in]  *p_data                数据体
/// @return int 0
typedef int (*klb_ncm_ops_recv_cb)(void* ptr, int protocol, int id, int code, int packtype, klb_buf_t* p_data);


/// @brief 添加数据接收函数
/// @param [in]  *p_ncm                 ncm模块
/// @param [in]  cb_recv                数据接收函数
/// @param [in]  *p_obj                 用户指针
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_add_receiver(klb_ncm_t* p_ncm, klb_ncm_ops_recv_cb cb_recv, void* p_obj);


/// @struct klb_ncm_ops_lparam_t
/// @brief  ncm ops 连接参数1: 关键参数
typedef struct klb_ncm_ops_lparam_t_
{
    void*               p_ncm;              ///< ncm模块
    klb_socket_t*       p_socket;           ///< socket

    int                 protocol;           ///< 协议号
    int                 id;                 ///< ID

    klb_buf_malloc_cb   cb_malloc;          ///< 文本(TEXT)/二进制(BINARY)数据类型的内存池分配函数
    void*               p_pool;             ///< cb_malloc函数的操作对象(内存池)

    klb_buf_malloc_cb   cb_malloc_media;    ///< 媒体(MEDIA)数据类型的内存池分配函数
    void*               p_pool_media;       ///< cb_malloc_media函数的操作对象(内存池)

    klb_ncm_ops_recv_cb cb_recv;            ///< 当收到数据后, 通过此回调函数将数据交给ncm模块
}klb_ncm_ops_lparam_t;


/// @struct klb_ncm_ops_lparam_t
/// @brief  ncm ops 连接参数2: 辅助参数, 读写缓存建议大小等
typedef struct klb_ncm_ops_wparam_t_
{
    int read_buffer_size;                   ///< 读缓存建议大小
}klb_ncm_ops_wparam_t;


/// @struct klb_ncm_ops_t
/// @brief  ncm连接
typedef struct klb_ncm_ops_t_
{
    /// @brief 创建连接
    /// @param [in] *p_lparam    ncm连接关键参数1
    /// @param [in] *p_wparam    ncm连接辅助参数2
    /// @return void* 连接的指针
    void* (*cb_create)(klb_ncm_ops_lparam_t* p_lparam, klb_ncm_ops_wparam_t* p_wparam);

    /// @brief 销毁连接
    /// @param [in] *ptr            连接的指针
    /// @return 无
    void  (*cb_destroy)(void* ptr);

    /// @brief 初始数据等
    /// @param [in] *ptr            ops对象
    /// @param [in] *p_data         初始已经读取的数据
    /// @param [in] data_len        数据长度
    /// @return int 0.成功; 非0.失败
    int   (*cb_init)(void* ptr, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);

    /// @brief 对连接进行控制操作: get/set,etc.
    /// @param [in] *ptr            ops对象
    /// @return int 0.成功; 非0.失败
    int   (*cb_ctrl)(void* ptr, const klb_map_t* p_in, klb_map_t* p_out);

    /// @brief 主动发送常规数据: 文本/二进制/RPC数据等,非媒体数据
    /// @param [in] *ptr            ops对象
    /// @param [in] packtype        包类型: KLB_MNP_TEXT,KLB_MNP_BINARY,KLB_MNP_RPC_LUA,KLB_MNP_RPC_JSON
    /// @return int
    int   (*cb_send_normal)(void* ptr, klb_socket_t* p_socket, int packtype, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);

    /// @brief 主动发送媒体数据
    /// @param [in] *ptr            ops对象
    /// @return int
    int   (*cb_send_media)(void* ptr, klb_socket_t* p_socket, klb_buf_t* p_data);

    /// @brief 当网络上可以发送数据时
    /// @param [in] *ptr            ops对象
    /// @return int
    int   (*on_send)(void* ptr, klb_socket_t* p_socket, int64_t now);

    /// @brief 当网络上可以接收数据时
    /// @param [in] *ptr            ops对象
    /// @return int
    int   (*on_recv)(void* ptr, klb_socket_t* p_socket, int64_t now);

    /// @brief 当网络上有消息传来时
    /// @param [in] *ptr            ops对象
    /// @return int
    int   (*on_proc)(void* ptr, klb_socket_t* p_socket, int msg, int64_t now);
}klb_ncm_ops_t;


/// @brief 向ncm注册一个协议解析器
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_register(klb_ncm_t* p_ncm, int protocol, const klb_ncm_ops_t* p_ops);


/// @brief 向ncm放入一个socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int -1.放入失败; 大于0.为id号
KLB_API int klb_ncm_push(klb_ncm_t* p_ncm, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);


/// @brief 关闭socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_close(klb_ncm_t* p_ncm, int id);


/// @brief 发送文本数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_text(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送二进制数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_binary(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送媒体数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_media(klb_ncm_t* p_ncm, int id, klb_buf_t* p_data);


/// @brief 发送RPC-Lua数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_rpc(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送RPC-Json数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_send_rpc_json(klb_ncm_t* p_ncm, int id, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 对某个连接进行控制操作: get/set,etc.
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_ncm_ctrl(klb_ncm_t* p_ncm, int id, const klb_map_t* p_in, klb_map_t* p_out);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NCM_H__
//end
