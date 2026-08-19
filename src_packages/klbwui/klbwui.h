// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwui.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klb 可选控件包 klbwui 对外 API (根目录仅暴露外部接口)
///            embed: embed_wnd / embed_widgets / embed_shwnd (ARM 主交付)
///            sim: 上述 embed 全部 + sim_wnd / sim_widgets / sim_shwnd (PC/wlua)
///            core: core/ (register, klua env extension; internal only)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建目录与注册桩
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWUI_H__
#define __KLBWUI_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @brief 注册 embed 层控件 (ARM 主交付; embed_wnd / embed_widgets / embed_shwnd)
/// @param [in]  *p_gui    GUI 根
/// @return int 0.成功; 非0.失败
KLB_API int klbwui_register_embed(klb_gui_t* p_gui);


/// @brief 注册 sim 层控件 (PC/wlua; 含 embed 全部 + sim_* 增补)
/// @param [in]  *p_gui    GUI 根
/// @return int 0.成功; 非0.失败
KLB_API int klbwui_register_sim(klb_gui_t* p_gui);


#define KLBWUI_REGISTER_EMBED(PTR_GUI_)     klbwui_register_embed((PTR_GUI_))
#define KLBWUI_REGISTER_SIM(PTR_GUI_)       klbwui_register_sim((PTR_GUI_))


#if defined(__cplusplus)
}
#endif


#endif // __KLBWUI_H__
//end
