// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwui_widgets.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbwui 内部控件 type 与注册声明 (core 专用; 对外 API 见 ../klbwui.h)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建; embed_widgets 首批 kview/kbutton
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWUI_WIDGETS_H__
#define __KLBWUI_WIDGETS_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"


#if defined(__cplusplus)
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////
// embed_widgets — Lua type 字符串

#define KLBWUI_kbutton          "kbutton"
#define KLBWUI_kdemo            "kdemo"
#define KLBWUI_kpicture         "kpicture"
#define KLBWUI_kstatic          "kstatic"
#define KLBWUI_kview            "kview"


//////////////////////////////////////////////////////////////////////////
// embed_widgets — 注册函数

int klbui_register_kbutton(klb_gui_t* p_gui);
int klbui_register_kdemo(klb_gui_t* p_gui);
int klbui_register_kpicture(klb_gui_t* p_gui);
int klbui_register_kstatic(klb_gui_t* p_gui);
int klbui_register_kview(klb_gui_t* p_gui);


/// @brief 聚合注册 embed_widgets/ 下全部控件
#define KLBWUI_REGISTER_EMBED_WIDGETS(PTR_GUI_)     {   \
    klbui_register_kbutton((PTR_GUI_));                 \
    klbui_register_kdemo((PTR_GUI_));                   \
    klbui_register_kpicture((PTR_GUI_));                \
    klbui_register_kstatic((PTR_GUI_));                 \
    klbui_register_kview((PTR_GUI_));                   \
}


/// @brief 聚合注册 sim_widgets/ 下 PC 增补控件 (当前无)
#define KLBWUI_REGISTER_SIM_WIDGETS(PTR_GUI_)       {   \
}


#if defined(__cplusplus)
}
#endif


#endif // __KLBWUI_WIDGETS_H__
//end
