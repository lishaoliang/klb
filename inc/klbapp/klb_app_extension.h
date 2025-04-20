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
typedef struct klb_app_extension_t_ klb_app_extension_t;


/// @def   KLBAPPEX_DLSYM_name_max
/// @brief 扩展的名称最大值
#define KLBAPPEX_DLSYM_name_max             (128)


/// @def   KLBAPPEX_DLSYM_extension_init
/// @brief 函数名: 动态库插件 初始化/退出
#define KLBAPPEX_DLSYM_init_extension       "klbapp_init_extension"
#define KLBAPPEX_DLSYM_quit_extension       "klbapp_quit_extension"


/// @def   KLBAPPEX_DLSYM_extension_count
/// @brief 函数名: 动态库插件 支持的扩展数目
#define KLBAPPEX_DLSYM_extension_count      "klbapp_extension_count"

/// @def   KLBAPPEX_DLSYM_open_extension
/// @brief 函数名: 打开 扩展
#define KLBAPPEX_DLSYM_open_extension       "klbapp_open_extension"


/// @brief 动态库插件 初始化/退出
/// @return int 0.成功; 非0.失败
typedef int(*klbapp_init_extension_cb)();
typedef void(*klbapp_quit_extension_cb)();


/// @brief 获取 插件支持的 扩展数目
/// @return int 动态库插件支持的 扩展数目
typedef int(*klbapp_extension_count_cb)();


/// @brief 打开 第idx个 插件扩展
/// @param [in]     idx             第idx个扩展
/// @param [out]    *p_extension    app扩展接口
/// @param [out]    *p_name         扩展的名称
/// @param [in]     name_max        名称的缓存 最大长度
/// @return int 0.成功; 非0.失败
typedef int(*klbapp_open_extension_cb)(int idx, klb_app_extension_t* p_extension, char* p_name, int name_max);


/// @struct klbappex_ioctrl_t
/// @brief  扩展支持的 ioctrl 接口
///  klb app extension ioctrl
typedef struct klbappex_ioctrl_t_
{
    void*           ptr;                ///< 扩展指针

    /// @brief 自定义 直接操控函数
    /// @return int 0.成功; 非0.失败
    /// @note 若有需要与模块 通信, 扩展此系列函数
    ///    或者通过此接口函数 获取次级二级接口
    int(*ioctrl_opt0)(void* ptr, int opt);
    int(*ioctrl_opt1)(void* ptr, int opt, void* ptr1);
    int(*ioctrl_opt2)(void* ptr, int opt, void* ptr1, void* ptr2);
    int(*ioctrl_opt3)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3);
    int(*ioctrl_opt4)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4);
    int(*ioctrl_opt5)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5);
    int(*ioctrl_opt6)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6);
    int(*ioctrl_opt7)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7);
    int(*ioctrl_opt8)(void* ptr, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);
}klbappex_ioctrl_t;


/// @enum  klb_app_extension_msg_e
/// @brief app扩展 的 消息事件 定义
///   启动/退出 流程: init => setup => start ... => stop => quit
typedef enum klb_app_extension_msg_e_
{
    ///< 启动/退出 流程
    KLBAPPEX_MSG_init  = 1,            ///< 初始化
    KLBAPPEX_MSG_setup,                ///< 初始配置
    KLBAPPEX_MSG_start,                ///< 开启线程等
    KLBAPPEX_MSG_stop,                 ///< 关闭线程等
    KLBAPPEX_MSG_quit,                 ///< 退出
}klb_app_extension_msg_e;


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
    /// @param [in]     msg             消息事件: 详见 klb_app_extension_msg_e
    /// @param [in,out] *p_param_in_out 消息参数
    /// @param [in]     param_size      消息参数大小
    /// @return int 0
    /// @note 当框架需要扩展处理事务时调用
    int(*cb_control)(void* ptr, klb_app_t* p_app, int msg, uint8_t* p_param_in_out, int param_size);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_app          app对象
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_loop_once)(void* ptr, klb_app_t* p_app, int64_t now);

    /// @brief 获取 ioctrl 扩展支持功能
    /// @param [in]  *ptr           扩展的指针
    /// @param [out] *p_out_ioctrl  获取扩展功能接口
    /// @return int 0.成功; 非0.失败
    int (*cb_get_ioctrl)(void* ptr, klbappex_ioctrl_t* p_out_ioctrl);
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


//////////////////////////////////////////////////////////////////////////
// app extension ioctrl 扩展接口

/// @brief 注册app扩展的 ioctrl 接口
/// @param [in]  *p_app             app对象
/// @param [in]  *p_name            名称
/// @return klbappex_ioctrl_t* 指针
///   获取 ioctrl 接口后, 可以 通过通用预定义接口对接模块
KLB_API klbappex_ioctrl_t* klbappex_get_ioctrl(klb_app_t* p_app, const char* p_name);


/// @brief 注册app扩展的 ioctrl 接口
KLB_API int klbappex_ioctrl_opt0(klbappex_ioctrl_t* p_ioctrl, int opt);
KLB_API int klbappex_ioctrl_opt1(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1);
KLB_API int klbappex_ioctrl_opt2(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2);
KLB_API int klbappex_ioctrl_opt3(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3);
KLB_API int klbappex_ioctrl_opt4(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4);
KLB_API int klbappex_ioctrl_opt5(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5);
KLB_API int klbappex_ioctrl_opt6(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6);
KLB_API int klbappex_ioctrl_opt7(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7);
KLB_API int klbappex_ioctrl_opt8(klbappex_ioctrl_t* p_ioctrl, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);


//////////////////////////////////////////////////////////////////////////



#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_EXTENSION_H__
//end
