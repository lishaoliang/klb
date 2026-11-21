///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_netmulti.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   (klb net multiplex)使用select/pool/epool等多路复用; 适用于异步流程
/// @version 0.2
/// @history 修改历史
///  \n [2025-09] 创建文件
///  \n [2025-10] 添加托管关闭连接思路: klb_netmulti_closing
///  \n           将待关闭的连接, 直接交给 本模块托管关闭, 由本模块对 连接 做后续处理
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_NETMULTI_H__
#define __KLB_NETMULTI_H__


#include "klb_type.h"
#include "klbnet/klb_socket.h"
#include "klbnet/klb_netconn.h"
#include "klbnet/klb_netcode.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klb_netmulti_t
/// @brief  多路复用(klb net multiplex)
typedef struct klb_netmulti_t_ klb_netmulti_t;


/// @brief 创建
/// @return klb_netmulti_t*             复用对象
KLB_API klb_netmulti_t* klb_netmulti_create(int64_t tc);


/// @brief 销毁
/// @param [in]  *p_multi               复用对象
/// @return 无
KLB_API void klb_netmulti_destroy(klb_netmulti_t* p_multi);


/// @brief 放入连接对象
/// @param [in]  *p_multi               复用对象
/// @param [in]  *p_conn                连接对象
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 只负责使用, 并不负责p_conn销毁
KLB_API int klb_netmulti_push(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);


/// @brief 移除连接对象
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 直接移除, 非延时移除
KLB_API int klb_netmulti_remove(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);
KLB_API klb_netconn_t* klb_netmulti_remove_by_name(klb_netmulti_t* p_multi, const char* p_name);


/// @brief 托管关闭连接
/// @return int 0.成功; 非0.klb_netcode_e
/// @note 将连接 交给 复用模块 去关闭
///       移交之后, 调用者 不可以再使用 连接
///       将待关闭的连接, 直接交给 本模块托管关闭, 由本模块对 连接 做后续处理
KLB_API int klb_netmulti_closing(klb_netmulti_t* p_multi, klb_netconn_t* p_conn);


/// @brief 调用一次; 需要定期调用
/// @param [in] *p_multi                复用对象
/// @param [in] now                     当前系统滴答数
/// @return int 0
KLB_API int klb_netmulti_loop_once(klb_netmulti_t* p_multi, int64_t now);


/// @brief HTTP 连接升级为 WS 后, 由 kws 注册的接管函数
/// @param [in] *L                      Lua 状态, 类型为 lua_State*
/// @param [in] *p_multi                复用对象
/// @param [in] *p_socket               已从 HTTP 连接摘出的 socket; 失败时由回调关闭
/// @param [in] *p_tail                 握手后剩余字节; 可为 NULL; 回调负责释放
/// @param [in] *p_peer                 对端 "ip:port"; 可为 NULL 或空串
/// @return int 压栈个数
/// @note 无 kws 时不注册. 回调成功时压入 kws userdata
typedef int (*klb_netmulti_ws_adopt_fn)(void* L, klb_netmulti_t* p_multi, klb_socket_t* p_socket, klb_buf_t* p_tail, const char* p_peer);


/// @brief 注册 WS 接管
/// @param [in] fn                      接管函数; NULL 表示清除
/// @return 无
KLB_API void klb_netmulti_set_ws_adopt(klb_netmulti_ws_adopt_fn fn);


/// @brief 取得 WS 接管
/// @return klb_netmulti_ws_adopt_fn    未注册为 NULL
KLB_API klb_netmulti_ws_adopt_fn klb_netmulti_get_ws_adopt(void);


#ifdef __cplusplus
}
#endif

#endif // __KLB_NETMULTI_H__
//end
