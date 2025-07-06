///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_app.h
/// @brief   klb app, 主应用框架流程
///          主要目标:
///             1. 聚合 软件所有模块
///             2. 模块间 可以 通过统一的方式 相互感知使用
/// @version 0.2
/// @history 修改历史
///   \n [2025-4] 添加支持扩展机制: APP模块及扩展 均为 单例
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_APP_H__
#define __KLB_APP_H__

#include "klb_type.h"
#include "klbapp/klb_app_extension.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klb_app_t_ klb_app_t;


/// @brief app main 流程
/// @param [in]  argc           参数个数
/// @param [in]  **argv         参数字符串数组
/// @param [in]  cb_pre_load    所有 Lua 环境 的预加载库
/// @return int 0
///   调用次序: klb_base_init() => klb_app_main() => klb_base_quit()
KLB_API int klb_app_main(int argc, char** argv, lua_CFunction cb_pre_load);


/// @brief 获取app实例
/// @return klb_app_t* 实例指针
KLB_API klb_app_t* klb_app_instance();


//////////////////////////////////////////////////////////////////////////
// 以下函数 需要在 klb_app_main 函数之前配置完成

/// @brief 预加载函数
/// @return int 
typedef int(*klb_app_preload_cb)(klb_app_t* p_app);


/// @brief 预加载函数
KLB_API void klb_app_push_preload(klb_app_preload_cb cb_preload);


/// @brief 设置是否启用 (动态库)插件plugins
///   默认: 未启用
KLB_API void klb_app_enable_plugins(bool enable);


/// @brief 若启用(动态库)插件, 加载动态库插件的路径
KLB_API void klb_app_push_plugins_path(const char* p_path_plugins);


//////////////////////////////////////////////////////////////////////////


#if defined(__cplusplus)
}
#endif

#endif // __KLB_APP_H__
//end
