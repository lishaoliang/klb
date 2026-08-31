// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwui_widgets.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbwui 内部控件 type 与注册声明 (core 专用; 对外 API 见 ../klbwui.h)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建;
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
// embed_widgets - Lua type 字符串

#define KLBWUI_kanimation       "kanimation"
#define KLBWUI_kbtnex           "kbtnex"
#define KLBWUI_kbutton          "kbutton"
#define KLBWUI_kcalendar        "kcalendar"
#define KLBWUI_kcheck           "kcheck"
#define KLBWUI_kcombo           "kcombo"
#define KLBWUI_kdate            "kdate"
#define KLBWUI_kdemo            "kdemo"
#define KLBWUI_kdialog          "kdialog"
#define KLBWUI_kdiv             "kdiv"
#define KLBWUI_kedit            "kedit"
#define KLBWUI_kgroup           "kgroup"
#define KLBWUI_khscrollbar      "khscrollbar"
#define KLBWUI_kip              "kip"
#define KLBWUI_kline            "kline"
#define KLBWUI_klist            "klist"
#define KLBWUI_klistex          "klistex"
#define KLBWUI_kmenu            "kmenu"
#define KLBWUI_knum             "knum"
#define KLBWUI_kpassword        "kpassword"
#define KLBWUI_kpicture         "kpicture"
#define KLBWUI_kprogress        "kprogress"
#define KLBWUI_kqrcode          "kqrcode"
#define KLBWUI_kradio           "kradio"
#define KLBWUI_krichtext        "krichtext"
#define KLBWUI_kslider          "kslider"
#define KLBWUI_kspin            "kspin"
#define KLBWUI_kstatic          "kstatic"
#define KLBWUI_ktab             "ktab"
#define KLBWUI_kticker          "kticker"
#define KLBWUI_ktime            "ktime"
#define KLBWUI_kview            "kview"
#define KLBWUI_kvscrollbar      "kvscrollbar"
#define KLBWUI_kvslider         "kvslider"


//////////////////////////////////////////////////////////////////////////
// embed_widgets - 注册函数

int klbui_register_kanimation(klb_gui_t* p_gui);
int klbui_register_kbtnex(klb_gui_t* p_gui);
int klbui_register_kbutton(klb_gui_t* p_gui);
int klbui_register_kcalendar(klb_gui_t* p_gui);
int klbui_register_kcheck(klb_gui_t* p_gui);
int klbui_register_kcombo(klb_gui_t* p_gui);
int klbui_register_kdate(klb_gui_t* p_gui);
int klbui_register_kdemo(klb_gui_t* p_gui);
int klbui_register_kdialog(klb_gui_t* p_gui);
int klbui_register_kdiv(klb_gui_t* p_gui);
int klbui_register_kedit(klb_gui_t* p_gui);
int klbui_register_kgroup(klb_gui_t* p_gui);
int klbui_register_khscrollbar(klb_gui_t* p_gui);
int klbui_register_kip(klb_gui_t* p_gui);
int klbui_register_kline(klb_gui_t* p_gui);
int klbui_register_klist(klb_gui_t* p_gui);
int klbui_register_klistex(klb_gui_t* p_gui);
int klbui_register_kmenu(klb_gui_t* p_gui);
int klbui_register_knum(klb_gui_t* p_gui);
int klbui_register_kpassword(klb_gui_t* p_gui);
int klbui_register_kpicture(klb_gui_t* p_gui);
int klbui_register_kprogress(klb_gui_t* p_gui);
int klbui_register_kqrcode(klb_gui_t* p_gui);
int klbui_register_kradio(klb_gui_t* p_gui);
int klbui_register_krichtext(klb_gui_t* p_gui);
int klbui_register_kslider(klb_gui_t* p_gui);
int klbui_register_kspin(klb_gui_t* p_gui);
int klbui_register_kstatic(klb_gui_t* p_gui);
int klbui_register_ktab(klb_gui_t* p_gui);
int klbui_register_kticker(klb_gui_t* p_gui);
int klbui_register_ktime(klb_gui_t* p_gui);
int klbui_register_kview(klb_gui_t* p_gui);
int klbui_register_kvscrollbar(klb_gui_t* p_gui);
int klbui_register_kvslider(klb_gui_t* p_gui);


/// @brief 聚合注册 embed_widgets/ 下全部控件
#define KLBWUI_REGISTER_EMBED_WIDGETS(PTR_GUI_)     {   \
    klbui_register_kanimation((PTR_GUI_));              \
    klbui_register_kbtnex((PTR_GUI_));                  \
    klbui_register_kbutton((PTR_GUI_));                 \
    klbui_register_kcalendar((PTR_GUI_));               \
    klbui_register_kcheck((PTR_GUI_));                  \
    klbui_register_kcombo((PTR_GUI_));                  \
    klbui_register_kdate((PTR_GUI_));                   \
    klbui_register_kdemo((PTR_GUI_));                   \
    klbui_register_kdialog((PTR_GUI_));                 \
    klbui_register_kdiv((PTR_GUI_));                    \
    klbui_register_kedit((PTR_GUI_));                   \
    klbui_register_kgroup((PTR_GUI_));                  \
    klbui_register_khscrollbar((PTR_GUI_));             \
    klbui_register_kip((PTR_GUI_));                     \
    klbui_register_kline((PTR_GUI_));                   \
    klbui_register_klist((PTR_GUI_));                   \
    klbui_register_klistex((PTR_GUI_));                 \
    klbui_register_kmenu((PTR_GUI_));                   \
    klbui_register_knum((PTR_GUI_));                    \
    klbui_register_kpassword((PTR_GUI_));               \
    klbui_register_kpicture((PTR_GUI_));                \
    klbui_register_kprogress((PTR_GUI_));               \
    klbui_register_kqrcode((PTR_GUI_));                 \
    klbui_register_kradio((PTR_GUI_));                  \
    klbui_register_krichtext((PTR_GUI_));               \
    klbui_register_kslider((PTR_GUI_));                 \
    klbui_register_kspin((PTR_GUI_));                   \
    klbui_register_kstatic((PTR_GUI_));                 \
    klbui_register_ktab((PTR_GUI_));                    \
    klbui_register_kticker((PTR_GUI_));                 \
    klbui_register_ktime((PTR_GUI_));                   \
    klbui_register_kview((PTR_GUI_));                   \
    klbui_register_kvscrollbar((PTR_GUI_));             \
    klbui_register_kvslider((PTR_GUI_));                \
}


/// @brief 聚合注册 sim_widgets/ 下 PC 增补控件 (当前无)
#define KLBWUI_REGISTER_SIM_WIDGETS(PTR_GUI_)       {   \
}


#if defined(__cplusplus)
}
#endif


#endif // __KLBWUI_WIDGETS_H__

// end
