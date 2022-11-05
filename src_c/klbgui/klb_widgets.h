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


#if defined(__cplusplus)
extern "C" {
#endif


klb_wnd_t* klb_dialog_create(int x, int y, int w, int h);
klb_wnd_t* klbui_static_create(int x, int y, int w, int h);
klb_wnd_t* klbui_button_create(int x, int y, int w, int h);
klb_wnd_t* klbui_edit_create(int x, int y, int w, int h);
klb_wnd_t* klbui_check_create(int x, int y, int w, int h);
klb_wnd_t* klbui_combo_create(int x, int y, int w, int h);
klb_wnd_t* klbui_date_create(int x, int y, int w, int h);
klb_wnd_t* klbui_ip_create(int x, int y, int w, int h);
klb_wnd_t* klbui_picture_create(int x, int y, int w, int h);
klb_wnd_t* klbui_radio_create(int x, int y, int w, int h);
klb_wnd_t* klbui_slider_create(int x, int y, int w, int h);
klb_wnd_t* klbui_group_create(int x, int y, int w, int h);
klb_wnd_t* klbui_list_create(int x, int y, int w, int h);
klb_wnd_t* klbui_progress_create(int x, int y, int w, int h);
klb_wnd_t* klbui_rich_text_create(int x, int y, int w, int h);
klb_wnd_t* klbui_spin_create(int x, int y, int w, int h);
klb_wnd_t* klbui_tab_create(int x, int y, int w, int h);
klb_wnd_t* klbui_hscrollbar_create(int x, int y, int w, int h);
klb_wnd_t* klbui_vscrollbar_create(int x, int y, int w, int h);
klb_wnd_t* klbui_calendar_create(int x, int y, int w, int h);
klb_wnd_t* klbui_animation_create(int x, int y, int w, int h);



#define KLB_GUI_REGISTER_STD(PTR_GUI_)  { \
    klb_gui_register((PTR_GUI_), "kdialog",     klb_dialog_create);         \
    klb_gui_register((PTR_GUI_), "kstatic",     klbui_static_create);       \
    klb_gui_register((PTR_GUI_), "kbutton",     klbui_button_create);       \
    klb_gui_register((PTR_GUI_), "kedit",       klbui_edit_create);         \
    klb_gui_register((PTR_GUI_), "kcheck",      klbui_check_create);        \
    klb_gui_register((PTR_GUI_), "kcombo",      klbui_combo_create);        \
    klb_gui_register((PTR_GUI_), "kdate",       klbui_date_create);         \
    klb_gui_register((PTR_GUI_), "kip",         klbui_ip_create);           \
    klb_gui_register((PTR_GUI_), "kpicture",    klbui_picture_create);      \
    klb_gui_register((PTR_GUI_), "kradio",      klbui_radio_create);        \
    klb_gui_register((PTR_GUI_), "kslider",     klbui_slider_create);       \
    klb_gui_register((PTR_GUI_), "kgroup",      klbui_group_create);        \
    klb_gui_register((PTR_GUI_), "klist",       klbui_list_create);         \
    klb_gui_register((PTR_GUI_), "kprogress",   klbui_progress_create);     \
    klb_gui_register((PTR_GUI_), "krichtext",   klbui_rich_text_create);    \
    klb_gui_register((PTR_GUI_), "kspin",       klbui_spin_create);         \
    klb_gui_register((PTR_GUI_), "ktab",        klbui_tab_create);          \
    klb_gui_register((PTR_GUI_), "khscrollbar", klbui_hscrollbar_create);   \
    klb_gui_register((PTR_GUI_), "kvscrollbar", klbui_vscrollbar_create);   \
    klb_gui_register((PTR_GUI_), "kcalendar",   klbui_calendar_create);     \
    klb_gui_register((PTR_GUI_), "kanimation",  klbui_animation_create);    \
}


#ifdef __cplusplus
}
#endif


#endif // __KLB_WIDGET_H__
//end
