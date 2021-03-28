///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2021/03/28
//
/// @file    klb_multiplex.h
/// @brief   使用select/epool等复用; 适用于异步流程
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2021/03/28 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MULTIPLEX_H__
#define __KLB_MULTIPLEX_H__

#include "klb_type.h"
#include "klbnet/klb_socket.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_multiplex_t_ klb_multiplex_t;


/// @brief 创建
/// @return klb_multiplex_t* 复用对象
KLB_API klb_multiplex_t* klb_multiplex_create();


/// @brief 销毁
/// @param [in]  *p_multi               复用对象
/// @return 无
KLB_API void klb_multiplex_destroy(klb_multiplex_t* p_multi);


/// @struct klua_ex_multiplex_obj_t
/// @brief  复用对象接口
typedef struct klb_multiplex_obj_t_
{
    /// @brief 当移除之后, 可以销毁时机
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @return int 0
    /// @note cb_recv/cb_send回调函数中, 不可以销毁对象
    int (*cb_remove)(void* p_lparam, void* p_wparam, int id);

    /// @brief 当可以接收数据时
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @param [in] now                 当前系统滴答(毫秒)
    /// @return int 接收的数据量
    int (*cb_recv)(void* p_lparam, void* p_wparam, int id, int64_t now);

    /// @brief 当可以发送数据时
    /// @param [in] *p_lparam           调用者指定的参数1
    /// @param [in] *p_wparam           调用者指定的参数2
    /// @param [in] id                  ID编号
    /// @param [in] now                 当前系统滴答(毫秒)
    /// @return int 发送的数据量
    int (*cb_send)(void* p_lparam, void* p_wparam, int id, int64_t now);

    /// @brief 调用者设置的参数1
    /// @param [in] *p_lparam           调用者指定的参数1
    void* p_lparam;

    /// @brief 调用者设置的参数2
    /// @param [in] *p_lparam           调用者指定的参数2
    void* p_wparam;
}klb_multiplex_obj_t;


/// @brief 放入socket
/// @param [in]  *p_multi               复用对象
/// @param [in]  *p_socket              socket
/// @param [in]  *p_obj                 附加的参数
/// @return int 返回ID编号; 大于0的正整数
/// @note 只负责使用, 并不负责p_socket销毁
///    使用klb_multiplex_remove移除
KLB_API int klb_multiplex_push_socket(klb_multiplex_t* p_multi, klb_socket_t* p_socket, const klb_multiplex_obj_t* p_obj);


/// @brief 移除id对应的
/// @param [in]  *p_multi               复用对象
/// @param [in]  id                     ID编号
/// @return int 0.成功; 非0.失败
/// @note 必须等到 klb_multiplex_obj_t.cb_remove 函数被调用, 才正式移除完成
KLB_API int klb_multiplex_remove(klb_multiplex_t* p_multi, int id);


/// @brief 调用一次; 需要定期调用
/// @param [in] *p_multi                复用对象
/// @param [in] now                     当前系统滴答数
/// @return int 0
KLB_API int klb_multiplex_loop_once(klb_multiplex_t* p_multi, int64_t now);


#ifdef __cplusplus
}
#endif

#endif // __KLB_MULTIPLEX_H__
//end
