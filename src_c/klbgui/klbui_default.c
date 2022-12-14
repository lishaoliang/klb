#include "klbgui/klbui_default.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"


void klbui_default_init(klb_gui_t* p_gui)
{
    p_gui->def.btn_image_normal = NULL;
    p_gui->def.btn_image_focus = NULL;
    p_gui->def.btn_image_disable = NULL;
}

void klbui_default_quit(klb_gui_t* p_gui)
{
    KLB_FREE_BY(p_gui->def.btn_image_normal, sdsfree);
    KLB_FREE_BY(p_gui->def.btn_image_focus, sdsfree);
    KLB_FREE_BY(p_gui->def.btn_image_disable, sdsfree);
}
