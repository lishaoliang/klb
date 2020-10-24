#ifndef __KLB_WIDGET_H__
#define __KLB_WIDGET_H__

#include "klb_type.h"
#include "gui/klb_wnd.h"
#include "gui/klb_gui.h"


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
