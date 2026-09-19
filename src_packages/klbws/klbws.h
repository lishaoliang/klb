// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbws.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbws 对外入口 (根目录仅暴露外部接口; 实现见 core/)
/// @version 0.2
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWS_H__
#define __KLBWS_H__


#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief 扩展库"kws"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_kws(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBWS_H__

// end
