#include "klbgui/klbui_default.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"
#include "klbgui/klbui_css.h"


void klbui_default_init(klb_gui_t* p_gui)
{
    // ---------------------------------------------------
    // 文本 text
    p_gui->def.text_color = KLB_ARGB8888(255, 255, 128, 0);


    // ---------------------------------------------------
    // 字体 font
    p_gui->def.font_size = 24;


    // ---------------------------------------------------
    // 背景 background
    p_gui->def.background_color = KLB_ARGB8888(255, 30, 30, 30);


    // ---------------------------------------------------
    // 边框 border
    p_gui->def.border_width = 1;
    p_gui->def.border_color = KLB_ARGB8888(255, 80, 80, 80);
    p_gui->def.border_radius = 0;
}

void klbui_default_quit(klb_gui_t* p_gui)
{

}
