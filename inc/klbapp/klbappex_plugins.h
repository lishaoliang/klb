///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbappex_plugins.h
/// @brief   klb app extension plugins (动态库)插件管理
/// @version 0.1
/// @history 修改历史
/// @warning 无警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBAPPEX_PLUGINS_H__
#define __KLBAPPEX_PLUGINS_H__


#include "klb_type.h"
#include "klbapp/klb_app_extension.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbappex_plugins_t
/// @brief  klb app extension plugins 扩展
typedef struct klbappex_plugins_t_ klbappex_plugins_t;


/// @brief 获取 plugins 扩展
/// @return klbappex_plugins_t* 扩展的指针
KLB_API klbappex_plugins_t* klbappex_get_plugins(klb_app_t* p_app);
KLB_API klbappex_plugins_t* klbappex_get_plugins2();



#if defined(__cplusplus)
}
#endif

#endif // __KLBAPPEX_PLUGINS_H__
//end
