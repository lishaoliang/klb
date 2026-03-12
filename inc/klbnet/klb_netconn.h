///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netconn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   网络连接 基础结构(klb net connect)
///          思路: 将网络数据的收发 抽象为 本结构体, 具体连接 继承 本结构体
///          原本对socket处理较为分散, 这里参考 窗口klb_wnd_t 思路, 每一个 网络组件 类似 窗口的控件
/// @version 0.1
/// @history 修改历史
///  \n [2025-09] 基础结构定义
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETCONN_H__
#define __KLB_NETCONN_H__


#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbutil/klb_map.h"
#include "klbbase/klb_mnp.h"
#include "klbthird/sds.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netcode.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_netconn_t
/// @brief  网络连接 基础结构
///         klb net connect
typedef struct klb_netconn_t_ klb_netconn_t;


/// @enum  klb_protocol_e
/// @brief 协议类型
typedef enum klb_protocol_e_
{
    KLB_PROTOCOL_UNKOWN         = 0,        ///< unkown
    KLB_PROTOCOL_MNP            = 1,        ///< mnp 协议
    KLB_PROTOCOL_RTMP           = 2,        ///< rtmp 协议
    KLB_PROTOCOL_RTSP           = 3,        ///< rtsp 协议

    KLB_PROTOCOL_HTTP_MNP       = 4,        ///< http mnp 协议
    KLB_PROTOCOL_HTTP_FLV       = 5,        ///< http flv 协议
    KLB_PROTOCOL_WS_MNP         = 6,        ///< websocket mnp 协议
    KLB_PROTOCOL_WS_FLV         = 7,        ///< websocket flv 协议

    KLB_PROTOCOL_HTTP           = 8,        ///< http 协议
    KLB_PROTOCOL_WS             = 9,        ///< websocket 协议

    KLB_PROTOCOL_RPC_MNP_LUA    = 10,       ///< RPC mnp-lua
    KLB_PROTOCOL_RPC_MNP_JSON   = 11,       ///< RPC mnp-json

    KLB_PROTOCOL_RPC_HTTP_LUA   = 12,       ///< RPC http-lua
    KLB_PROTOCOL_RPC_HTTP_JSON  = 13,       ///< RPC http-json

    KLB_PROTOCOL_RPC_WS_LUA     = 14,       ///< RPC ws-lua
    KLB_PROTOCOL_RPC_WS_JSON    = 15,       ///< RPC ws-json

    KLB_PROTOCOL_MAX,

    KLB_PROTOCOL_user           = 2000,     ///< 用户自定义 开始
}klb_protocol_e;


/// @def   KLB_NETCONN_NAME_MAX
/// @brief 连接名称最大字符
#define KLB_NETCONN_NAME_LEN    4
#define KLB_NETCONN_NAME_MAX    8


/// @enum  klb_netconn_msg_e
/// @brief 连接内部消息
typedef enum klb_netconn_msg_e_
{
    KLB_NETCONN_MSG_null                = 0,
    KLB_NETCONN_MSG_connect_timeout,                ///< connect 超时

    KLB_NETCONN_MSG_connected           = 20,       ///< connect 握手完成
    KLB_NETCONN_MSG_onticker,                       ///< ticker 定时器消息
}klb_netconn_msg_e;



/// @brief 销毁
/// @param [in] *p_conn      连接对象
/// @return 无
typedef void(*klb_netconn_destroy_cb)(klb_netconn_t* p_conn);

/// @brief 当前连接接收到数据/错误等信息,后调用此函数
/// @param [in]  *p_conn                连接指针
/// @param [in]  code                   错误码: 0.成功; klb_netcode_e
/// @param [in]  packtype               数据类型: klb_mnp_packtype_e
/// @param [in]  *p_data                数据体
/// @return int 0
typedef int(*klb_netconn_recv_data_cb)(klb_netconn_t* p_conn, int code, int packtype, klb_buf_t* p_data);

/// @brief 对连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
typedef int(*klb_netconn_ioctrl_cb)(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out);

/// @brief 发送常规数据包
/// @param [in] packtype      数包类型: klb_mnp_packtype_e
typedef int(*klb_netconn_send_normal_cb)(klb_netconn_t* p_conn, int packtype, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);

/// @brief 发送媒体数据包
/// @return int
typedef int(*klb_netconn_send_media_cb)(klb_netconn_t* p_conn, klb_buf_t* p_data);

/// @brief 当网络上可以发送数据时
/// @return int
typedef int(*klb_netconn_on_send_cb)(klb_netconn_t* p_conn, int64_t now);

/// @brief 当网络上可以接收数据时
/// @return int
typedef int(*klb_netconn_on_recv_cb)(klb_netconn_t* p_conn, int64_t now);

/// @brief 当网络上有消息传来时
/// @param [in] msg             消息类型: klb_netconn_msg_e
/// @return int
typedef int(*klb_netconn_on_msg_cb)(klb_netconn_t* p_conn, int msg, int64_t now);

/// @struct klb_netconn_vtable_t
/// @brief  网络连接 抽象函数表
typedef struct klb_netconn_vtable_t_
{
    /// @brief 销毁
    /// @param [in] *p_wnd      窗体对象
    /// @return 无
    klb_netconn_destroy_cb      destroy;

    /// @brief 当前连接接收到数据/错误等信息,后调用此函数
    klb_netconn_recv_data_cb    recv_data;

    /// @brief 对连接进行控制操作: get/set,etc.
    klb_netconn_ioctrl_cb       ioctrl;

    /// @brief 调用者 发送常规数据包
    klb_netconn_send_normal_cb  send_normal;

    /// @brief 调用者 发送媒体数据包
    klb_netconn_send_media_cb   send_media;

    /// @brief 当网络上可以发送数据时
    klb_netconn_on_send_cb      on_send;

    /// @brief 当网络上可以接收数据时
    klb_netconn_on_recv_cb      on_recv;

    /// @brief 当网络上有消息传来时
    klb_netconn_on_msg_cb       on_msg;
}klb_netconn_vtable_t;

/// @struct klb_netconn_t
/// @brief  网络连接 基础结构
///     参考: 窗口klb_wnd_t, 封装做网络数据收发 的统一基础结构(或叫基类)
///     具体的 某种 协议 继承 本基础结构(或叫子类)
typedef struct klb_netconn_t_
{
    klb_netconn_vtable_t        vtable;         ///< 函数表

    // 用于记录树形结构的 连接结构
    // 当某个连接, 需要多个 socket 完成时, 使用子树 记录维护
    // eg. rtsp - udp 协议 需要 1个tcp + 2个udp完成
    klb_netconn_t*              p_parent;       ///< 父节点, 树形结构
    klb_netconn_t*              p_child;        ///< 子节点

    klb_netconn_t*              p_prev;         ///< 前一个兄弟节点
    klb_netconn_t*              p_next;         ///< 后一个兄弟节点

    // socket
    klb_socket_t*               p_socket;       ///< socket

    // 协议相关
    int                         protocol;       ///< 协议号: klb_protocol_e
    int                         id;             ///< 连接ID

    // 连接
    char                        name[KLB_NETCONN_NAME_MAX]; ///< 连接名称; 用于用于集合之后识别

    // 用户数据
    void*                       p_udata;        ///< public user data, [附加指针]
    void*                       p_lparam;       ///< 自定义含义 [附加指针1]
    void*                       p_wparam;       ///< 自定义含义 [附加指针2]

    // 扩展
    char                        extra[];        ///< 扩展数据
}klb_netconn_t;


/// @brief 销毁连接
KLB_API void klb_netconn_destroy(klb_netconn_t* p_conn);


/// @brief 绑定数据接收函数
KLB_API int klb_netconn_bind_recv_data(klb_netconn_t* p_conn, klb_netconn_recv_data_cb cb_recv);


/// @brief 设置连接名称
KLB_API void klb_netconn_set_name(klb_netconn_t* p_conn, const char* p_name);


/// @brief 获取连接名称
KLB_API const char* klb_netconn_get_name(klb_netconn_t* p_conn);


/// @brief 发送文本数据
/// @return int 0.成功; 非0.失败
KLB_API int klb_netconn_send_text(klb_netconn_t* p_conn, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送二进制数据
/// @return int 0.成功; 非0.失败
KLB_API int klb_netconn_send_binary(klb_netconn_t* p_conn, int sequence, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送媒体数据
/// @return int 0.成功; 非0.失败
KLB_API int klb_netconn_send_media(klb_netconn_t* p_conn, klb_buf_t* p_data);


/// @brief 对某个连接进行控制操作: get/set,etc.
/// @return int 0.成功; 非0.失败
KLB_API int klb_netconn_ioctrl(klb_netconn_t* p_conn, const klb_map_t* p_in, klb_map_t* p_out);


/// @brief 设置 udata/lparam/wparam
KLB_API void klb_netconn_set_udata(klb_netconn_t* p_conn, void* p_udata);
KLB_API void klb_netconn_set_lparam(klb_netconn_t* p_conn, void* p_lparam);
KLB_API void klb_netconn_set_wparam(klb_netconn_t* p_conn, void* p_wparam);


#if defined(__cplusplus)
}
#endif

#endif // __KLB_NETCONN_H__
//end
