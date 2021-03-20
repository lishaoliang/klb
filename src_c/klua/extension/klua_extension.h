///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2021, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_extension.h
/// @brief   标准lua扩展
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
///  \n https://gitee.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_EXTENSION_H__
#define __KLUA_EXTENSION_H__

#include "klb_type.h"
#include "klua/klua_env.h"

#if defined(__cplusplus)
extern "C" {
#endif


int klua_register_extension_std(klua_env_t* p_env);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_EXTENSION_H__
//end
