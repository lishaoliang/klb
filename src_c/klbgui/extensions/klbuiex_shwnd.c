// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_time.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_hlist.h"
#include <string.h>


#define KLBUIEX_SHWND   "_KLBUIEX_SHWND_"


typedef struct klbuiex_shwnd_t_
{
    klb_gui_t*      p_gui;          ///< GUI

    klb_hlist_t*    p_hlist;        ///< 存储所有顶层窗口: klb_wnd_t*
}klbuiex_shwnd_t;


//////////////////////////////////////////////////////////////////////////
// on

static int cb_clear_top_wnd_klbuiex_shwnd(void* p_obj, void* p_data)
{
    klbuiex_shwnd_t* p_shwnd = (klbuiex_shwnd_t*)p_obj;
    klb_wnd_t* p_top_wnd = (klb_wnd_t*)p_data;

    // 释放顶层窗口
    // TODO. 释放窗口树
    KLB_FREE_WND(p_top_wnd);

    return 0;
}

static void on_klbuiex_shwnd_clear(klbuiex_shwnd_t* p_shwnd)
{
    klb_hlist_clear(p_shwnd->p_hlist, cb_clear_top_wnd_klbuiex_shwnd, p_shwnd);
}


//////////////////////////////////////////////////////////////////////////
// create / destroy

static void* klbuiex_shwnd_create(klb_gui_t* p_gui)
{
    klbuiex_shwnd_t* p_shwnd = KLB_MALLOCZ(klbuiex_shwnd_t, 1, 0);

    p_shwnd->p_gui = p_gui;
    p_shwnd->p_hlist = klb_hlist_create(0);

    return p_shwnd;
}

static void klbuiex_shwnd_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_shwnd_t* p_shwnd = (klbuiex_shwnd_t*)ptr;

    // 释放窗口
    on_klbuiex_shwnd_clear(p_shwnd);

    KLB_FREE_BY(p_shwnd->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_shwnd)
}

/// @brief 控制操作消息
static int klbuiex_shwnd_control(void* ptr, klb_gui_t* p_gui, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbuiex_shwnd_t* p_shwnd = (klbuiex_shwnd_t*)ptr;

    switch (msg)
    {
    case KLBUI_EX_MSG_clear:
        on_klbuiex_shwnd_clear(p_shwnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export

int klbuiex_shwnd_push(klbuiex_shwnd_t* p_shwnd, const char* p_path, klb_wnd_t* p_top_wnd)
{
    assert(NULL != p_shwnd);
    assert(NULL != p_path);
    assert(NULL != p_top_wnd);

    int len = strlen(p_path);

    if (NULL != klb_hlist_find_iter(p_shwnd->p_hlist, p_path, len))
    {
        return 1; // 已经存在路径(key)
    }

    // 放入
    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_shwnd->p_hlist, p_path, len, p_top_wnd);
    assert(NULL != p_iter);

    return 0;
}

klb_wnd_t* klbuiex_shwnd_get(klbuiex_shwnd_t* p_shwnd, const char* p_path)
{
    int len = strlen(p_path);

    return (klb_wnd_t*)klb_hlist_find(p_shwnd->p_hlist, p_path, len);
}

//////////////////////////////////////////////////////////////////////////
// register / get

klbuiex_shwnd_t* klbuiex_get_shwnd(klb_gui_t* p_gui)
{
    return (klbuiex_shwnd_t*)klb_gui_get_extension(p_gui, KLBUIEX_SHWND);
}

int klbuiex_register_shwnd(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_shwnd_create;
    ex.cb_destroy = klbuiex_shwnd_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_SHWND, &ex);

    return 0;
}
