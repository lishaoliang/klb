///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_widgets.h
/// @brief   窗口组件
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_WIDGETS_H__
#define __KLB_WIDGETS_H__

#include "klb_type.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_gui.h"


#define KLG_GUI_CHECK_CANVAS_RETRUN(CANVAS_) { \
    if (!(CANVAS_)) { return 1;} \
}

klb_wnd_t* klb_dialog_create(int x, int y, int w, int h);
klb_wnd_t* klb_button_create(int x, int y, int w, int h);


#define KLB_GUI_REGISTER_STD(P_GUI_) { \
    klb_gui_register((P_GUI_), "kdialog", klb_dialog_create); \
    klb_gui_register((P_GUI_), "kbutton", klb_button_create); \
}

#endif // __KLB_WIDGET_H__
//end
