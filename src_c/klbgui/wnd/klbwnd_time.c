// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_time.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_time_quit_attribute(klbwnd_time_t* p_time);
static void klbwnd_time_relayout(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klb_rect_t* p_rect, int font_h);
static void klbwnd_time_limit(int* p_value, int min, int max);
static void sync_title_klbwnd_time(klb_wnd_t* p_wnd, klbwnd_time_t* p_time);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_time_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    klbwnd_time_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_time_on_paint_status(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klbwnd_time_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 重新布局
    klb_rect_t rect = *p_rect;
    rect.x += p_css->padding.left;
    rect.y += p_css->padding.top;
    rect.w -= (p_css->padding.left + p_css->padding.right);
    rect.h -= (p_css->padding.top + p_css->padding.bottom);

    rect.x += p_attr->border.width.left;
    rect.y += p_attr->border.width.top;
    rect.w -= (p_attr->border.width.left + p_attr->border.width.right);
    rect.w -= (p_attr->border.width.top + p_attr->border.width.bottom);

    klbwnd_time_relayout(p_wnd, p_time, &rect, p_attr->font.size);

    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    // :
    for (int i = 0; i < 2; i++)
    {
        klbwnd_time_region_t* p_region = &p_time->colon[i];
        klbuicssex_draw_text(p_wnd, p_region->title, &p_region->rect, NULL, NULL, &p_attr->text, &p_attr->font);
    }

    // 时分秒
    for (int i = 0; i < 3; i++)
    {
        klbwnd_time_region_t* p_region = &p_time->hms[i];
        klbuicssex_draw_text(p_wnd, p_region->title, &p_region->rect, NULL, NULL, &p_attr->text, &p_attr->font);
    }
}

static int klbwnd_time_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_time_css_t* p_css = p_time->p_css;

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

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_time_on_paint_status(p_wnd, p_time, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_popup_end_klbwnd_time(void* ptr, klb_wnd_t* p_wnd_dec, bool ok, int value)
{
    klb_wnd_t* p_wnd_time = (klb_wnd_t*)ptr;
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd_time->ctrl;

    if (KLBWND_TIME_idx_hour == p_time->sel_idx)
    {
        klbwnd_time_limit(&value, 0, 23);
        p_time->hour = value;
    }
    else if(KLBWND_TIME_idx_minute == p_time->sel_idx)
    {
        klbwnd_time_limit(&value, 0, 59);
        p_time->minute = value;
    }
    else
    {
        klbwnd_time_limit(&value, 0, 59);
        p_time->second = value;
    }

    sync_title_klbwnd_time(p_wnd_time, p_time);

    return 0;
}

static int klbwnd_time_on_click(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, const klb_point_t* p_pt1)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    // 点中哪个区域
    klb_rect_t rect_pt = { 0 };
    int sel = -1;
    for (int i = 0; i < 3; i++)
    {
        if (klb_pt_in_rect(&p_time->hms[i].rect, p_pt1->x, p_pt1->y))
        {
            sel = i;
            rect_pt = p_time->hms[i].rect;
            break;
        }
    }

    // 若点中
    if (0 <= sel)
    {
        // 0.
        p_time->sel_idx = sel;

        int value = p_time->second;
        if (KLBWND_TIME_idx_hour == sel)
        {
            value = p_time->hour;
        }
        else if (KLBWND_TIME_idx_minute == sel)
        {
            value = p_time->minute;
        }


        // 1. 设置初始值
        klbshw_decimal_set_value(p_time->p_decimal, value);
        klbshw_decimal_set_max_len(p_time->p_decimal, 2);

        // 2. 设置css

        // 3. 绑定响应
        klbshw_decimal_bind(p_time->p_decimal, on_popup_end_klbwnd_time, p_wnd);

        // 4. 处理位置 
        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

        int menu_w = 0, menu_h = 0;
        klbshw_decimal_wh(p_wnd->p_gui, &menu_w, &menu_h);

        klb_rect_t rect = p_wnd->pos.rect_in_canvas;

        int x = (rect_pt.x + menu_w <= screen_w) ? rect_pt.x : screen_w - menu_w;
        int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

        klb_wnd_move(p_time->p_decimal, x, y);

        // 5. popup
        klb_gui_popup_wnd(p_wnd->p_gui, p_time->p_decimal);
    }

    return 0;
}

static int klbwnd_time_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_time_on_paint(p_wnd);

    case KLBUI_click:
    case KLBUI_dblclick:
        klbwnd_time_on_click(p_wnd, p_time, p_pt1);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

static void klbwnd_time_relayout(klb_wnd_t* p_wnd, klbwnd_time_t* p_time, klb_rect_t* p_rect, int font_h)
{
    int w_hour = 0, w_minute = 0, w_second = 0, w_colon0 = 0, w_colon1 = 0;
    sds title = NULL;

    // 时
    title = p_time->hms[0].title;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_h, &w_hour, NULL);

    // 分
    title = p_time->hms[1].title;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_h, &w_minute, NULL);

    // 秒
    title = p_time->hms[2].title;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_h, &w_second, NULL);

    // 冒号0
    title = p_time->colon[0].title;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_h, &w_colon0, NULL);

    // 冒号1
    title = p_time->colon[1].title;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_h, &w_colon1, NULL);

    int w_fix = 2;
    int offx = 1;

    int sx = p_rect->x;
    int sy = p_rect->y;
    int h = p_rect->h;

    // 时
    klb_rect_init(&p_time->hms[0].rect, sx, sy, w_hour + w_fix, h);
    sx += w_hour + w_fix + offx;

    // :
    klb_rect_init(&p_time->colon[0].rect, sx, sy, w_colon0 + w_fix, h);
    sx += w_colon0 + w_fix + offx;

    // 分
    klb_rect_init(&p_time->hms[1].rect, sx, sy, w_minute + w_fix, h);
    sx += w_minute + w_fix + offx;

    // :
    klb_rect_init(&p_time->colon[1].rect, sx, sy, w_colon1 + w_fix, h);
    sx += w_colon1 + w_fix + offx;

    // 秒
    klb_rect_init(&p_time->hms[2].rect, sx, sy, w_second + w_fix, h);
    sx += w_second + w_fix + offx;
}

static void klbwnd_time_limit(int* p_value, int min, int max)
{
    if (*p_value <= min) { *p_value = min; }
    if (max <= *p_value) { *p_value = max; }
}

static sds klbwnd_time_sdscpy_int(sds s, int v)
{
    sds dst = (NULL != s) ? s : sdsempty();

    char str[16] = { 0 };
    snprintf(str, sizeof(str) - 1, "%02d", v);

    return sdscpy(dst, str);
}

static void sync_title_klbwnd_time(klb_wnd_t* p_wnd, klbwnd_time_t* p_time)
{
    p_time->hms[0].title = klbwnd_time_sdscpy_int(p_time->hms[0].title, p_time->hour);
    p_time->hms[1].title = klbwnd_time_sdscpy_int(p_time->hms[1].title, p_time->minute);
    p_time->hms[2].title = klbwnd_time_sdscpy_int(p_time->hms[2].title, p_time->second);

    p_time->colon[0].title = klb_sdscpy(p_time->colon[0].title, ":");
    p_time->colon[1].title = klb_sdscpy(p_time->colon[1].title, ":");

    p_time->apm.title = klb_sdscpy(p_time->apm.title, "AM");
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_time_set_css(klb_wnd_t* p_wnd, klbwnd_time_css_t* p_css)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    p_time->p_css = p_css;
}

void klbwnd_time_set_value(klb_wnd_t* p_wnd, int hour, int minute, int second)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    klbwnd_time_limit(&hour, 0, 23);
    klbwnd_time_limit(&minute, 0, 59);
    klbwnd_time_limit(&second, 0, 59);

    p_time->hour = hour;
    p_time->minute = minute;
    p_time->second = second;

    sync_title_klbwnd_time(p_wnd, p_time);
}

void klbwnd_time_get_value(klb_wnd_t* p_wnd, int* p_hour, int* p_minute, int* p_second)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    if (p_hour) { *p_hour = p_time->hour; };
    if (p_minute) { *p_minute = p_time->minute; };
    if (p_second) { *p_second = p_time->second; };
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_time_init_attribute(klb_wnd_t* p_wnd, klbwnd_time_t* p_time)
{
    p_time->sel_idx = -1;

    p_time->hour = 0;
    p_time->minute = 0;
    p_time->second = 0;

    sync_title_klbwnd_time(p_wnd, p_time);
}

static void klbwnd_time_quit_attribute(klbwnd_time_t* p_time)
{
    for (int i = 0; i < 3; i++)
    {
        KLB_FREE_BY(p_time->hms[i].title, sdsfree);
    }

    for (int i = 0; i < 2; i++)
    {
        KLB_FREE_BY(p_time->colon[i].title, sdsfree);
    }

    KLB_FREE_BY(p_time->apm.title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_time_css_init(klbwnd_time_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_time_css_quit(klbwnd_time_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

void klbwnd_time_css_copy(klbwnd_time_css_t* p_dst, klbwnd_time_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_time_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                       // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_time_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                    // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                        // 由继承者重写
    p_wnd->vtable.on_get = NULL;                        // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 内部属性
    klbwnd_time_init_attribute(p_wnd, p_time);

    // 10进制软件键盘
    p_time->p_decimal = klbui_shwnd_get_decimal(p_gui);
}

void klbwnd_time_quit(klb_wnd_t* p_wnd)
{
    klbwnd_time_t* p_time = (klbwnd_time_t*)p_wnd->ctrl;

    klbwnd_time_quit_attribute(p_time);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_time_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_time_t));

    klbwnd_time_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_time_destroy;

    return p_wnd;
}
