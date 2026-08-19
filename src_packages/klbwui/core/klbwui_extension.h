// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwui_extension.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbwui klua env 扩展 (core 内部)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建桩
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWUI_EXTENSION_H__
#define __KLBWUI_EXTENSION_H__


#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbgui/klb_gui.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBWUI_EXTENSION_NAME           "_KLBWUI_EXTENSION_"


typedef struct klbwui_extension_t_ klbwui_extension_t;


int kluaex_register_klbwui(klua_env_t* p_env);

klbwui_extension_t* kluaex_get_klbwui(klua_env_t* p_env);
klbwui_extension_t* kluaex_get_klbwui_by_L(lua_State* L);

klb_gui_t* kluaex_klbwui_get_gui(klbwui_extension_t* p_ex);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWUI_EXTENSION_H__
//end
