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
#include "lua.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_app_t_ klb_app_t;
typedef struct klb_app_extension_t_ klb_app_extension_t;


/// @def   KLBAPPEX_DL_name_max
/// @brief 动态库插件 回写扩展名的最大字符数(不含'\0')
/// @note  调用方缓冲区至少 name_max+1 字节
///        插件须保证 p_name 以'\0'结尾, 且 strlen(p_name) <= name_max
#define KLBAPPEX_DL_name_max                (64)


/// @def   KLBAPPEX_DL_init
/// @brief 函数名: 动态库插件 初始化/退出
#define KLBAPPEX_DL_init                    "klbappex_init"
#define KLBAPPEX_DL_quit                    "klbappex_quit"

/// @def   KLBAPPEX_DL_ex_count
/// @brief 函数名: 动态库插件 支持的 app 扩展数目
#define KLBAPPEX_DL_ex_count                "klbappex_ex_count"

/// @def   KLBAPPEX_DL_ex_open
/// @brief 函数名: 打开 app 扩展
#define KLBAPPEX_DL_ex_open                 "klbappex_ex_open"

/// @def   KLBAPPEX_DL_pre_count
/// @brief 函数名: 动态库插件 支持 klua 预加载函数的数目
#define KLBAPPEX_DL_pre_count               "klbappex_pre_count"

/// @def   KLBAPPEX_DL_pre_open
/// @brief 函数名: 按索引获取 klua 预加载 openlib (对称于 KLBAPPEX_DL_ex_open)
/// @note  框架内部行为 (klbappex_plugins.c, 本步不调用 openlib):
///   1. plugins_preload 阶段 klb_dlsym 取本符号, 须与 KLBAPPEX_DL_pre_count 成对
///   2. 循环 idx=0..pre_count()-1 调用 klbappex_pre_open; 成功项 klbappex_klua_push_preload 入 p_preload_nlist
///   3. 仅登记 (name, openlib) 指针, 不二次 dlopen, 于 klua_env_doinit 前完成
///   4. doinit 时 on_preload_klualib 对每项 klua_loadlib 写入 registry._PRELOAD
///   5. 入口 dofile 之后 require "name" 懒加载并执行 openlib
#define KLBAPPEX_DL_pre_open                "klbappex_pre_open"


/// @brief 动态库插件 初始化/退出
/// @return int 0.成功; 非0.失败
typedef int(*klbappex_init_cb)();
typedef void(*klbappex_quit_cb)();

/// @brief 获取 插件支持的 app 扩展数目
/// @return int 动态库插件支持的 扩展数目
typedef int(*klbappex_ex_count_cb)();

/// @brief 打开 第idx个 插件 app 扩展
/// @param [in]     idx             第idx个扩展
/// @param [out]    *p_extension    app扩展接口
/// @param [out]    *p_name         扩展的名称(以'\0'结尾)
/// @param [in]     name_max        名称缓存可写最大字符数(不含'\0'), 见 KLBAPPEX_DL_name_max
/// @return int 0.成功; 非0.失败
typedef int(*klbappex_ex_open_cb)(int idx, klb_app_extension_t* p_extension, char* p_name, int name_max);

/// @brief 获取 插件支持的 klua 预加载数目
/// @return int 动态库插件支持的 预加载回调数目
typedef int(*klbappex_pre_count_cb)();

/// @brief 按索引导出第 idx 个 klua 预加载项 (name + openlib)
/// @param [in]     idx             第 idx 个预加载项, 范围 [0, pre_count())
/// @param [out]    *p_out_preload  openlib 函数地址 (lua_CFunction)
/// @param [out]    *p_out_name     require 短名 (以'\0'结尾, 插件持有生命周期, 通常为静态串)
/// @return int 0.成功; 非0.失败
/// @note  对称于 klbappex_ex_open_cb: 框架枚举导出, 非 dlopen 亦非执行 openlib
///   见 KLBAPPEX_DL_pre_open @note: push 预加载链 -> doinit klua_loadlib -> require 懒加载
typedef int(*klbappex_pre_open_cb)(int idx, lua_CFunction* p_out_preload, const char** p_out_name);


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
///   启动/退出 流程: init => quit
typedef enum klb_app_extension_msg_e_
{
    ///< 启动/退出 流程
    KLBAPPEX_MSG_init  = 1,            ///< 初始化
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
