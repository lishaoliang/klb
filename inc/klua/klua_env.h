﻿///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua_env.h
/// @brief   lua_State简易封装
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/30 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_ENV_H__
#define __KLUA_ENV_H__

#include "klb_type.h"
#include "klbthird/sds.h"
#include "klua/klua_data.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef struct klua_env_t klua_env_t;


/// @brief 创建一个lua环境
/// @param [in] cb_pre_load       预加载额外库函数
/// @return klua_env_t* lua环境
KLB_API klua_env_t* klua_env_create(lua_CFunction cb_pre_load);


/// @brief 销毁lua环境
/// @param [in] *p_env             lua环境 
/// @return 无
KLB_API void klua_env_destroy(klua_env_t* p_env);


/// @brief 设置用户数据指针(user data)
/// @param [in] *p_env             lua环境
/// @param [in] *p_udata           用户数据指针(user data)
/// @return 无
KLB_API void klua_env_set_udata(klua_env_t* p_env, void* p_udata);


/// @brief 获取用户数据指针(user data)
/// @param [in] *p_env             lua环境
/// @return void* 用户数据指针
KLB_API void* klua_env_get_udata(klua_env_t* p_env);


/// @brief 获取当前lua环境滴答数
/// @param [in] *p_env             lua环境 
/// @return int64_t tc
KLB_API int64_t klua_env_get_tc(klua_env_t* p_env);


/// @brief 按路径方式加载一个脚本文件
/// @param [in] *p_env             lua环境
/// @param [in] *p_entry           入口lua文件路径; eg. "./test.lua"
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_dofile(klua_env_t* p_env, const char* p_entry);


/// @brief 按库方式加载一个脚本文件
/// @param [in] *p_env             lua环境
/// @param [in] *p_entry           入口lua文件; eg. "server.entry_gui"
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_dolibrary(klua_env_t* p_env, const char* p_entry);


/// @brief 结束dofile/dolibrary
/// @param [in] *p_env             lua环境
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_doend(klua_env_t* p_env);


/// @brief 从lua_State*获取klua_env_t*指针
/// @param [in] *L             L
/// @return klua_env_t* lua环境
KLB_API klua_env_t* klua_env_get_by_L(lua_State* L);


/// @brief 获取原始lua_State*
/// @param [in] *p_env             lua环境
/// @return lua_State* L
KLB_API lua_State* klua_env_get_L(klua_env_t* p_env);


/// @brief 脚本中是否有"kgo"函数
/// @param [in] *p_env             lua环境
/// @return 0.有; 非0.无
KLB_API int klua_env_has_kgo(klua_env_t* p_env);


/// @brief 脚本中"kgo"函数的引用值
/// @param [in] *p_env             lua环境
/// @return 0.无引用值; 大于0.引用值
KLB_API int klua_env_kgo(klua_env_t* p_env);


/// @brief 调用脚本中的"kgo"函数
/// @param [in] *p_env              lua环境
/// @param [in] *p_msg              消息
/// @param [in] *p_msgex            扩展消息
/// @param [in] *p_lparam           参数1
/// @param [in] *p_wparam           参数2
/// @param [in] *ptr                C指针
/// @return int 0.成功; 非0.失败
KLB_API int klua_env_call_kgo(klua_env_t* p_env, const char* p_msg, const char* p_msgex, const char* p_lparam, const char* p_wparam, void* ptr);


/// @brief 报错
/// @param [in] *p_env              lua环境
/// @param [in] status              非0时, 报错
/// @return int status
KLB_API int klua_env_report(klua_env_t* p_env, int status);


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
    void  (*cb_destroy)(void* ptr);

    /// @brief 对扩展直接控制设置
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] *p_param        控制参数
    /// @return klua_ctrlex_msg_t* 返回信息
    /// @note eg. 设置扩展的参数, 获取扩展的参数等.
    klua_ctrlex_msg_t* (*cb_ctrlex)(void* ptr, klua_env_t* p_env, klua_ctrlex_msg_t* p_param);

    /// @brief 消息处理
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] now             当前滴答数
    /// @return int 0
    int   (*cb_msg)(void* ptr, klua_env_t* p_env, int64_t now, klua_msg_t* p_msg);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_env          lua环境
    /// @param [in] last_tc         上一次的滴答数
    /// @param [in] now             当前滴答数
    /// @return int 0
    int   (*cb_loop_once)(void* ptr, klua_env_t* p_env, int64_t last_tc, int64_t now);
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


/// @brief 调用一次lua环境; 需要定期调用
/// @param [in] *p_env              lua环境
/// @return int 0
KLB_API int klua_env_loop_once(klua_env_t* p_env);


/// @brief 是否退出
/// @param [in] *p_env              lua环境
/// @return bool true.退出; false.不退出
KLB_API bool klua_env_is_exit(klua_env_t* p_env);


/// @brief 设置名称
/// @param [in] *p_env              lua环境
/// @return 无
KLB_API void klua_env_set_name(klua_env_t* p_env, const char* p_name, size_t name_len);


/// @brief 获取名称
/// @param [in] *p_env              lua环境
/// @return sds 名称
KLB_API const sds klua_env_get_name(klua_env_t* p_env);


/// @brief 设置获取消息标记
/// @param [in] *p_env              lua环境
/// @return sds 名称
KLB_API void klua_env_set_msg_flag(klua_env_t* p_env);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_ENV_H__
//end
