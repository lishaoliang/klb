#ifndef __KLBUI_CUIWIDGETS_HPP__
#define __KLBUI_CUIWIDGETS_HPP__

#include "klbgui/CGui.hpp"

namespace klbui {

#if 0

//////////////////////////////////////////////////////////////////////////
// 控件名称/TID


#define KLBUI_cdemo             "cdemo"
#define KLBUI_clabel            "clabel"
#define KLBUI_cbutton           "cbutton"
#define KLBUI_cticker           "cticker"
#define KLBUI_ctimer            "ctimer"


/// @enum  klbui_cpp_widgets_tid_e
/// @brief CPP 控件部分 tid 定义值, [1000, 4000]
typedef enum klbui_widgets_tid_e_
{
    KLBUI_cppbase_tid           = 1000,
    KLBUI_cdemo_tid,
    KLBUI_clabel_tid,
    KLBUI_cbutton_tid,
    KLBUI_cticker_tid,
    KLBUI_ctimer_tid,
}klbui_cpp_widgets_tid_e;


//////////////////////////////////////////////////////////////////////////
// 定义函数


int klbui_register_cdemo(klb_gui_t* p_gui);
int klbui_register_clabel(klb_gui_t* p_gui);
int klbui_register_cbutton(klb_gui_t* p_gui);
int klbui_register_cticker(klb_gui_t* p_gui);
int klbui_register_ctimer(klb_gui_t* p_gui);


//////////////////////////////////////////////////////////////////////////
// 注册宏

/// @def   KLB_GUI_REGISTER_STD_CPP
/// @brief 注册标准GUI控件(cpp)
#define KLB_GUI_REGISTER_STD_CPP(PTR_GUI_)  {   \
    klbui_register_cdemo((PTR_GUI_));           \
    klbui_register_clabel((PTR_GUI_));          \
    klbui_register_cbutton((PTR_GUI_));         \
    klbui_register_cticker((PTR_GUI_));         \
    klbui_register_ctimer((PTR_GUI_));          \
}

#endif

} // namespace klbui

#endif // __KLBUI_CUIWIDGETS_HPP__
//end
