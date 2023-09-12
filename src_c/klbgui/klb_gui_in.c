// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbgui/extensions/klbuiex_shwnd.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// klbui_shwnd.h

int klb_gui_push_shwnd(klb_gui_t* p_gui, const char* p_path, klb_wnd_t* p_top_wnd)
{
    return klbuiex_shwnd_push(klbuiex_get_shwnd(p_gui), p_path, p_top_wnd);
}

klb_wnd_t* klb_gui_get_shwnd(klb_gui_t* p_gui, const char* p_path)
{
    return klbuiex_shwnd_get(klbuiex_get_shwnd(p_gui), p_path);
}

int klb_gui_shwnd_css_set(klb_gui_t* p_gui, const char* p_path, const klb_map_t* p_map)
{
    return klbuiex_shwnd_css_set(klbuiex_get_shwnd(p_gui), p_path, p_map);
}

klb_map_t* klb_gui_shwnd_css_get(klb_gui_t* p_gui, const char* p_path, const klb_map_t* p_map)
{
    return klbuiex_shwnd_css_get(klbuiex_get_shwnd(p_gui), p_path, p_map);
}
