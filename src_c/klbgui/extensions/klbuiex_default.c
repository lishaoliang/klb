#include "klbgui/extensions/klbuiex_default.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"


#define KLBUIEX_DEFAULT   "_KLBUIEX_DEFAULT_"

typedef struct klbuiex_default_t_
{
    klb_gui_t*          p_gui;

    klbui_default_t     def;
}klbuiex_default_t;

//////////////////////////////////////////////////////////////////////////
// 

static void klbuiex_default_init(klbuiex_default_t* p_default)
{
    // ---------------------------------------------------
    // 文本 text
    p_default->def.text_color = KLB_ARGB8888(255, 255, 128, 0);


    // ---------------------------------------------------
    // 字体 font
    p_default->def.font_size = 24;


    // ---------------------------------------------------
    // 背景 background
    p_default->def.background_color = KLB_ARGB8888(255, 30, 30, 30);


    // ---------------------------------------------------
    // 边框 border
    p_default->def.border_width = 1;
    p_default->def.border_color = KLB_ARGB8888(255, 80, 80, 80);
    p_default->def.border_radius = 0;
}

static void klbuiex_default_quit(klbuiex_default_t* p_default)
{

}

//////////////////////////////////////////////////////////////////////////
// 


klbui_default_t* klbuiex_default_get_value(klbuiex_default_t* p_default)
{
    return &p_default->def;
}


//////////////////////////////////////////////////////////////////////////
// 

static void* klbuiex_default_create(klb_gui_t* p_gui)
{
    klbuiex_default_t* p_default = KLB_MALLOCZ(klbuiex_default_t, 1, 0);

    p_default->p_gui = p_gui;

    klbuiex_default_init(p_default);
    
    return p_default;
}

static void klbuiex_default_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_default_t* p_default = (klbuiex_default_t*)ptr;

    klbuiex_default_quit(p_default);

    KLB_FREE(p_default)
}

//////////////////////////////////////////////////////////////////////////

int klbuiex_register_default(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_default_create;
    ex.cb_destroy = klbuiex_default_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_DEFAULT, &ex);

    return 0;
}

klbuiex_default_t* klbuiex_get_default(klb_gui_t* p_gui)
{
    return (klbuiex_default_t*)klb_gui_get_extension(p_gui, KLBUIEX_DEFAULT);
}
