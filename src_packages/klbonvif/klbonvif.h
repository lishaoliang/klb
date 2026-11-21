// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbonvif.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbonvif 对外入口 (根目录仅暴露外部接口; 实现见 core/)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBONVIF_H__
#define __KLBONVIF_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库"konvif"
/// @param [in] *L          Lua状态
/// @return int 返回1
/// @note discover / discover_listen 已接 WS-Discovery 组播; connect / listen 仍为桩
KLB_API int klua_open_konvif(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBONVIF_H__

// end
