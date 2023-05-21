// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_date.h"
#include "klbgui/wnd/klbwnd_calendar.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_date_quit_attribute(klbwnd_date_t* p_date);
static void sync_title_klbwnd_date(klb_wnd_t* p_wnd, klbwnd_date_t* p_date);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_date_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    klbwnd_date_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_date_on_paint_status(klb_wnd_t* p_wnd, klbwnd_date_t* p_date, klbwnd_date_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (0 < sdslen(p_attr->background.image))
    {
        // 图片背景
        klb_wnd_draw_image(p_wnd, p_rect, p_attr->background.image, NULL);
    }
    else
    {
        // 纯色背景
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

        // 边框
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    // 标题文本
    klbuicssex_draw_text(p_wnd, p_date->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_date_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_date_css_t* p_css = p_date->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    if (KLB_WND_STYLE_NOFOCUS & p_wnd->state.style)
    {
        klbwnd_date_on_paint_status(p_wnd, p_date, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_date_on_paint_status(p_wnd, p_date, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_date_on_paint_status(p_wnd, p_date, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_popup_end_klbwnd_date(void* ptr, klb_wnd_t* p_wnd_cal, bool ok, int year, int month, int day)
{
    klb_wnd_t* p_wnd_date = (klb_wnd_t*)ptr;
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd_date->ctrl;

    if (ok)
    {
        p_date->year = year;
        p_date->month = month;
        p_date->day = day;

        sync_title_klbwnd_date(p_wnd_date, p_date);

        klb_wnd_update(p_wnd_date);
    }

    return 0;
}

static int klbwnd_date_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_date_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            // 1. 设置初始值
            klbshw_calendar_set_date(p_date->p_calendar, p_date->year, p_date->month, p_date->day);

            // 2. 设置css


            // 3. 绑定响应
            klbshw_calendar_bind(p_date->p_calendar, on_popup_end_klbwnd_date, p_wnd);

            // 4. 处理位置 
            int screen_w = 0, screen_h = 0;
            klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

            int menu_w = 0, menu_h = 0;
            klbshw_calendar_wh(p_wnd->p_gui, &menu_w, &menu_h);

            klb_rect_t rect = p_wnd->pos.rect_in_canvas;

            int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
            int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

            klb_wnd_move(p_date->p_calendar, x, y);

            // 5. popup
            klb_gui_popup_wnd(p_wnd->p_gui, p_date->p_calendar);
        }
        break;
    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 内部私有函数

static void sync_title_klbwnd_date(klb_wnd_t* p_wnd, klbwnd_date_t* p_date)
{
    sdsclear(p_date->title);
    p_date->title = sdscatfmt(p_date->title, "%i/%i/%i", p_date->year, p_date->month, p_date->day);
}

//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_date_set_css(klb_wnd_t* p_wnd, klbwnd_date_css_t* p_css)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    p_date->p_css = p_css;
}

void klbwnd_date_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    bool ok = false;
    int year = 0, month = 0, day = 0;

    if (3 == sscanf(p_title, "%d/%d/%d", &year, &month, &day))
    {
        ok = true;
    }
    else if( 3 == sscanf(p_title, "%d-%d-%d", &year, &month, &day))
    {
        ok = true;
    }

    if (ok)
    {
        // 限制正确日期
        klbwnd_calendar_limit(&year, &month, &day);

        // 更新
        p_date->year = year;
        p_date->month = month;
        p_date->day = day;

        // 同步标题
        sync_title_klbwnd_date(p_wnd, p_date);
    }
}

const sds klbwnd_date_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    return p_date->title;
}

void klbwnd_date_set_value(klb_wnd_t* p_wnd, int year, int month, int day)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    // 限制正确日期
    klbwnd_calendar_limit(&year, &month, &day);

    // 更新
    p_date->year = year;
    p_date->month = month;
    p_date->day = day;

    // 同步标题
    sync_title_klbwnd_date(p_wnd, p_date);
}

void klbwnd_date_get_value(klb_wnd_t* p_wnd, int* p_year, int* p_month, int* p_day)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    if (p_year) { *p_year = p_date->year; };
    if (p_month) { *p_month = p_date->month; };
    if (p_day) { *p_day = p_date->day; };
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_date_init_attribute(klb_wnd_t* p_wnd, klbwnd_date_t* p_date)
{
    p_date->title = sdsempty();
    p_date->value = sdsempty();

    p_date->year = KLBWND_CAL_YEAR_min;
    p_date->month = 1;
    p_date->day = 1;

    sync_title_klbwnd_date(p_wnd, p_date);
}

static void klbwnd_date_quit_attribute(klbwnd_date_t* p_date)
{
    KLB_FREE_BY(p_date->title, sdsfree);
    KLB_FREE_BY(p_date->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_date_css_init(klbwnd_date_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_date_css_quit(klbwnd_date_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_date_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                       // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_date_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                    // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                        // 由继承者重写
    p_wnd->vtable.on_get = NULL;                        // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 内部属性
    klbwnd_date_init_attribute(p_wnd, p_date);

    // 日历菜单
    p_date->p_calendar = klbui_shwnd_get_calendar(p_gui);
}

void klbwnd_date_quit(klb_wnd_t* p_wnd)
{
    klbwnd_date_t* p_date = (klbwnd_date_t*)p_wnd->ctrl;

    klbwnd_date_quit_attribute(p_date);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_date_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_date_t));

    klbwnd_date_init(p_wnd, p_gui, x, y, w, h);

    // 重写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_date_destroy;

    return p_wnd;
}
