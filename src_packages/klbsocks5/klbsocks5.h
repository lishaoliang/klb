// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbsocks5.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbsocks5 对外入口 (根仅对外; 实现见 core/ socks5_klua/)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件 (桩; Lua 协议在 klbcore.klbsocks5)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSOCKS5_H__
#define __KLBSOCKS5_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 扩展库 "ksocks5" (RFC1928/1929 wire 编解码)
/// @param [in] *L          Lua 状态
/// @return int 返回 1
KLB_API int klua_open_ksocks5(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif // __KLBSOCKS5_H__

// end
