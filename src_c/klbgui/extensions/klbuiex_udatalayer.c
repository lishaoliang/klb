// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_udatalayer.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klb_gui_in.h"
#include "klbgui/klb_wnd_in.h"
#include "klbutil/klb_nlist.h"


#define KLB_GUIEX_udatalayer   "KLB-GUIEX-udatalayer"


/// @struct klbuiex_udatalayer_t
/// @brief  user data layer canvas 用户自定义图层
///   1. 对于 此图层 无法精确控制 每个控件的局部刷新
///   2. 只要有刷新, 即 认为需要刷新 整个图层
///   3. 因 只可以 整体更新, 所以为提升效率, 不要频繁更新此图层; 或尽量小区域更新
typedef struct klbuiex_udatalayer_t_
{
    klb_gui_t*          p_gui;              ///< GUI模块

    klb_canvas_t*       p_canvas;           ///< user data layer 用户自定义 画布
    klb_wnd_t*          p_udata_wnd;        ///< udata 用户自定义 窗口

    bool                is_show;            ///< 是否显示TIP
    bool                is_redraw;          ///< 是否需要重绘

    // 历史绘制信息
    // 当从 显示 => 隐藏 时, 需要更新重绘 脏矩形区域
    struct
    {
        klb_rect_t      dirty_rect;         ///< 标记脏矩形区域
        bool            is_dirty;           ///< 是否有脏矩形区域
    };

    // 窗口 定时器
    struct
    {
        int64_t         tick_count;         ///< 计时
        int64_t         interval_tc;        ///< 间隔 单位毫秒

        klb_nlist_t*    p_ticker_list;      ///< 定时器 窗口列表
    };
}klbuiex_udatalayer_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void clear_ticker_list_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex);
static int do_ticker_list_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex, klb_nlist_t* p_list);


//////////////////////////////////////////////////////////////////////////
// 适配 

static void* klbuiex_udatalayer_create(klb_gui_t* p_gui)
{
    klbuiex_udatalayer_t* p_ex = KLB_MALLOCZ(klbuiex_udatalayer_t, 1, 0);

    p_ex->p_gui = p_gui;

    p_ex->is_show = false;
    p_ex->is_redraw = false;
    p_ex->is_dirty = false;

    // 定时器
    p_ex->p_ticker_list = klb_nlist_create();

    p_ex->tick_count = klb_gui_get_tick_count(p_gui);
    p_ex->interval_tc = 500;

    return p_ex;
}

static void klbuiex_udatalayer_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_udatalayer_t* p_ex = (klbuiex_udatalayer_t*)ptr;

    KLB_FREE_BY(p_ex->p_ticker_list, klb_nlist_destroy);
    KLB_FREE(p_ex)
}

/// @brief 控制操作消息
/// @note 当UI框架需要扩展处理事务时调用
static int klbuiex_udatalayer_control(void* ptr, klb_gui_t* p_gui, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbuiex_udatalayer_t* p_ex = (klbuiex_udatalayer_t*)ptr;

    if (KLBUI_EX_MSG_quit == msg)
    {
        klbuiex_udatalayer_bind_wnd(p_ex, NULL); // 解绑窗口
    }
    else if (KLBUI_EX_MSG_clear == msg)
    {
        klbuiex_udatalayer_bind_wnd(p_ex, NULL); // 解绑窗口
    }

    return 0;
}

static int klbuiex_udatalayer_loop_once(void* ptr, klb_gui_t* p_gui, int64_t now)
{
    klbuiex_udatalayer_t* p_ex = (klbuiex_udatalayer_t*)ptr;

    if (!klbuiex_udatalayer_is_show(p_ex))
    {
        return 0; // 未显示  无需处理
    }

    // 定时 计数
    if (ABS_SUB(now, p_ex->tick_count) <= p_ex->interval_tc)
    {
        return 0; // 时间未到
    }

    p_ex->tick_count = now;

    // 处理窗口定时器
    do_ticker_list_klbuiex_udatalayer(p_ex, p_ex->p_ticker_list);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 内部函数

// 清空 定时器窗口 列表
static void clear_ticker_list_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex)
{
    klb_nlist_clear(p_ex->p_ticker_list, NULL, NULL);
}

// 推送 onticker 事件
static int do_ticker_list_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex, klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    if (klb_nlist_size(p_list) <= 0)
    {
        return 0;
    }

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_list);

    while (NULL != p_iter)
    {
        klb_wnd_t* p_wnd = klb_nlist_data(p_iter);
        assert(NULL != p_wnd);

        // 窗口 onticker 事件
        // 这里 只触发 控件 事件, 由控件决定后续处理
        klb_wnd_call_control(p_wnd, KLBUI_onticker, NULL, NULL, 0, 0);

        p_iter = klb_nlist_next(p_iter);
    }

    return 0;
}

// 将需要处理定时器的 窗口加入链表
static int do_push_ticker_list_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex, klb_nlist_t* p_list, klb_wnd_t* p_wnd)
{
    // 递归 遍历窗口
    if (NULL == p_wnd)
    {
        return 0;
    }

    // 带有 KLB_WND_STYLE_TICKER_TOP 样式, 则加入链表
    if (KLB_WND_STYLE_TICKER & p_wnd->state.style)
    {
        klb_nlist_push_tail(p_list, p_wnd);
    }

    // 遍历子窗口
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        do_push_ticker_list_klbuiex_udatalayer(p_ex, p_list, p_next);

        p_next = p_next->p_next;
    }

    return 0;
}

// 需要递归执行的窗口事件: eg. KLBUI_onload
static void do_control_event_recursive_klbuiex_udatalayer(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (NULL == p_wnd)
    {
        return;
    }

    klb_wnd_t* p_next = p_wnd->p_child;

    while (NULL != p_next)
    {
        do_control_event_recursive_klbuiex_udatalayer(p_next, e, p_pt1, p_pt2, lparam, wparam);

        p_next = p_next->p_next;
    }

    klb_wnd_call_control(p_wnd, e, p_pt1, p_pt2, lparam, wparam);
}

// 窗口 绑定时 的处理
static void do_wnd_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex)
{
    klb_wnd_t* p_wnd = p_ex->p_udata_wnd;

    // "onload" 事件
    {
        // on_control 递归调用所有的控件
        do_control_event_recursive_klbuiex_udatalayer(p_wnd, KLBUI_onload, NULL, NULL, 0, 0);

        // 只有最顶层窗口拥有 "onload"事件响应
        klb_wnd_call_command(p_wnd, KLBUI_onload, NULL, NULL, 0, 0);
    }

    // "onpredraw" 事件
    {
        // 在 计算 画布 绝对坐标之前, 是因为: 部分控件 需要 在KLBUI_onpredraw事件中 重新调整 子控件的位置
        // on_control 递归调用所有的控件
        do_control_event_recursive_klbuiex_udatalayer(p_wnd, KLBUI_onpredraw, NULL, NULL, 0, 0);
    }

    // 标记需要更新画布坐标
    klb_wnd_update_canvas_rect(p_wnd);

    // 将所有 需要处理定时器的窗口 放入定时器列表
    do_push_ticker_list_klbuiex_udatalayer(p_ex, p_ex->p_ticker_list, p_wnd);

    // 需要重绘
    p_ex->is_redraw = true;
}

// 窗口 解绑 时的 处理
static void do_wnd_end_klbuiex_udatalayer(klbuiex_udatalayer_t* p_ex)
{
    klb_wnd_t* p_wnd = p_ex->p_udata_wnd;

    // "onunload" 事件
    {
        // 只有最顶层窗口拥有 "onunload"事件响应
        klb_wnd_call_command(p_wnd, KLBUI_onunload, NULL, NULL, 0, 0);

        // on_control 递归调用所有的控件
        do_control_event_recursive_klbuiex_udatalayer(p_wnd, KLBUI_onunload, NULL, NULL, 0, 0);
    }

    // 清理定时器列表
    clear_ticker_list_klbuiex_udatalayer(p_ex);

    // 需要重绘
    p_ex->is_redraw = true;
}

//////////////////////////////////////////////////////////////////////////
// 对外接口

void klbuiex_udatalayer_try_attach_canvas(klbuiex_udatalayer_t* p_ex, const klb_canvas_t* p_main_canvas)
{
    if (NULL != p_main_canvas)
    {
        assert(NULL == p_ex->p_canvas);

        // 申请 udata layer 图层 使用的画布
        // 注意: 图形适配层 不一定会有此图层 p_ex->p_canvas = NULL 是可能的
        p_ex->p_canvas = klb_canvas_malloc((klb_canvas_t*)p_main_canvas, 0, 0, KLB_CANVAS_LAYER_udata);
    }
    else
    {
        // 释放画布
        KLB_FREE_BY(p_ex->p_canvas, klb_canvas_destroy);
    }

    p_ex->is_show = false;
    p_ex->is_redraw = false;
    p_ex->is_dirty = false;
}

/// @brief 是否有 用户自定义 画布
bool klbuiex_udatalayer_has_canvas(klbuiex_udatalayer_t* p_ex)
{
    return (NULL != p_ex->p_canvas) ? true : false;
}

/// @brief 获取 用户自定义 画布
klb_canvas_t* klbuiex_udatalayer_get_canvas(klbuiex_udatalayer_t* p_ex)
{
    return p_ex->p_canvas;
}

void klbuiex_udatalayer_set_interval(klbuiex_udatalayer_t* p_ex, int64_t interval)
{
    p_ex->interval_tc = interval;
}

int64_t klbuiex_udatalayer_get_interval(klbuiex_udatalayer_t* p_ex)
{
    return p_ex->interval_tc;
}

void klbuiex_udatalayer_bind_wnd(klbuiex_udatalayer_t* p_ex, klb_wnd_t* p_top)
{
    // 处理原窗口 的结束
    if (NULL != p_ex->p_udata_wnd)
    {
        // 窗口结束 处理 
        do_wnd_end_klbuiex_udatalayer(p_ex);
        p_ex->p_udata_wnd = NULL;
    }

    if (NULL != p_top)
    {
        assert(NULL == p_top->p_parent);

        // 顶层窗口
        klb_wnd_set_top(p_top);

        // udata 图层
        klb_gui_set_wnd_layer_type(p_top, KLB_CANVAS_LAYER_udata);

        // 更新窗口
        p_ex->p_udata_wnd = p_top;

        // 窗口预先 处理
        do_wnd_klbuiex_udatalayer(p_ex);
    }
}

klb_wnd_t* klbuiex_udatalayer_get_wnd(klbuiex_udatalayer_t* p_ex)
{
    return p_ex->p_udata_wnd;
}

/// @brief 获取是否 显示
bool klbuiex_udatalayer_is_show(klbuiex_udatalayer_t* p_ex)
{
    // 1. 需要有画布
    if (NULL == p_ex->p_canvas)
    {
        return false;
    }

    // 2. 需要绑定了窗口
    if (NULL == p_ex->p_udata_wnd)
    {
        return false;
    }

    // 3. is_show 标记
    return p_ex->is_show;
}

/// @brief 设置 是否显示
void klbuiex_udatalayer_show(klbuiex_udatalayer_t* p_ex, bool show)
{
    p_ex->is_show = show;

    // 更新时间
    p_ex->tick_count = klb_gui_get_tick_count(p_ex->p_gui);
}

/// @brief 移动tip位置
void klbuiex_udatalayer_move(klbuiex_udatalayer_t* p_ex, int x, int y)
{
    klb_wnd_t* p_wnd = klbuiex_udatalayer_get_wnd(p_ex);

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

    // 重新设置 画布尺寸
    klb_canvas_resize(p_ex->p_canvas, p_wnd->pos.rect_in_parent.w, p_wnd->pos.rect_in_parent.h);

    // 标记需要更新画布坐标
    klb_wnd_update_canvas_rect(p_wnd);

    // 需要重绘
    p_ex->is_redraw = true;
}

void klbuiex_udatalayer_set_redraw(klbuiex_udatalayer_t* p_ex)
{
    p_ex->is_redraw = true;
}

bool klbuiex_udatalayer_wnd_in_bind(klbuiex_udatalayer_t* p_ex, klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    if (NULL != p_ex->p_udata_wnd && p_top == p_ex->p_udata_wnd)
    {
        return true;
    }

    return false;
}

/// @brief 重新绘制
bool klbuiex_udatalayer_redraw(klbuiex_udatalayer_t* p_ex)
{
    bool redraw = false;

    // 若需要重绘, 则重绘
    if (p_ex->is_redraw)
    {
        klb_wnd_t* p_wnd = klbuiex_udatalayer_get_wnd(p_ex);

        if (NULL != p_ex->p_canvas && NULL != p_wnd)
        {
            klb_wnd_draw(p_wnd);

            redraw = true;
        }

        p_ex->is_redraw = false; // 下次 , 不需要重绘了
    }

    return redraw; // 重绘过
}

void klbuiex_udatalayer_set_dirty(klbuiex_udatalayer_t* p_ex, bool dirty, const klb_rect_t* p_diry_rect)
{
    if (dirty)
    {
        p_ex->dirty_rect = *p_diry_rect;
    }

    p_ex->is_dirty = dirty;
}

bool klbuiex_udatalayer_get_dirty(klbuiex_udatalayer_t* p_ex, klb_rect_t* p_out_diry_rect)
{
    if (NULL != p_out_diry_rect && p_ex->is_dirty)
    {
        *p_out_diry_rect = p_ex->dirty_rect;
    }

    return p_ex->is_dirty;
}

//////////////////////////////////////////////////////////////////////////
// 注册/获取

klbuiex_udatalayer_t* klbuiex_get_udatalayer(klb_gui_t* p_gui)
{
    return (klbuiex_udatalayer_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_udatalayer);
}

int klbuiex_register_udatalayer(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_udatalayer_create;
    ex.cb_destroy = klbuiex_udatalayer_destroy;
    ex.cb_control = klbuiex_udatalayer_control;
    ex.cb_loop_once = klbuiex_udatalayer_loop_once;

    klb_gui_register_extension(p_gui, KLB_GUIEX_udatalayer, &ex);

    return 0;
}

// end
