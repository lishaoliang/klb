#ifndef __KLB_CUIWIDGETS_HPP__
#define __KLB_CUIWIDGETS_HPP__

#include "klbgui/CGui.hpp"

namespace klbui {


#define KLBUI_cbutton           "cbutton"


int klbui_register_cbutton(klb_gui_t* p_gui);

/// @def   KLB_GUI_REGISTER_STD_CPP
/// @brief 注册标准GUI控件(cpp)
#define KLB_GUI_REGISTER_STD_CPP(PTR_GUI_)  {   \
    klbui_register_cbutton((PTR_GUI_));         \
}

} // klbui

#endif // __KLB_CUIWIDGETS_HPP__
//end
