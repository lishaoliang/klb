///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_nsc.h
/// @brief   net single connect: 单个网络连接
///  \n 将网络协议统一为一种接口调用方式;
///  \n 优势: 协议部分代码复用, 接口统一
///  \n 缺陷: 失去灵活性
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NSC_H__
#define __KLB_NSC_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_socket_tls.h"
#include "klbmem/klb_buf.h"
#include "klbnet/klb_multiplex.h"
#include "klbnet/klb_ncm.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_nsc_t_ klb_nsc_t;


/// @brief 创建nsc(net single connect); 单个网络连接
/// @param [in]  *p_multi               socekt复用模块
/// @return klb_nsc_t* 管理模块
KLB_API klb_nsc_t* klb_nsc_create(klb_multiplex_t* p_multi);


/// @brief 销毁nsc
/// @param [in]  *p_nsc                 nsc模块
/// @return 无
KLB_API void klb_nsc_destroy(klb_nsc_t* p_nsc);


/// @brief 添加数据接收函数
/// @param [in]  *p_nsc                 nsc模块
/// @param [in]  cb_recv                数据接收函数
/// @param [in]  *p_obj                 用户指针
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_set_receiver(klb_nsc_t* p_nsc, klb_ncm_ops_recv_cb cb_recv, void* p_obj);


/// @brief 向nsc放入socket
/// @param [in]  *p_nsc                 nsc模块
/// @param [in]  *p_ops                 连接操作接口ops
/// @return int -1.放入失败; 大于0.为id号
KLB_API int klb_nsc_push_by_ops(klb_nsc_t* p_nsc, const klb_ncm_ops_t* p_ops, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);


/// @brief 向nsc放入socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int -1.放入失败; 大于0.为id号
KLB_API int klb_nsc_push(klb_nsc_t* p_nsc, int protocol, klb_socket_t* p_socket, const uint8_t* p_data, int data_len);


/// @brief 关闭socket
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_close(klb_nsc_t* p_nsc);


/// @brief 获取nsc的状态
/// @param [in]  *p_ncm                 ncm模块
/// @return int klb_socket_status_e; -1:没有socket/或socket已关闭
KLB_API int klb_nsc_get_status(klb_nsc_t* p_nsc);


/// @brief 发送文本数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_send_text(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送二进制数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_send_binary(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送媒体数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_send_media(klb_nsc_t* p_nsc, klb_buf_t* p_data);


/// @brief 发送RPC-Lua数据
/// @param [in]  *p_ncm                 ncm模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_send_rpc(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


/// @brief 发送RPC-Json数据
/// @param [in]  *p_nsc                 nsc模块
/// @return int 0.成功; 非0.失败
KLB_API int klb_nsc_send_rpc_json(klb_nsc_t* p_nsc, uint32_t sequence, uint32_t uid, const uint8_t* p_head, int head_len, const uint8_t* p_body, int body_len);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NSC_H__
//end
