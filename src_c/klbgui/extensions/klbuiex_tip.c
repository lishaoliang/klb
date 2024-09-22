// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_tip.h"
#include "klbgui/shwnd/klbshw_tip.h"
#include "klbgui/klb_wnd_in.h"
#include "klbmem/klb_mem.h"


#define KLBUIEX_TIP   "KLBUIEX-tip"


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

    p_ex->is_redraw = false;
    p_ex->is_show = false;

    p_ex->is_old_refresh = false;

    return p_ex;
}

static void klbuiex_tip_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_tip_t* p_ex = (klbuiex_tip_t*)ptr;

    KLB_FREE(p_ex)
}

//////////////////////////////////////////////////////////////////////////
// 私有

static bool has_canvas_klbuiex_tip(klbuiex_tip_t* p_ex)
{
    return (NULL != p_ex->p_canvas) ? true : false;
}

static klb_wnd_t* get_wnd_klbuiex_tip(klbuiex_tip_t* p_ex)
{
    if (NULL == p_ex->p_tip_wnd)
    {
        p_ex->p_tip_wnd = klbui_shwnd_get_tip(p_ex->p_gui);

        klb_wnd_move(p_ex->p_tip_wnd, 0, 0);
        klb_wnd_resize(p_ex->p_tip_wnd, 320, 32);
    }

    return p_ex->p_tip_wnd;
}

//////////////////////////////////////////////////////////////////////////
// 导出, export

void klbuiex_tip_attach_canvas(klbuiex_tip_t* p_ex, klb_canvas_t* p_canvas)
{
    if (NULL != p_canvas)
    {
        assert(NULL == p_ex->p_canvas);

        // 申请tip图层 使用的画布
        // 许可画布接口只支持主画布: 即 不支持tip等多层画布
        p_ex->p_canvas = klb_canvas_malloc(p_canvas, KLBUIEX_TIP_WIDTH_max, KLBUIEX_TIP_HEIGHT_max, p_canvas->color_fmt);

        p_ex->rect_dst.w = 320;
        p_ex->rect_dst.h = 32;

        //klbuiex_tip_show(p_ex, true, 100, 100, "123456");
    }
    else
    {
        // 释放画布
        KLB_FREE_BY(p_ex->p_canvas, klb_canvas_destroy);

        p_ex->p_tip_wnd = NULL;
    }
}

bool klbuiex_tip_need_repaint(klbuiex_tip_t* p_ex)
{
    return p_ex->is_redraw;
}

void klbuiex_tip_update(klbuiex_tip_t* p_ex, const char* p_tilte)
{
    if (p_ex->is_show)
    {
        // note. 更新 tip标题 之后, 可能导致 需求的区域变更
        // 需要重新计算位置

        int x = p_ex->rect_dst.x;
        int y = p_ex->rect_dst.y;

        klbuiex_tip_set_tilte(p_ex, p_tilte, NULL, NULL);
        klbuiex_tip_show(p_ex, true, x, y);

        p_ex->is_redraw = true;
    }
}

void klbuiex_tip_redraw(klbuiex_tip_t* p_ex)
{
    if (NULL != p_ex->p_tip_wnd && p_ex->is_show)
    {
        klb_wnd_draw(p_ex->p_tip_wnd);
    }

    p_ex->is_redraw = false;
}

klb_canvas_t* klbuiex_tip_get_refresh(klbuiex_tip_t* p_ex, bool* p_is_show, klb_rect_t* p_dst, klb_rect_t* p_src)
{
    if (NULL != p_is_show)
    {
        *p_is_show = p_ex->is_show;
    }

    if (NULL != p_dst)
    {
        *p_dst = p_ex->rect_dst;
    }

    if (NULL != p_src)
    {
        klb_wnd_t* p_wnd = get_wnd_klbuiex_tip(p_ex);

        *p_src = p_wnd->pos.rect_in_canvas;
    }

    return p_ex->p_canvas;
}

void klbuiex_tip_set_tilte(klbuiex_tip_t* p_ex, const char* p_title, int* p_out_w, int* p_out_h)
{
    klb_wnd_t* p_wnd = get_wnd_klbuiex_tip(p_ex);

    // 设置标题
    klbshw_tip_set_title(p_wnd, p_title);

    // Bug. 尽可能使用 tip 画布提供的 宽高
    // 重新布局
    klbshw_tip_layout(p_wnd, p_ex->p_canvas->rect.w, p_ex->p_canvas->rect.h, p_out_w, p_out_h);
}

void klbuiex_tip_show(klbuiex_tip_t* p_ex, bool show, int x, int y)
{
    if (show != p_ex->is_show)
    {
        p_ex->is_redraw = true;
    }

    p_ex->is_show = show;

    if (p_ex->is_show)
    {
        klb_wnd_t* p_wnd = get_wnd_klbuiex_tip(p_ex);

        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_ex->p_gui, &screen_w, &screen_h);

        klb_rect_t rect = p_wnd->pos.rect_in_parent;

        if (x < 0) { x = 0; };
        if (screen_w < x + rect.w) { x = screen_w - rect.w; };

        if (y < 0) { y = 0; };
        if (screen_h < y + rect.h) { y = screen_h - rect.h; };

        p_ex->rect_dst.x = x;
        p_ex->rect_dst.y = y;
        p_ex->rect_dst.w = rect.w;
        p_ex->rect_dst.h = rect.h;
    }
}

void klbuiex_tip_set_old(klbuiex_tip_t* p_ex, klb_rect_t* p_rect_old)
{
    p_ex->rect_old = *p_rect_old;
}

bool klbuiex_tip_get_old(klbuiex_tip_t* p_ex, klb_rect_t* p_rect_old)
{
    *p_rect_old = p_ex->rect_old;
    return p_ex->is_old_refresh;
}

void klbuiex_tip_set_old_refresh(klbuiex_tip_t* p_ex, bool refresh)
{
    p_ex->is_old_refresh = refresh;
}

//////////////////////////////////////////////////////////////////////////
// register / get


klbuiex_tip_t* klbuiex_get_tip(klb_gui_t* p_gui)
{
    return (klbuiex_tip_t*)klb_gui_get_extension(p_gui, KLBUIEX_TIP);
}

int klbuiex_register_tip(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_tip_create;
    ex.cb_destroy = klbuiex_tip_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_TIP, &ex);

    return 0;
}
