// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_ticker.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_ticker_deinit_attribute(klbwnd_ticker_t* p_ticker);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_ticker_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_ticker_on_paint(klb_wnd_t* p_wnd)
{
    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 定时器控件, 无绘图
    return 0;
}

static int klbwnd_ticker_on_load(klb_wnd_t* p_wnd, klbwnd_ticker_t* p_ticker)
{
    // 页面加载之后, 使用默认值更新
    p_ticker->enable = p_ticker->enable_default;

    // 更新计时
    p_ticker->tick_count = klb_wnd_get_tick_count(p_wnd);

    return 0;
}

static int klbwnd_ticker_on_ticker(klb_wnd_t* p_wnd, klbwnd_ticker_t* p_ticker)
{
    // KLBUI_onticker 事件在窗口卸载后, 不会产生
    int64_t tc = klb_wnd_get_tick_count(p_wnd);

    if (!p_ticker->enable)
    {
        // 不可用, 直接更新时间
        p_ticker->tick_count = tc;
        return 0;
    }

    if (ABS_SUB(tc, p_ticker->tick_count) < p_ticker->tc_interval)
    {
        return 0;
    }

    // 定时器到了, 更新
    p_ticker->tick_count = tc;

    // 触发 KLBUI_onticker 事件
    // 对于 KLBUI_onticker 事件: 存在以下场景
    // 1. 标准的定时器控件, 需要隐藏状态 (无需绘图)
    // 2. 动画等控件, 在不使能状态时, 依旧可能需要触发事件
    // 所以 onticker 需要与控件状态无关, 由控件自行决定适当时机触发
    klb_wnd_call_command(p_wnd, KLBUI_onticker, NULL, NULL, 0, 0);

    return 0;
}

static int klbwnd_ticker_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    if (KLBUI_onpaint == msg)
    {
        return klbwnd_ticker_on_paint(p_wnd);
    }
    else if (KLBUI_onload == msg)
    {
        return klbwnd_ticker_on_load(p_wnd, p_ticker);
    }
    else if (KLBUI_onticker == msg)
    {
        return klbwnd_ticker_on_ticker(p_wnd, p_ticker);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_ticker_set_css(klb_wnd_t* p_wnd, klbwnd_ticker_css_t* p_css)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_ticker->p_css = p_css;
}

void klbwnd_ticker_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_ticker->index = index;
}

int klbwnd_ticker_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    return p_ticker->index;
}

void klbwnd_ticker_set_enable(klb_wnd_t* p_wnd, bool is_enable)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_ticker->enable = is_enable;
}

bool klbwnd_ticker_get_enable(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    return p_ticker->enable;
}

void klbwnd_ticker_set_enable_default(klb_wnd_t* p_wnd, bool is_enable)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_ticker->enable_default = is_enable;
}

bool klbwnd_ticker_get_enable_default(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    return p_ticker->enable_default;
}

void klbwnd_ticker_set_interval(klb_wnd_t* p_wnd, int64_t interval)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_ticker->tc_interval = interval;
}

int64_t klbwnd_ticker_get_interval(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    return p_ticker->tc_interval;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_ticker_init_attribute(klb_wnd_t* p_wnd, klbwnd_ticker_t* p_ticker)
{
    p_ticker->index = 0;

    p_ticker->enable = true;
    p_ticker->enable_default = true;
    p_ticker->tick_count = klb_wnd_get_tick_count(p_wnd);
    p_ticker->tc_interval = 1000;
}

static void klbwnd_ticker_deinit_attribute(klbwnd_ticker_t* p_ticker)
{
    (void)p_ticker;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_ticker_css_init(klbwnd_ticker_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;
}

void klbwnd_ticker_css_deinit(klbwnd_ticker_css_t* p_css)
{
    (void)p_css;
}

void klbwnd_ticker_css_copy(klbwnd_ticker_css_t* p_dst, klbwnd_ticker_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_ticker_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_ticker_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_TICKER;

    // 默认隐藏, 无需绘图
    p_wnd->state.status = KLB_WND_STATUS_HIDE;

    klbwnd_ticker_init_attribute(p_wnd, p_ticker);
}

void klbwnd_ticker_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_ticker_t* p_ticker = (klbwnd_ticker_t*)p_wnd->ctrl;

    klbwnd_ticker_deinit_attribute(p_ticker);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_ticker_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_ticker_t));

    // step2. 初始化基础部分
    klbwnd_ticker_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbwnd_ticker_destroy;

    return p_wnd;
}

// end
