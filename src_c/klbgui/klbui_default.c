#include "klbgui/klbui_default.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"


void klbui_default_init(klb_gui_t* p_gui)
{
    p_gui->def.btn_image_normal = NULL;
    p_gui->def.btn_image_focus = NULL;
    p_gui->def.btn_image_disable = NULL;
}
