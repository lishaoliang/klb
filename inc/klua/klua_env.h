///////////////////////////////////////////////////////////////////////////
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


#ifdef __cplusplus
}
#endif

#endif // __KLUA_ENV_H__
//end
