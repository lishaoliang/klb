// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbice.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbice 对外入口 (ICE/STUN/TURN; 实现见 core/)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
///  \n [2026-10] 0.2 kice 绑定面实现 (agent 工厂 + userdata 方法)
/// @warning TURN 客户端为最小实现; IPv6 候选未接入
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBICE_H__
#define __KLBICE_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库 "kice"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note 桩: create 失败返回 nil, code
KLB_API int klua_open_kice(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLBICE_H__

// end
