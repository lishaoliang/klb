// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netprobe_conn.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   探测连接
///          对已 accept 的 socket fd 嗅探 TLS / 明文协议, 不替代 listen
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETPROBE_CONN_H__
#define __KLB_NETPROBE_CONN_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netmulti.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @def   KLB_NETPROBE_PEEK_MAX
/// @brief 嗅探前缀最大字节
#define KLB_NETPROBE_PEEK_MAX           4096

/// @def   KLB_NETPROBE_TIMEOUT_DEFAULT
/// @brief 默认探测超时(毫秒)
#define KLB_NETPROBE_TIMEOUT_DEFAULT    3000


/// @enum  klb_netprobe_result_e
/// @brief 前缀判定结果
typedef enum klb_netprobe_result_e_
{
    KLB_NETPROBE_OK         = 0,    ///< 已得出结论 (TLS 和/或明文协议)
    KLB_NETPROBE_NEED_MORE  = 1,    ///< 字节不足, 继续收
    KLB_NETPROBE_UNKNOWN    = 2,    ///< 无法识别
}klb_netprobe_result_e;


/// @brief 按已读前缀判定 TLS 与协议
/// @param [in]  *p_buf             前缀; len>0 时不可为 NULL
/// @param [in]  len                前缀字节数
/// @param [out] *p_tls             是否 TLS ClientHello
/// @param [out] *p_protocol        明文协议: klb_protocol_e; TLS 时为 UNKOWN
/// @return int klb_netprobe_result_e
/// @note 判定顺序: TLS -> MNP magic -> SMP magic -> 首行 RTSP/ / HTTP/
///  \n 不消费数据; 不区分 HTTP Upgrade(WS)
KLB_API int klb_netprobe_check(const uint8_t* p_buf, int len, bool* p_tls, int* p_protocol);


/// @brief 创建探测连接
/// @param [in]  *p_multi           复用模块
/// @return klb_netconn_t*          探测连接
KLB_API klb_netconn_t* klb_netprobe_conn_create(klb_netmulti_t* p_multi);


/// @brief 释放连接
/// @param [in]  *p_conn            探测连接
/// @return 无
/// @note 若已 open 且尚未 done, 关闭 fd; 若已 done, 勿再调用 (已交给 netmulti closing)
KLB_API void klb_netprobe_conn_free(klb_netconn_t* p_conn);


/// @brief 挂接已 accept 的 fd 并开始探测
/// @param [in]  *p_conn            探测连接
/// @param [in]  fd                 已 accept 的 socket fd
/// @param [in]  timeout_ms         超时(毫秒); <=0 使用 KLB_NETPROBE_TIMEOUT_DEFAULT
/// @return int 0.成功; 非0.失败
/// @note MSG_PEEK 不消费内核数据; done 后调用方仍可从 fd 头读
KLB_API int klb_netprobe_conn_open(klb_netconn_t* p_conn, klb_socket_fd fd, int timeout_ms);


/// @brief 探测结束回调
/// @param [in]  *p_conn            探测连接; 回调返回后即将 closing, 勿 destroy
/// @param [in]  *ptr               用户指针
/// @param [in]  fd                 原 socket fd; 无回调时由探测连接关闭
/// @param [in]  protocol           klb_protocol_e; TLS 未握手时为 UNKOWN
/// @param [in]  tls                是否判定为 TLS
/// @param [in]  *p_peek            本次 PEEK 副本; 仅回调期间有效
/// @param [in]  peek_len           副本字节数
/// @return int 0.成功; 非0.失败
typedef int(*klb_netprobe_conn_done_cb)(klb_netconn_t* p_conn, void* ptr, klb_socket_fd fd, int protocol, bool tls, const uint8_t* p_peek, int peek_len);


/// @brief 设置探测结束回调
/// @param [in]  *p_conn            探测连接
/// @param [in]  cb_done            结束回调; 可为 NULL (结束时关闭 fd)
/// @param [in]  *ptr               cb_done 附加指针
/// @return int 0
KLB_API int klb_netprobe_conn_set_done(klb_netconn_t* p_conn, klb_netprobe_conn_done_cb cb_done, void* ptr);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETPROBE_CONN_H__

// end
