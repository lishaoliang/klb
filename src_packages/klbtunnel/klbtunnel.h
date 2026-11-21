// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbtunnel.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbtunnel 对外入口 (alua-tunnel; 根仅对外; 实现见 core/ tunnel_klua/)
/// @version 0.3
/// @history 修改历史
///  \n [2026] 创建文件 (桩; Lua 协议在 klbcore.klbtunnel)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBTUNNEL_H__
#define __KLBTUNNEL_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 扩展库 "ktunnel" (帧 / PSK SHA256-CTR wire)
/// @param [in] *L          Lua 状态
/// @return int 返回 1
KLB_API int klua_open_ktunnel(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif // __KLBTUNNEL_H__

// end
