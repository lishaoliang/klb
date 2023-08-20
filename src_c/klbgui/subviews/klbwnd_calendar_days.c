// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_calendar_days.h"
#include "klbgui/subviews/klbwnd_calendar_btn.h"
#include "klbgui/wnd/klbwnd_static.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_calendar_days_quit_attribute(klbwnd_calendar_days_t* p_days);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_calendar_days_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    klbwnd_calendar_days_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_calendar_days_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_calendar_css_t* p_css = p_days->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    // 无需绘图

    return 0;
}


static int klbwnd_calendar_days_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_calendar_days_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_calendar_days_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    p_days->p_css = p_css;

    // 第一排 静态框
    for (int i = 0; i < KLBWND_CAL_DAYS_column; i++)
    {
        klb_wnd_t* p_sta = p_days->p_statics[i];
        klbwnd_static_set_css(p_sta, &p_css->css_sta);
    }

    // 第二排 ~ 第七排, 按钮
    for (int m = 0; m < KLBWND_CAL_DAYS_row - 1; m++)
    {
        for (int n = 0; n < KLBWND_CAL_DAYS_column; n++)
        {
            klb_wnd_t* p_btn = p_days->p_btns[m * KLBWND_CAL_DAYS_column + n];
            klbwnd_calendar_btn_set_css(p_btn, &p_css->css_btn);
        }
    }
}


void klbwnd_calendar_days_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    // 重新布局
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int offx = 2;
    int offy = 2;

    int item_w = w / KLBWND_CAL_DAYS_column - offx;
    int item_h = h / KLBWND_CAL_DAYS_row - offy;

    int left_x = (w - (item_w * KLBWND_CAL_DAYS_column + offx * (KLBWND_CAL_DAYS_column - 1))) / 2;
    int sx = left_x;
    int sy = (h - (item_h * KLBWND_CAL_DAYS_row + offy * (KLBWND_CAL_DAYS_row - 1))) / 2;

    char str[32] = { 0 };

    // 第一排 静态框
    for (int i = 0; i < KLBWND_CAL_DAYS_column; i++)
    {
        klb_wnd_t* p_sta = p_days->p_statics[i];

        klb_wnd_move(p_sta, sx, sy);
        klb_wnd_resize(p_sta, item_w, item_h);

        sx += item_w + offx;

        snprintf(str, sizeof(str) - 1, "%d", i + 1);
        klbwnd_static_set_title(p_sta, str);
    }

    // 第二排 ~ 第七排, 按钮
    for (int m = 0; m < KLBWND_CAL_DAYS_row - 1; m++)
    {
        sx = left_x;
        sy += item_h + offy;

        for (int n = 0; n < KLBWND_CAL_DAYS_column; n++)
        {
            klb_wnd_t* p_btn = p_days->p_btns[m* KLBWND_CAL_DAYS_column + n];

            klb_wnd_move(p_btn, sx, sy);
            klb_wnd_resize(p_btn, item_w, item_h);

            sx += item_w + offx;

            //klbwnd_calendar_btn_set_title(p_btn, "2");
        }
    }
}

void klbwnd_calendar_days_set_date_page(klb_wnd_t* p_wnd, klbwnd_calendar_ymd_t* p_ymd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    int year = p_ymd->year;
    int month = p_ymd->month;
    
    int week = klbui_weekday(year, month, 0);

    char str[32] = { 0 };
    int day = 1;

    // 前一个月份的日期
    int prev_year = 0, prev_month = 0;
    klbui_prev_month(year, month, &prev_year, &prev_month);
    int prev_month_days = klbui_month_days(prev_year, prev_month);

    for (int i = 0; i < week; i++)
    {
        day = prev_month_days - week + i + 1;
        snprintf(str, sizeof(str), "%d", day);

        klbwnd_calendar_btn_set_title(p_days->p_btns[i], str);
        klbwnd_calendar_btn_set_ymd(p_days->p_btns[i], prev_year, prev_month, day);
    }

    // 当前月份日期
    int month_days = klbui_month_days(year, month);
    day = 1;
    for (int i = week; i < month_days + week; i++)
    {
        snprintf(str, sizeof(str), "%d", day);

        klbwnd_calendar_btn_set_title(p_days->p_btns[i], str);
        klbwnd_calendar_btn_set_ymd(p_days->p_btns[i], year, month, day);

        day += 1;
    }

    // 后一个月份日期
    int next_year = 0, next_month = 0;
    klbui_next_month(year, month, &next_year, &next_month);
    for (int i = month_days + week; i < (KLBWND_CAL_DAYS_row - 1) * KLBWND_CAL_DAYS_column; i++)
    {
        day = i - month_days - week + 1;
        snprintf(str, sizeof(str), "%d", i - month_days - week + 1);

        klbwnd_calendar_btn_set_title(p_days->p_btns[i], str);
        klbwnd_calendar_btn_set_ymd(p_days->p_btns[i], next_year, next_month, day);
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_calendar_days_init_attribute(klbwnd_calendar_days_t* p_days)
{

}

static void klbwnd_calendar_days_quit_attribute(klbwnd_calendar_days_t* p_days)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit


//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_calendar_days_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;


    // 第一排 静态框
    for (int i = 0; i < KLBWND_CAL_DAYS_column; i++)
    {
        klb_wnd_t* p_sta = klbwnd_static_create(p_gui, 0, 0, 32, 32);
        klb_wnd_push_child(p_wnd, p_sta);
        p_days->p_statics[i] = p_sta;
    }


    // 第二排 ~ 第七排, 按钮
    for (int m = 0; m < KLBWND_CAL_DAYS_row - 1; m++)
    {
        for (int n = 0; n < KLBWND_CAL_DAYS_column; n++)
        {
            klb_wnd_t* p_btn = klbwnd_calendar_btn_create(p_gui, 0, 0, 32, 32);
            klb_wnd_push_child(p_wnd, p_btn);
            p_days->p_btns[m * KLBWND_CAL_DAYS_column + n] = p_btn;
        }
    }

    // 重新布局
    klbwnd_calendar_days_relayout(p_wnd);
}

void klbwnd_calendar_days_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                                   // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_calendar_days_on_control;     // 这里指定
    p_wnd->vtable.on_command = NULL;                                // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                                    // 由继承者重写
    p_wnd->vtable.on_get = NULL;                                    // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 初始化属性
    klbwnd_calendar_days_init_attribute(p_days);

    // 初始化子控件 
    klbwnd_calendar_days_init_subwnds(p_wnd);
}

void klbwnd_calendar_days_quit(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_days_t* p_days = (klbwnd_calendar_days_t*)p_wnd->ctrl;

    klbwnd_calendar_days_quit_attribute(p_days);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_calendar_days_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_calendar_days_t));

    klbwnd_calendar_days_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_calendar_days_destroy;

    return p_wnd;
}
