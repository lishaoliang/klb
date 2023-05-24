// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_util.h"
#include "klbmem/klb_mem.h"


#define KLBUIEX_UTIL   "KLBUIEX-util"


//////////////////////////////////////////////////////////////////////////
// 前置定义


//////////////////////////////////////////////////////////////////////////
// create / destroy / loop once

static void* klbuiex_util_create(klb_gui_t* p_gui)
{
    klbuiex_util_t* p_util = KLB_MALLOCZ(klbuiex_util_t, 1, 0);

    p_util->p_gui = p_gui;

    // 日期时间
    {
        p_util->datefmt = KLBUI_DATE_YMD1;
        p_util->timefmt = KLBUI_TIME_24H;
    }

    // 当前鼠标位置
    {
        p_util->mouse_pt.x = 0;
        p_util->mouse_pt.y = 0;
    }

    // CSS
    {
        klb_map_init(&p_util->css_map);
    }

    return p_util;
}

static void klbuiex_util_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_util_t* p_util = (klbuiex_util_t*)ptr;

    // CSS
    {
        klb_map_quit(&p_util->css_map);
    }

    KLB_FREE(p_util)
}

//////////////////////////////////////////////////////////////////////////
// 私有


//////////////////////////////////////////////////////////////////////////
// 导出, export


//////////////////////////////////////////////////////////////////////////
// register / get


klbuiex_util_t* klbuiex_get_util(klb_gui_t* p_gui)
{
    return (klbuiex_util_t*)klb_gui_get_extension(p_gui, KLBUIEX_UTIL);
}

int klbuiex_register_util(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_util_create;
    ex.cb_destroy = klbuiex_util_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_UTIL, &ex);

    return 0;
}
