///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_ex_multiplex.h
/// @brief   复用
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EX_MULTIPLEX_H__
#define __KLUA_EX_MULTIPLEX_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef klb_socket_fd   klua_ex_fd;
#define INVALID_EX_FD   INVALID_SOCKET


typedef struct klua_ex_multiplex_t_ klua_ex_multiplex_t;


/// @brief 注册复用扩展
/// @param [in] *p_env           lua环境
/// @return int 0
int klua_ex_register_multiplex(klua_env_t* p_env);


/// @brief 获取复用扩展
/// @param [in] *p_env           lua环境
/// @return klua_ex_multiplex_t* 扩展的指针
klua_ex_multiplex_t* klua_ex_get_multiplex(klua_env_t* p_env);


/// @struct klua_ex_multiplex_obj_t
/// @brief  复用对象接口
typedef struct klua_ex_multiplex_obj_t_
{
    /// @brief 当移除之后, 可以销毁时机
    /// @param [in] *p_env              lua环境
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @return int 0
    /// @note cb_recv/cb_send回调函数中, 不可以销毁对象
    int (*cb_remove)(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id);

    /// @brief 当可以接收数据时
    /// @param [in] *p_env              lua环境
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @param [in] now                 当前系统滴答(毫秒)
    /// @return int 接收的数据量
    int (*cb_recv)(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now);

    /// @brief 当可以发送数据时
    /// @param [in] *p_env              lua环境
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @param [in] now                 当前系统滴答(毫秒)
    /// @return int 发送的数据量
    int (*cb_send)(klua_env_t* p_env, void* p_lparam, void* p_wparam, int id, int64_t now);

    /// @brief 调用者设置的参数1
    /// @param [in] *p_lparam           调用者指定的参数1
    void* p_lparam;

    /// @brief 调用者设置的参数2
    /// @param [in] *p_lparam           调用者指定的参数2
    void* p_wparam;
}klua_ex_multiplex_obj_t;


/// @brief 放入socket
/// @param [in]  *p_ex                  扩展的指针
/// @param [in]  *p_socket              socket
/// @param [in]  *p_obj                 附加的参数
/// @return int 返回ID编号; 大于0的正整数
/// @note 只负责使用, 并不负责p_socket销毁
///   使用klua_ex_multiplex_remove移除
int klua_ex_multiplex_push_socket(klua_ex_multiplex_t* p_ex, klb_socket_t* p_socket, const klua_ex_multiplex_obj_t* p_obj);


/// @brief 移除id对应的
/// @param [in]  *p_ex                  扩展的指针
/// @param [in]  id                     ID编号
/// @return int 0.成功; 非0.失败
/// @note 必须等到 klua_ex_multiplex_obj_t.cb_remove 函数被调用, 才正式移除完成
int klua_ex_multiplex_remove(klua_ex_multiplex_t* p_ex, int id);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EX_MULTIPLEX_H__
//end
