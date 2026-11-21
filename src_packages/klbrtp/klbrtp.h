// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbrtp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbrtp 对外入口 (RTP/RTCP 打包解包; 实现见 core/)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 桩: 接口未实现, 调用返回失败
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBRTP_H__
#define __KLBRTP_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库 "krtp"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note 桩: 打包接口返回 nil, code
KLB_API int klua_open_krtp(lua_State* L);


#if defined(__cplusplus)
}
#endif

#endif // __KLBRTP_H__

// end
