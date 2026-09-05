// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_util.h"
#include "klbmem/klb_mem.h"


#define KLB_GUIEX_util   "KLB-GUIEX-util"


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

    // 完全绘制
    p_util->is_redraw_full = true;

    // 当前鼠标位置
    {
        p_util->mouse_pt.x = 0;
        p_util->mouse_pt.y = 0;
    }

    // CSS
    {
        klb_map_init(&p_util->css_map);
        klb_map_init(&p_util->globalcss_map);
        klb_map_init(&p_util->globalcss_attr_map);
    }

    return p_util;
}

static void klbuiex_util_clear_globalcss_attr_map(klbuiex_util_t* p_util)
{
    klb_map_iter_t* p_iter = klb_map_begin(&p_util->globalcss_attr_map);

    // 遍历, 销毁CSS
    while (NULL != p_iter)
    {
        klb_adt_t* p_adt = klb_map_data(p_iter);

        const void* ptr = NULL;
        void* p_css = (void*)klb_adt_to_ptr(p_adt, &ptr);

        klb_gui_globalcss_destroy_cb cb_destroy = (klb_gui_globalcss_destroy_cb)ptr;

        if (NULL != p_css && NULL != cb_destroy)
        {
            cb_destroy(p_css);
        }

        p_iter = klb_map_next(p_iter);
    }

    klb_map_clear(&p_util->globalcss_attr_map);
}

static void klbuiex_util_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_util_t* p_util = (klbuiex_util_t*)ptr;

    // CSS
    {
        // p_util->globalcss_attr_map 存储的数据需要手动释放
        klbuiex_util_clear_globalcss_attr_map(p_util);

        klb_map_quit(&p_util->css_map);
        klb_map_quit(&p_util->globalcss_map);
        klb_map_quit(&p_util->globalcss_attr_map);
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
    return (klbuiex_util_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_util);
}

int klbuiex_register_util(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_util_create;
    ex.cb_destroy = klbuiex_util_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLB_GUIEX_util, &ex);

    return 0;
}
