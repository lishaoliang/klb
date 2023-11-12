///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_app_extension.h
/// @brief   klb app extension, 主应用框架扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_APP_EXTENSION_H__
#define __KLB_APP_EXTENSION_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_app_t_ klb_app_t;


/// @struct klb_app_extension_t
/// @brief  app扩展
typedef struct klb_app_extension_t_
{
    /// @brief 创建扩展
    /// @param [in] *p_app          app对象
    /// @return void* 扩展的指针
    void* (*cb_create)(klb_app_t* p_app);

    /// @brief 销毁扩展
    /// @param [in] *ptr            扩展的指针
    /// @return 无
    void(*cb_destroy)(void* ptr, klb_app_t* p_app);

    /// @brief 控制操作消息
    /// @param [in]     *ptr            扩展的指针
    /// @param [in]     *p_app          app对象
    /// @param [in]     msg             消息事件: 详见
    /// @param [in,out] *p_param_in_out 消息参数
    /// @param [in]     param_size      消息参数大小
    /// @return int 0
    /// @note 当UI框架需要扩展处理事务时调用
    int(*cb_control)(void* ptr, klb_app_t* p_app, int msg, uint8_t* p_param_in_out, int param_size);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_app          app对象
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_loop_once)(void* ptr, klb_app_t* p_app, int64_t now);
}klb_app_extension_t;


/// @brief 注册app扩展
/// @param [in] *p_app              app对象
/// @param [in] *p_name             名称
/// @param [in] *p_extension        扩展的接口函数
/// @return int 0
KLB_API int klb_app_register_extension(klb_app_t* p_app, const char* p_name, const klb_app_extension_t* p_extension);


/// @brief 获取app扩展
/// @param [in] *p_app              app对象
/// @param [in] *p_name             名称
/// @return void* 扩展的指针
KLB_API void* klb_app_get_extension(klb_app_t* p_app, const char* p_name);



#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_EXTENSION_H__
//end
