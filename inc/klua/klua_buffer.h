///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2025, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klua_buffer.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
///  \n [2025-10] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLUA_BUFFER_H__
#define __KLUA_BUFFER_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbmem/klb_buf.h"
#include "klbmem/klb_bufpoolfix.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 获取固定内存池
/// @param [in]  *p_env         Lua环境 
/// @return klb_bufpoolfix_t* 
KLB_API klb_bufpoolfix_t* klua_bufpoolfix_get(klua_env_t* p_env);
KLB_API int klua_bufpoolfix_get2(klua_env_t* p_env, klb_bufagent_t* p_out);


/// @brief 获取固定内存池
/// @param [in]  *L             lua_State 
/// @return klb_bufpoolfix_t* 
KLB_API klb_bufpoolfix_t* klua_bufpoolfix_get_by_L(lua_State* L);
KLB_API int klua_bufpoolfix_get2_by_L(lua_State* L, klb_bufagent_t* p_out);


#ifdef __cplusplus
}
#endif

#endif // __KLUA_BUFFER_H__
//end
