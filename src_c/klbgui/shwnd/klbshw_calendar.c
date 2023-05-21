// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/shwnd/klbshw_calendar.h"
#include "klbgui/wnd/klbwnd_calendar.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "klbgui/klb_gui.h"


/// @struct klbshw_calendar_t
/// @brief  日历菜单
typedef struct klbshw_calendar_t_
{
    klbwnd_calendar_css_t   calendar_css;   ///< 日历控件css
    klb_wnd_t*              p_calendar;     ///< 日历控件: klbwnd_calendar_create(...)

    struct
    {
        klbshw_calendar_cb  cb;
        void*               ptr;
    };
}klbshw_calendar_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义

static klb_wnd_t* klbshw_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h);
static void klbshw_calendar_quit_attribute(klbshw_calendar_t* p_cal);
static void klbshw_calendar_popup_end(klb_wnd_t* p_wnd_menu, klbshw_calendar_t* p_cal, bool ok);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_calendar_destroy(klb_wnd_t* p_wnd)
{
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;

    klbshw_calendar_quit_attribute(p_cal);

    KLB_FREE(p_wnd);
}

static void klbshw_calendar_on_paint_status(klb_wnd_t* p_wnd, klbshw_calendar_t* p_cal, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 31, 41, 21));

    // 边框
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 61, 61, 61));
}

static int klbshw_calendar_on_paint(klb_wnd_t* p_wnd)
{
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    //klb_rect_t paint_rect = *p_rect;

    //klbshw_calendar_on_paint_status(p_wnd, p_calendar, NULL, &paint_rect);

    return 0;
}

static int klbshw_calendar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbshw_calendar_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;
            if (!klb_pt_in_rect(p_rect, p_pt1->x, p_pt1->y))
            {
                klbshw_calendar_popup_end(p_wnd, p_cal, false);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法




//////////////////////////////////////////////////////////////////////////
// 内部私有函数

// 结束
static void klbshw_calendar_popup_end(klb_wnd_t* p_wnd, klbshw_calendar_t* p_cal, bool ok)
{
    if (p_cal->cb)
    {
        int year = 0, month = 0, day = 0;
        klbwnd_calendar_get_date(p_cal->p_calendar, &year, &month, &day);

        p_cal->cb(p_cal->ptr, p_wnd, ok, year, month, day);
    }

    // 结束弹出
    klb_gui_popup_end(p_wnd->p_gui, true);
}


// 日历子控件响应
static int on_command_subcalendar_klbshw_calendar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_cal = p_wnd->p_udata;
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd_cal->ctrl;

    switch (e)
    {
    case KLBUI_onchange:
    {
        klbshw_calendar_popup_end(p_wnd_cal, p_cal, true);
    }
    break;
    default:
        break;
    }

    return 0;
}

static int klbshw_calendar_init_subwnd(klb_wnd_t* p_wnd, klbshw_calendar_t* p_cal, klb_gui_t* p_gui)
{
    klb_rect_t rect = p_wnd->pos.rect_in_parent;

    p_cal->p_calendar = klbwnd_calendar_create(p_gui, 0, 0, rect.w, rect.h);
    klb_wnd_push_child(p_wnd, p_cal->p_calendar);

    klbwnd_calendar_set_css(p_cal->p_calendar, &p_cal->calendar_css); // css

    klb_wnd_bind_command(p_cal->p_calendar, on_command_subcalendar_klbshw_calendar, p_wnd);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出

/// @brief 获取日历顶层对话框(菜单)
klb_wnd_t* klbui_shwnd_get_calendar(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_calendar);

    if (NULL == p_wnd)
    {
        int w = 0, h = 0;
        klbshw_calendar_wh(p_gui, &w, &h);

        p_wnd = klbshw_calendar_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_calendar, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

/// @brief 获取所需要的宽高
int klbshw_calendar_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{
    if (p_out_w) { *p_out_w = 320; }
    if (p_out_h) { *p_out_h = 360; }

    return 0;
}

int klbshw_calendar_bind(klb_wnd_t* p_wnd, klbshw_calendar_cb cb, void* ptr)
{
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;

    p_cal->cb = cb;
    p_cal->ptr = ptr;

    return 0;
}

void klbshw_calendar_set_date(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;

    klbwnd_calendar_set_date(p_cal->p_calendar, year, month, day);
    klbwnd_calendar_set_date_page(p_cal->p_calendar, year, month, day);
}


//////////////////////////////////////////////////////////////////////////
// init attribute 初始化属性默认值

static void klbshw_calendar_init_attribute(klb_wnd_t* p_wnd, klbshw_calendar_t* p_cal)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);
    klb_gui_t* p_gui = p_wnd->p_gui;

    klbwnd_calendar_css_init(&p_cal->calendar_css, p_gui);
}

static void klbshw_calendar_quit_attribute(klbshw_calendar_t* p_cal)
{
    klbwnd_calendar_css_quit(&p_cal->calendar_css);
}

//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_calendar_t));
    klbshw_calendar_t* p_cal = (klbshw_calendar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbshw_calendar_destroy;
    p_wnd->vtable.on_control = klbshw_calendar_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    // css
    klbshw_calendar_init_attribute(p_wnd, p_cal);

    // 子窗口
    klbshw_calendar_init_subwnd(p_wnd, p_cal, p_gui);

    return p_wnd;
}
