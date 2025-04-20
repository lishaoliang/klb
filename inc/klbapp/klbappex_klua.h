///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbappex_klua.h
/// @brief   klb app extension klua, klua扩展
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBAPPEX_KLUA_H__
#define __KLBAPPEX_KLUA_H__

#include "klb_type.h"
#include "klbapp/klb_app_extension.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbappex_klua_t
/// @brief  klb app extension klua 扩展
typedef struct klbappex_klua_t_ klbappex_klua_t;


/// @brief 注册 klua 扩展
/// @return int 0.成功; 非0.失败
KLB_API int klbappex_register_klua(klb_app_t* p_app);


/// @brief 获取 klua
/// @return klbappex_klua_t* 扩展的指针
KLB_API klbappex_klua_t* klbappex_get_klua(klb_app_t* p_app);
KLB_API klbappex_klua_t* klbappex_get_klua2();


/// @brief 设置klua扩展(主lua环境) 的 预加载库函数
/// @return int 0.成功; 非0.失败
KLB_API int klbappex_klua_set_preload(klbappex_klua_t* p_appex, lua_CFunction cb_pre_load);


/// @brief 获取Lua环境指针
/// @return klua_env_t* Lua环境 指针
KLB_API klua_env_t* klbappex_klua_get_klua_env(klbappex_klua_t* p_appex);


#if defined(__cplusplus)
}
#endif

#endif // __KLBAPPEX_KLUA_H__
//end
