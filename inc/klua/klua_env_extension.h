///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_env_extension.h
/// @brief   lua_State简易封装扩展
/// @version 0.1
/// @history 修改历史
///  \n 2023 0.1 将klua_env_t的扩展, 转移到专门的文件中
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_ENV_EXTENSION_H__
#define __KLUA_ENV_EXTENSION_H__


#include "klb_type.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klua_env_t_ klua_env_t;
typedef struct klua_msg_t_ klua_msg_t;


/// @struct klua_env_extension_t
/// @brief  lua环境扩展
typedef struct klua_env_extension_t_
{
    /// @brief 创建扩展
    /// @param [in] *p_env          lua环境
    /// @return void* 扩展的指针
    void* (*cb_create)(klua_env_t* p_env);

    /// @brief 销毁扩展
    /// @param [in] *ptr            扩展的指针
    /// @return 无
    void(*cb_destroy)(void* ptr);

    /// @brief 对扩展直接控制设置
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] *p_param        控制参数
    /// @return klua_msg_t* 返回信息
    /// @note eg. 设置扩展的参数, 获取扩展的参数等.
    klua_msg_t* (*cb_ctrl)(void* ptr, klua_env_t* p_env, klua_msg_t* p_msg);

    /// @brief 消息处理
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_msg)(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] last_tc         上一次的滴答数
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_loop_once)(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now);
}klua_env_extension_t;


/// @brief 注册lua环境扩展
/// @param [in] *p_env              lua环境
/// @param [in] *p_name             名称
/// @param [in] *p_extension        扩展的接口函数
/// @return int 0
KLB_API int klua_env_register_extension(klua_env_t* p_env, const char* p_name, const klua_env_extension_t* p_extension);


/// @brief 获取lua环境扩展
/// @param [in] *p_env              lua环境
/// @param [in] *p_name             名称
/// @return void* 扩展的指针
KLB_API void* klua_env_get_extension(klua_env_t* p_env, const char* p_name);



#if defined(__cplusplus)
}
#endif

#endif // __KLUA_ENV_EXTENSION_H__
//end
