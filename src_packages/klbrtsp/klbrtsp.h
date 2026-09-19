// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbrtsp.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可裁剪包 klbrtsp 对外入口 (根目录仅暴露外部接口; 实现见 core/)
/// @version 0.1
/// @history 修改历史
///  \n [2026] 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBRTSP_H__
#define __KLBRTSP_H__

#include "klb_type.h"
#include "klua/klua.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 扩展库"krtsp"
/// @param [in] *L          Lua状态
/// @return int 返回1
KLB_API int klua_open_krtsp(lua_State* L);


#ifdef __cplusplus
}
#endif

#endif // __KLBRTSP_H__

// end
