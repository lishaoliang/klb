// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_waitlayer.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klb_gui_in.h"
#include "klbgui/klb_wnd_in.h"


#define KLBUIEX_WAITLAYER   "KLBUIEX-waitlayer"


/// @struct klbuiex_waitlayer_t
/// @brief  wait layer canvas 等待图层
///   1. 对于 此图层 无法精确控制 每个控件的局部刷新
///   2. 只要有刷新, 即 认为需要刷新 整个图层
///   3. 因 只可以 整体更新, 所以为提升效率, 不要频繁更新此图层; 或尽量小区域更新
typedef struct klbuiex_waitlayer_t_
{
    klb_gui_t*          p_gui;              ///< GUI模块

    klb_canvas_t*       p_canvas;           ///< wait layer 等待 画布
    klb_wnd_t*          p_wait_wnd;         ///< wait 等待 窗口

    bool                is_show;            ///< 是否显示TIP
    bool                is_redraw;          ///< 是否需要重绘

    // 历史绘制信息
    // 当从 显示 => 隐藏 时, 需要更新重绘 脏矩形区域
    struct
    {
        klb_rect_t      dirty_rect;         ///< 标记脏矩形区域
        bool            is_dirty;           ///< 是否有脏矩形区域
    };
}klbuiex_waitlayer_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义


//////////////////////////////////////////////////////////////////////////
// 适配 

static void* klbuiex_waitlayer_create(klb_gui_t* p_gui)
{
    klbuiex_waitlayer_t* p_ex = KLB_MALLOCZ(klbuiex_waitlayer_t, 1, 0);

    p_ex->p_gui = p_gui;

    p_ex->is_show = false;
    p_ex->is_redraw = false;
    p_ex->is_dirty = false;

    return p_ex;
}

static void klbuiex_waitlayer_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_waitlayer_t* p_ex = (klbuiex_waitlayer_t*)ptr;

    KLB_FREE(p_ex)
}

//////////////////////////////////////////////////////////////////////////
// 内部函数


//////////////////////////////////////////////////////////////////////////
// 对外接口

void klbuiex_waitlayer_try_attach_canvas(klbuiex_waitlayer_t* p_ex, const klb_canvas_t* p_main_canvas)
{
    if (NULL != p_main_canvas)
    {
        assert(NULL == p_ex->p_canvas);

        // 申请 wait layer 图层 使用的画布
        // 注意: 图形适配层 不一定会有此图层 p_ex->p_canvas = NULL 是可能的
        p_ex->p_canvas = klb_canvas_malloc((klb_canvas_t*)p_main_canvas, 0, 0, KLB_CANVAS_LAYER_wait);
    }
    else
    {
        // 释放画布
        KLB_FREE_BY(p_ex->p_canvas, klb_canvas_destroy);
    }

    p_ex->p_wait_wnd = NULL;

    p_ex->is_show = false;
    p_ex->is_redraw = false;
    p_ex->is_dirty = false;
}

/// @brief 是否有 等待 画布
bool klbuiex_waitlayer_has_canvas(klbuiex_waitlayer_t* p_ex)
{
    return (NULL != p_ex->p_canvas) ? true : false;
}

/// @brief 获取 等待 画布
klb_canvas_t* klbuiex_waitlayer_get_canvas(klbuiex_waitlayer_t* p_ex)
{
    return p_ex->p_canvas;
}

void klbuiex_waitlayer_bind_wnd(klbuiex_waitlayer_t* p_ex, klb_wnd_t* p_top)
{
    if (NULL != p_top)
    {
        assert(NULL == p_top->p_parent);

        // 顶层窗口
        klb_wnd_set_top(p_top);

        // udata 图层
        klb_gui_set_wnd_layer_type(p_top, KLB_CANVAS_LAYER_wait);
    }

    p_ex->p_wait_wnd = p_top;
}

klb_wnd_t* klbuiex_waitlayer_get_wnd(klbuiex_waitlayer_t* p_ex)
{
    return p_ex->p_wait_wnd;
}

/// @brief 获取是否 显示
bool klbuiex_waitlayer_is_show(klbuiex_waitlayer_t* p_ex)
{
    // 1. 需要有画布
    if (NULL == p_ex->p_canvas)
    {
        return false;
    }

    // 2. 需要绑定了窗口
    if (NULL == p_ex->p_wait_wnd)
    {
        return false;
    }

    // 3. is_show 标记
    return p_ex->is_show;
}

/// @brief 设置 是否显示
void klbuiex_waitlayer_show(klbuiex_waitlayer_t* p_ex, bool show)
{
    p_ex->is_show = show;
}

/// @brief 移动位置
void klbuiex_waitlayer_move(klbuiex_waitlayer_t* p_ex, int x, int y)
{
    klb_wnd_t* p_wnd = klbuiex_waitlayer_get_wnd(p_ex);

    if (NULL == p_wnd || NULL == p_ex->p_canvas)
    {
        return; // 未绑定窗口 或 无画布
    }

    // 限制 移动范围, 防止超过屏幕范围
    int sx = x, sy = y;

    {
        int w = p_wnd->pos.rect_in_parent.w, h = p_wnd->pos.rect_in_parent.h;

        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_ex->p_gui, &screen_w, &screen_h);

        if (screen_w < sx + w) { sx = screen_w - w; };
        if (screen_h < sy + h) { sy = screen_h - h - 1; };

        if (sx <= 0) { sx = 0; };
        if (sy <= 0) { sy = 0; };
    }

    // 移动窗口
    klb_wnd_move(p_wnd, sx, sy);

    // 移动画布
    klb_canvas_move(p_ex->p_canvas, sx, sy);
}

/// @brief 重新绘制
bool klbuiex_waitlayer_redraw(klbuiex_waitlayer_t* p_ex)
{
    bool redraw = false;

    // 若需要重绘, 则重绘
    if (p_ex->is_redraw)
    {
        klb_wnd_t* p_wnd = klbuiex_waitlayer_get_wnd(p_ex);

        if (NULL != p_ex->p_canvas && NULL != p_wnd)
        {
            klb_wnd_draw(p_wnd);

            redraw = true;
        }

        p_ex->is_redraw = false; // 下次 , 不需要重绘了
    }

    return redraw; // 重绘过
}

/// @brief 设置/获取 脏矩形区域
void klbuiex_waitlayer_set_dirty(klbuiex_waitlayer_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect)
{
    if (dirty)
    {
        p_ex->dirty_rect = *p_diry_rect;
    }

    p_ex->is_dirty = dirty;
}

bool klbuiex_waitlayer_get_dirty(klbuiex_waitlayer_t* p_ex, klb_rect_t* p_out_diry_rect)
{
    if (NULL != p_out_diry_rect && p_ex->is_dirty)
    {
        *p_out_diry_rect = p_ex->dirty_rect;
    }

    return p_ex->is_dirty;
}

//////////////////////////////////////////////////////////////////////////
// 注册/获取

klbuiex_waitlayer_t* klbuiex_get_waitlayer(klb_gui_t* p_gui)
{
    return (klbuiex_waitlayer_t*)klb_gui_get_extension(p_gui, KLBUIEX_WAITLAYER);
}

int klbuiex_register_waitlayer(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_waitlayer_create;
    ex.cb_destroy = klbuiex_waitlayer_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_WAITLAYER, &ex);

    return 0;
}

// end
