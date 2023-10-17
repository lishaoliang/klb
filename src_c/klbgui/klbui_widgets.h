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


#define KLBUI_kstatic           "kstatic"
#define KLBUI_kbutton           "kbutton"
#define KLBUI_kbtnex            "kbtnex"
#define KLBUI_kedit             "kedit"
#define KLBUI_kpassword         "kpassword"
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


int klbui_register_kstatic(klb_gui_t* p_gui);
int klbui_register_kbutton(klb_gui_t* p_gui);
int klbui_register_kbtnex(klb_gui_t* p_gui);
int klbui_register_kedit(klb_gui_t* p_gui);
int klbui_register_kpassword(klb_gui_t* p_gui);
int klbui_register_knum(klb_gui_t* p_gui);
int klbui_register_kcheck(klb_gui_t* p_gui);
int klbui_register_kcombo(klb_gui_t* p_gui);
int klbui_register_kdate(klb_gui_t* p_gui);
int klbui_register_ktime(klb_gui_t* p_gui);
int klbui_register_kip(klb_gui_t* p_gui);
int klbui_register_kpicture(klb_gui_t* p_gui);
int klbui_register_kradio(klb_gui_t* p_gui);
int klbui_register_kslider(klb_gui_t* p_gui);
int klbui_register_kvslider(klb_gui_t* p_gui);
int klbui_register_kgroup(klb_gui_t* p_gui);
int klbui_register_klist(klb_gui_t* p_gui);
int klbui_register_klistex(klb_gui_t* p_gui);
int klbui_register_kprogress(klb_gui_t* p_gui);
int klbui_register_krichtext(klb_gui_t* p_gui);
int klbui_register_kspin(klb_gui_t* p_gui);
int klbui_register_ktab(klb_gui_t* p_gui);
int klbui_register_kview(klb_gui_t* p_gui);
int klbui_register_khscrollbar(klb_gui_t* p_gui);
int klbui_register_kvscrollbar(klb_gui_t* p_gui);
int klbui_register_kcalendar(klb_gui_t* p_gui);
int klbui_register_kanimation(klb_gui_t* p_gui);
int klbui_register_kqrcode(klb_gui_t* p_gui);
int klbui_register_kmenu(klb_gui_t* p_gui);


/// @def   KLB_GUI_REGISTER_STD
/// @brief 注册标准GUI控件
#define KLB_GUI_REGISTER_STD(PTR_GUI_)  {       \
    klbui_register_kstatic((PTR_GUI_));         \
    klbui_register_kbutton((PTR_GUI_));         \
    klbui_register_kbtnex((PTR_GUI_));          \
    klbui_register_kedit((PTR_GUI_));           \
    klbui_register_kpassword((PTR_GUI_));       \
    klbui_register_knum((PTR_GUI_));            \
    klbui_register_kcheck((PTR_GUI_));          \
    klbui_register_kcombo((PTR_GUI_));          \
    klbui_register_kdate((PTR_GUI_));           \
    klbui_register_ktime((PTR_GUI_));           \
    klbui_register_kip((PTR_GUI_));             \
    klbui_register_kpicture((PTR_GUI_));        \
    klbui_register_kradio((PTR_GUI_));          \
    klbui_register_kslider((PTR_GUI_));         \
    klbui_register_kvslider((PTR_GUI_));        \
    klbui_register_kgroup((PTR_GUI_));          \
    klbui_register_klist((PTR_GUI_));           \
    klbui_register_klistex((PTR_GUI_));         \
    klbui_register_kprogress((PTR_GUI_));       \
    klbui_register_krichtext((PTR_GUI_));       \
    klbui_register_kspin((PTR_GUI_));           \
    klbui_register_ktab((PTR_GUI_));            \
    klbui_register_kview((PTR_GUI_));           \
    klbui_register_khscrollbar((PTR_GUI_));     \
    klbui_register_kvscrollbar((PTR_GUI_));     \
    klbui_register_kcalendar((PTR_GUI_));       \
    klbui_register_kanimation((PTR_GUI_));      \
    klbui_register_kqrcode((PTR_GUI_));         \
    klbui_register_kmenu((PTR_GUI_));           \
}


#ifdef __cplusplus
}
#endif


#endif // __KLB_WIDGET_H__
//end
