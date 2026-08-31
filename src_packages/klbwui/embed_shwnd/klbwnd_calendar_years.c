// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_calendar_years.h"
#include "klbwui/embed_shwnd/klbwnd_calendar_btn.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_calendar_years_deinit_attribute(klbwnd_calendar_years_t* p_years);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_calendar_years_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    klbwnd_calendar_years_deinit(p_wnd);

    KLB_FREE(p_wnd);
}


static int klbwnd_calendar_years_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_calendar_css_t* p_css = p_years->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    // 无需绘图

    return 0;
}


static int klbwnd_calendar_years_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_calendar_years_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_calendar_years_set_css(klb_wnd_t* p_wnd, klbwnd_calendar_css_t* p_css)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    p_years->p_css = p_css;

    for (int m = 0; m < KLBWND_CAL_YEARS_row; m++)
    {
        for (int n = 0; n < KLBWND_CAL_YEARS_column; n++)
        {
            klb_wnd_t* p_btn = p_years->p_btns[m][n];
            klbwnd_calendar_btn_set_css(p_btn, &p_css->css_btn);
        }
    }
}


void klbwnd_calendar_years_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    // 重新布局
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int offx = 2;
    int offy = 2;

    int item_w = w / KLBWND_CAL_YEARS_column - offx;
    int item_h = h / KLBWND_CAL_YEARS_row - offy;

    int left_x = (w - (item_w * KLBWND_CAL_YEARS_column + offx * (KLBWND_CAL_YEARS_column - 1))) / 2;
    int sx = left_x;
    int sy = (h - (item_h * KLBWND_CAL_YEARS_row + offy * (KLBWND_CAL_YEARS_row - 1))) / 2;

    // 按钮
    for (int m = 0; m < KLBWND_CAL_YEARS_row; m++)
    {
        for (int n = 0; n < KLBWND_CAL_YEARS_column; n++)
        {
            klb_wnd_t* p_btn = p_years->p_btns[m][n];

            klb_wnd_move(p_btn, sx, sy);
            klb_wnd_resize(p_btn, item_w, item_h);

            sx += item_w + offx;

            //klbwnd_calendar_btn_set_title(p_btn, "2020");
        }

        sx = left_x;
        sy += item_h + offy;
    }
}

// 设置显示页面的年份
void klbwnd_calendar_years_set_date_page(klb_wnd_t* p_wnd, klbwnd_calendar_ymd_t* p_ymd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    int year = p_ymd->year;
    int month = p_ymd->month;
    int day = p_ymd->day;

    // 按钮总数
    int page_count = (int)KLBWND_CAL_YEARS_row * KLBWND_CAL_YEARS_column; 

    // 第一个显示的年份
    int base_year = ((year - KLBWND_CAL_YEAR_min) / page_count) * page_count + KLBWND_CAL_YEAR_min;

    char str[32] = { 0 };

    for (int m = 0; m < KLBWND_CAL_YEARS_row; m++)
    {
        for (int n = 0; n < KLBWND_CAL_YEARS_column; n++)
        {
            klb_wnd_t* p_btn = p_years->p_btns[m][n];

            int tmp_year = base_year + m * KLBWND_CAL_YEARS_column + n;
            snprintf(str, sizeof(str) - 1, "%d", tmp_year);

            klbwnd_calendar_btn_set_title(p_btn, str);
            klbwnd_calendar_btn_set_ymd(p_btn, tmp_year, month, day);
        };
    }
}

void klbwnd_calendar_years_get_year_ranges(klb_wnd_t* p_wnd, int* p_year_min, int* p_year_max)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    if (p_year_min) 
    { 
        int year_min = 0;
        klbwnd_calendar_btn_get_ymd(p_years->p_btns[0][0], &year_min, NULL, NULL);

        *p_year_min = year_min; 
    }

    if (p_year_max) 
    {
        int year_max = 0;
        klbwnd_calendar_btn_get_ymd(p_years->p_btns[KLBWND_CAL_YEARS_row - 1][KLBWND_CAL_YEARS_column - 1], &year_max, NULL, NULL);

        *p_year_max = year_max;
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_calendar_years_init_attribute(klbwnd_calendar_years_t* p_years)
{

}

static void klbwnd_calendar_years_deinit_attribute(klbwnd_calendar_years_t* p_years)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit



//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_calendar_years_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    for (int m = 0; m < KLBWND_CAL_YEARS_row; m++)
    {
        for (int n = 0; n < KLBWND_CAL_YEARS_column; n++)
        {
            klb_wnd_t* p_btn = klbwnd_calendar_btn_create(p_gui, 0, 0, 32, 32);
            klb_wnd_push_child(p_wnd, p_btn);
            p_years->p_btns[m][n] = p_btn;
        }
    }

    // 重新布局
    klbwnd_calendar_years_relayout(p_wnd);
}

void klbwnd_calendar_years_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_calendar_years_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 属性
    klbwnd_calendar_years_init_attribute(p_years);

    // 子控件
    klbwnd_calendar_years_init_subwnds(p_wnd);
}

void klbwnd_calendar_years_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_calendar_years_t* p_years = (klbwnd_calendar_years_t*)p_wnd->ctrl;

    klbwnd_calendar_years_deinit_attribute(p_years);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_calendar_years_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_calendar_years_t));

    klbwnd_calendar_years_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_calendar_years_destroy;

    return p_wnd;
}

// end
