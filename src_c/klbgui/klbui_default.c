#include "klbgui/klbui_default.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"


void klbui_default_init(klb_gui_t* p_gui)
{
    p_gui->def.text_color = KLB_ARGB8888(255, 255, 128, 0);
    p_gui->def.background_color = KLB_ARGB8888(255, 30, 30, 30);
}

void klbui_default_quit(klb_gui_t* p_gui)
{

}
