// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_tip.h"
#include "klbgui/shwnd/klbshw_tip.h"
#include "klbgui/klb_wnd_in.h"
#include "klbmem/klb_mem.h"


#define KLB_GUIEX_tip   "KLB-GUIEX-tip"


/// @struct klbuiex_tip_t
/// @brief  tip扩展
typedef struct klbuiex_tip_t_
{
    klb_gui_t*      p_gui;              ///< gui

    klb_canvas_t*   p_canvas;           ///< tip 图层画布
    klb_wnd_t*      p_tip_wnd;          ///< tip 窗口; klbui_shwnd_get_tip()

    bool            is_show;            ///< 是否显示TIP
    bool            is_redraw;          ///< 是否需要重绘

    // 历史绘制信息
    struct
    {
        klb_rect_t  dirty_rect;         ///< 标记脏矩形区域
        bool        is_dirty;           ///< 是否有脏矩形区域
    };
}klbuiex_tip_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义


//////////////////////////////////////////////////////////////////////////
// create / destroy / loop once

static void* klbuiex_tip_create(klb_gui_t* p_gui)
{
    klbuiex_tip_t* p_ex = KLB_MALLOCZ(klbuiex_tip_t, 1, 0);

    p_ex->p_gui = p_gui;

    p_ex->p_canvas = NULL;
    p_ex->p_tip_wnd = NULL;

    p_ex->is_show = false;
    p_ex->is_redraw = false;

    p_ex->is_dirty = false;

    return p_ex;
}

static void klbuiex_tip_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_tip_t* p_ex = (klbuiex_tip_t*)ptr;

    KLB_FREE(p_ex)
}

//////////////////////////////////////////////////////////////////////////
// 私有

static klb_wnd_t* get_wnd_klbuiex_tip(klbuiex_tip_t* p_ex)
{
    if (NULL == p_ex->p_tip_wnd)
    {
        p_ex->p_tip_wnd = klbui_shwnd_get_tip(p_ex->p_gui);
    }

    return p_ex->p_tip_wnd;
}

//////////////////////////////////////////////////////////////////////////
// 导出, export

void klbuiex_tip_try_attach_canvas(klbuiex_tip_t* p_ex, const klb_canvas_t* p_main_canvas)
{
    if (NULL != p_main_canvas)
    {
        assert(NULL == p_ex->p_canvas);

        // 申请tip图层 使用的画布
        // 注意: 图形适配层 不一定会有TIP图层 p_ex->p_canvas = NULL 是可能的
        p_ex->p_canvas = klb_canvas_malloc((klb_canvas_t*)p_main_canvas, 0, 0, KLB_CANVAS_LAYER_tip);
    }
    else
    {
        // 释放画布
        KLB_FREE_BY(p_ex->p_canvas, klb_canvas_destroy);

        p_ex->p_tip_wnd = NULL;
    }

    p_ex->is_show = false;
    p_ex->is_redraw = false;

    p_ex->is_dirty = false;
}

bool klbuiex_tip_has_canvas(klbuiex_tip_t* p_ex)
{
    return (NULL != p_ex->p_canvas) ? true : false;
}

klb_canvas_t* klbuiex_tip_get_canvas(klbuiex_tip_t* p_ex)
{
    return p_ex->p_canvas;
}

bool klbuiex_tip_is_show(klbuiex_tip_t* p_ex)
{
    // 1. 需要有画布
    if (NULL == p_ex->p_canvas)
    {
        return false;
    }

    klb_wnd_t* p_tip_wnd = get_wnd_klbuiex_tip(p_ex);
    sds title = klbshw_tip_get_title(p_tip_wnd);

    // 2. 需要 设置了 title 且 不为空
    if (NULL == title || sdslen(title) <= 0)
    {
        return false;
    }

    // 3. is_show 标记
    return p_ex->is_show;
}

void klbuiex_tip_show(klbuiex_tip_t* p_ex, bool show)
{
    p_ex->is_show = show;
}

void klbuiex_tip_set_title(klbuiex_tip_t* p_ex, const char* p_title)
{
    klb_wnd_t* p_tip_wnd = get_wnd_klbuiex_tip(p_ex);
    klbshw_tip_set_title(p_tip_wnd, p_title);

    int screen_w = 0;
    klb_gui_get_wh(p_ex->p_gui, &screen_w, NULL);

    int w = 0, h = 0;
    klbshw_tip_layout(p_tip_wnd, screen_w, 128, &w, &h);

    // 重设画布大小
    klb_canvas_resize(p_ex->p_canvas, w, h);

    // 变更显示内容了, 需要重绘
    p_ex->is_redraw = true;
}

void klbuiex_tip_move(klbuiex_tip_t* p_ex, int x, int y)
{
    klb_wnd_t* p_tip_wnd = get_wnd_klbuiex_tip(p_ex);

    // 限制 移动范围, 防止超过屏幕范围
    int sx = x, sy = y;

    {
        int w = p_tip_wnd->pos.rect_in_parent.w, h = p_tip_wnd->pos.rect_in_parent.h;

        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_ex->p_gui, &screen_w, &screen_h);

        if (screen_w < sx + w) { sx = screen_w - w; };
        if (screen_h < sy + h) { sy = screen_h - h - 1; };

        if (sx <= 0) { sx = 0; };
        if (sy <= 0) { sy = 0; };
    }

    // 移动窗口
    klb_wnd_move(p_tip_wnd, sx, sy);

    // 移动画布
    klb_canvas_move(p_ex->p_canvas, sx, sy);
}

void klbuiex_tip_set_redraw(klbuiex_tip_t* p_ex)
{
    p_ex->is_redraw = true;
}

bool klbuiex_tip_redraw(klbuiex_tip_t* p_ex)
{
    bool redraw = false;

    // 若需要重绘, 则重绘
    if (p_ex->is_redraw)
    {
        if (NULL != p_ex->p_canvas)
        {
            klb_wnd_t* p_tip_wnd = get_wnd_klbuiex_tip(p_ex);
            klb_wnd_draw(p_tip_wnd);

            redraw = true;
        }

        p_ex->is_redraw = false; // 下次 , 不需要重绘了

        redraw = true;
    }

    return redraw;
}

void klbuiex_tip_set_dirty(klbuiex_tip_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect)
{
    if (dirty) { p_ex->dirty_rect = *p_diry_rect; }

    p_ex->is_dirty = dirty;
}

bool klbuiex_tip_get_dirty(klbuiex_tip_t* p_ex, klb_rect_t* p_out_diry_rect)
{
    if (p_ex->is_dirty && NULL != p_out_diry_rect)
    {
        *p_out_diry_rect = p_ex->dirty_rect;
    }

    return p_ex->is_dirty;
}

//////////////////////////////////////////////////////////////////////////
// register / get


klbuiex_tip_t* klbuiex_get_tip(klb_gui_t* p_gui)
{
    return (klbuiex_tip_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_tip);
}

int klbuiex_register_tip(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_tip_create;
    ex.cb_destroy = klbuiex_tip_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLB_GUIEX_tip, &ex);

    return 0;
}

// end
