///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    klua_env.h
/// @brief   文件简要描述
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
#include "klua/klua_cfg.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef struct klua_env_t klua_env_t;


KLB_API klua_env_t* klua_env_create(lua_CFunction cb_pre_load, char* p_main);
KLB_API void klua_env_destroy(klua_env_t* p_env);


//KLB_API int klua_env_dofile(klua_env_t* p_env, char* p_name);
//KLB_API int klua_env_dolibrary(klua_env_t* p_env, char* p_name);


KLB_API klua_env_t* klua_env_get_by_L(lua_State* L);
KLB_API lua_State*  klua_env_get_L(klua_env_t* p_env);


KLB_API int klua_env_call_kgo(klua_env_t* p_env);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_ENV_H__
//end
