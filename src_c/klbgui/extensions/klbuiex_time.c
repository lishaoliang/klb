#include "klbgui/extensions/klbuiex_time.h"
#include "klbmem/klb_mem.h"


#define KLBUIEX_TIME   "_KLBUIEX_TIME_"

typedef struct klbuiex_time_t_
{
    klb_gui_t*      p_gui;
}klbuiex_time_t;


//////////////////////////////////////////////////////////////////////////
// 


//////////////////////////////////////////////////////////////////////////
// 

static void* klbuiex_time_create(klb_gui_t* p_gui)
{
    klbuiex_time_t* p_time = KLB_MALLOCZ(klbuiex_time_t, 1, 0);

    p_time->p_gui = p_gui;

    return p_time;
}

static void klbuiex_time_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_time_t* p_time = (klbuiex_time_t*)ptr;

    KLB_FREE(p_time)
}

static int klbuiex_time_loop_once(void* ptr, klb_gui_t* p_gui, int64_t now)
{
    klbuiex_time_t* p_time = (klbuiex_time_t*)ptr;


    return 0;
}

//////////////////////////////////////////////////////////////////////////

int klbuiex_register_time(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_time_create;
    ex.cb_destroy = klbuiex_time_destroy;
    ex.cb_loop_once = klbuiex_time_loop_once;

    klb_gui_register_extension(p_gui, KLBUIEX_TIME, &ex);

    return 0;
}

klbuiex_time_t* klbuiex_get_time(klb_gui_t* p_gui)
{
    return (klbuiex_time_t*)klb_gui_get_extension(p_gui, KLBUIEX_TIME);
}
