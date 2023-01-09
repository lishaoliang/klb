///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_tab_button.h
/// @brief   "ktab"的私有子控件
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_TAB_BUTTON_H__
#define __KLBUI_TAB_BUTTON_H__


#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct klbui_tab_botton_t_
{
    int a;
}klbui_tab_botton_t;


klb_wnd_t* klbui_tab_botton_create(klb_gui_t* p_gui, int x, int y, int w, int h);




#ifdef __cplusplus
}
#endif

#endif // __KLBUI_TAB_BUTTON_H__
//end
