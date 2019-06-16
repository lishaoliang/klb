///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/09
//
/// @file    lua_t_klb.h
/// @brief   文件简要描述
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
///  \n 2019/06/09 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __LUA_T_KLB_H__
#define __LUA_T_KLB_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

#include "lua.h"


/// @brief klb test函数
/// @param [in] *L  lua状态
/// @return int 1
KLB_API int luaopen_t_klb(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __LUA_T_KLB_H__
//end
