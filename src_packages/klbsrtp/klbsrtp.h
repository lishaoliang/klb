// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbsrtp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbsrtp 对外入口 (SRTP/SRTCP 包保护; 实现见 core/)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
///  \n [2026-10] 0.2 ksrtp 绑定面实现 (上下文工厂 + protect/unprotect)
/// @warning 仅 AES128_CM_HMAC_SHA1_80
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSRTP_H__
#define __KLBSRTP_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库 "ksrtp"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note 桩: create 失败返回 nil, code
KLB_API int klua_open_ksrtp(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSRTP_H__

// end
