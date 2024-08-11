///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_extension.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb gui extension 扩展接口定义
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_EXTENSION_H__
#define __KLBUI_EXTENSION_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_gui_t_ klb_gui_t;


/// @struct klb_gui_extension_msg_e
/// @brief  对gui扩展的消息
typedef enum klb_gui_extension_msg_e_
{
    KLBUI_EX_MSG_quit   = 1,        ///< 退出UI框架: (清理所有)即将destroy时触发
    KLBUI_EX_MSG_clear  = 2,        ///< 清理UI数据: (调用klb_gui_clear)时, 触发; 无附加参数
}klb_gui_extension_msg_e;


/// @struct klb_gui_extension_t
/// @brief  gui扩展
typedef struct klb_gui_extension_t_
{
    /// @brief 创建扩展
    /// @param [in] *p_gui          gui对象
    /// @return void* 扩展的指针
    void* (*cb_create)(klb_gui_t* p_gui);

    /// @brief 销毁扩展
    /// @param [in] *ptr            扩展的指针
    /// @return 无
    void(*cb_destroy)(void* ptr, klb_gui_t* p_gui);

    /// @brief 控制操作消息
    /// @param [in]     *ptr            扩展的指针
    /// @param [in]     *p_gui          gui对象
    /// @param [in]     msg             消息事件: 详见 klb_gui_extension_msg_e
    /// @param [in,out] *p_param_in_out 消息参数
    /// @param [in]     param_size      消息参数大小
    /// @return int 0
    /// @note 当UI框架需要扩展处理事务时调用
    int(*cb_control)(void* ptr, klb_gui_t* p_gui, int msg, uint8_t* p_param_in_out, int param_size);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_gui          gui对象
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_loop_once)(void* ptr, klb_gui_t* p_gui, int64_t now);
}klb_gui_extension_t;


/// @brief 注册gui扩展
/// @param [in] *p_gui              gui对象
/// @param [in] *p_name             名称
/// @param [in] *p_extension        扩展的接口函数
/// @return int 0
KLB_API int klb_gui_register_extension(klb_gui_t* p_gui, const char* p_name, const klb_gui_extension_t* p_extension);


/// @brief 获取gui扩展
/// @param [in] *p_gui              gui对象
/// @param [in] *p_name             名称
/// @return void* 扩展的指针
KLB_API void* klb_gui_get_extension(klb_gui_t* p_gui, const char* p_name);



#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_EXTENSION_H__
//end
