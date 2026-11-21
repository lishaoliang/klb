// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwebrtc.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbwebrtc 对外入口 (SDP/PeerConnection 会话; 实现见 core/)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件(桩)
///  \n [2026-10] 0.2 会话编排落地; 绑定面由桩改为真实实现
/// @warning DTLS 与 SRTP 尚未接入会话
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWEBRTC_H__
#define __KLBWEBRTC_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库 "kwebrtc"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note 桩: peer_create 失败返回 nil, code
KLB_API int klua_open_kwebrtc(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLBWEBRTC_H__

// end
