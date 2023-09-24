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
#include "klbgui/klbui_event.h"


#if defined(__cplusplus)
extern "C" {
#endif


void klbui_static_init_globalcss(klb_gui_t* p_gui);
void klbui_button_init_globalcss(klb_gui_t* p_gui);
void klbui_btnex_init_globalcss(klb_gui_t* p_gui);
void klbui_edit_init_globalcss(klb_gui_t* p_gui);
void klbui_num_init_globalcss(klb_gui_t* p_gui);
void klbui_check_init_globalcss(klb_gui_t* p_gui);
void klbui_combo_init_globalcss(klb_gui_t* p_gui);
void klbui_date_init_globalcss(klb_gui_t* p_gui);
void klbui_time_init_globalcss(klb_gui_t* p_gui);
void klbui_ip_init_globalcss(klb_gui_t* p_gui);
void klbui_picture_init_globalcss(klb_gui_t* p_gui);
void klbui_view_init_globalcss(klb_gui_t* p_gui);
void klbui_qrcode_init_globalcss(klb_gui_t* p_gui);


klb_wnd_t* klbui_static_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_button_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_btnex_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_edit_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_num_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_check_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_combo_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_date_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_time_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_ip_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_picture_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_radio_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_slider_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_vslider_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_group_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_list_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_listex_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_progress_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_rich_text_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_spin_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_view_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_hscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_animation_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h);
klb_wnd_t* klbui_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h);


#define KLBUI_kstatic           "kstatic"
#define KLBUI_kbutton           "kbutton"
#define KLBUI_kbtnex            "kbtnex"
#define KLBUI_kedit             "kedit"
#define KLBUI_knum              "knum"
#define KLBUI_kcheck            "kcheck"
#define KLBUI_kcombo            "kcombo"
#define KLBUI_kdate             "kdate"
#define KLBUI_ktime             "ktime"
#define KLBUI_kip               "kip"
#define KLBUI_kpicture          "kpicture"
#define KLBUI_kradio            "kradio"
#define KLBUI_kslider           "kslider"
#define KLBUI_kvslider          "kvslider"
#define KLBUI_kgroup            "kgroup"
#define KLBUI_klist             "klist"
#define KLBUI_klistex           "klistex"
#define KLBUI_kprogress         "kprogress"
#define KLBUI_krichtext         "krichtext"
#define KLBUI_kspin             "kspin"
#define KLBUI_ktab              "ktab"
#define KLBUI_kview             "kview"
#define KLBUI_khscrollbar       "khscrollbar"
#define KLBUI_kvscrollbar       "kvscrollbar"
#define KLBUI_kcalendar         "kcalendar"
#define KLBUI_kanimation        "kanimation"
#define KLBUI_kqrcode           "kqrcode"
#define KLBUI_kmenu             "kmenu"


/// @def   KLB_GUI_REGISTER_STD
/// @brief 注册标准GUI控件
#define KLB_GUI_REGISTER_STD(PTR_GUI_)  { \
    klb_gui_register((PTR_GUI_), KLBUI_kstatic,     klbui_static_create);       \
    klb_gui_register((PTR_GUI_), KLBUI_kbutton,     klbui_button_create);       \
    klb_gui_register((PTR_GUI_), KLBUI_kbtnex,      klbui_btnex_create);        \
    klb_gui_register((PTR_GUI_), KLBUI_kedit,       klbui_edit_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_knum,        klbui_num_create);          \
    klb_gui_register((PTR_GUI_), KLBUI_kcheck,      klbui_check_create);        \
    klb_gui_register((PTR_GUI_), KLBUI_kcombo,      klbui_combo_create);        \
    klb_gui_register((PTR_GUI_), KLBUI_kdate,       klbui_date_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_ktime,       klbui_time_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_kip,         klbui_ip_create);           \
    klb_gui_register((PTR_GUI_), KLBUI_kpicture,    klbui_picture_create);      \
    klb_gui_register((PTR_GUI_), KLBUI_kradio,      klbui_radio_create);        \
    klb_gui_register((PTR_GUI_), KLBUI_kslider,     klbui_slider_create);       \
    klb_gui_register((PTR_GUI_), KLBUI_kvslider,    klbui_vslider_create);      \
    klb_gui_register((PTR_GUI_), KLBUI_kgroup,      klbui_group_create);        \
    klb_gui_register((PTR_GUI_), KLBUI_klist,       klbui_list_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_klistex,     klbui_listex_create);       \
    klb_gui_register((PTR_GUI_), KLBUI_kprogress,   klbui_progress_create);     \
    klb_gui_register((PTR_GUI_), KLBUI_krichtext,   klbui_rich_text_create);    \
    klb_gui_register((PTR_GUI_), KLBUI_kspin,       klbui_spin_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_ktab,        klbui_tab_create);          \
    klb_gui_register((PTR_GUI_), KLBUI_kview,       klbui_view_create);         \
    klb_gui_register((PTR_GUI_), KLBUI_khscrollbar, klbui_hscrollbar_create);   \
    klb_gui_register((PTR_GUI_), KLBUI_kvscrollbar, klbui_vscrollbar_create);   \
    klb_gui_register((PTR_GUI_), KLBUI_kcalendar,   klbui_calendar_create);     \
    klb_gui_register((PTR_GUI_), KLBUI_kanimation,  klbui_animation_create);    \
    klb_gui_register((PTR_GUI_), KLBUI_kqrcode,     klbui_qrcode_create);       \
    klb_gui_register((PTR_GUI_), KLBUI_kmenu,       klbui_menu_create);         \
    klbui_static_init_globalcss((PTR_GUI_));    \
    klbui_button_init_globalcss((PTR_GUI_));    \
    klbui_btnex_init_globalcss((PTR_GUI_));     \
    klbui_edit_init_globalcss((PTR_GUI_));      \
    klbui_num_init_globalcss((PTR_GUI_));       \
    klbui_check_init_globalcss((PTR_GUI_));     \
    klbui_combo_init_globalcss((PTR_GUI_));     \
    klbui_date_init_globalcss((PTR_GUI_));      \
    klbui_time_init_globalcss((PTR_GUI_));      \
    klbui_ip_init_globalcss((PTR_GUI_));        \
    klbui_picture_init_globalcss((PTR_GUI_));   \
    klbui_view_init_globalcss((PTR_GUI_));      \
    klbui_qrcode_init_globalcss((PTR_GUI_));    \
}


#ifdef __cplusplus
}
#endif


#endif // __KLB_WIDGET_H__
//end
