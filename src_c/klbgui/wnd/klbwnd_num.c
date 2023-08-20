// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_num.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_num_quit_attribute(klbwnd_num_t* p_num);
static void do_set_value_klbwnd_num(klb_wnd_t* p_wnd, klbwnd_num_t* p_num, int value);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_num_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    klbwnd_num_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_num_on_paint_status(klb_wnd_t* p_wnd, klbwnd_num_t* p_num, klbwnd_num_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
    klbuicssex_draw_text(p_wnd, p_num->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_num_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_num_css_t* p_css = p_num->p_css;

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
        klbwnd_num_on_paint_status(p_wnd, p_num, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_num_on_paint_status(p_wnd, p_num, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_num_on_paint_status(p_wnd, p_num, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_decimal_klbwnd_num(void* ptr, klb_wnd_t* p_wnd_dec, bool ok, int value)
{
    klb_wnd_t* p_wnd_num = (klb_wnd_t*)ptr;
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd_num->ctrl;

    if (ok)
    {
        do_set_value_klbwnd_num(p_wnd_num, p_num, value);
        klb_wnd_update(p_wnd_num);
    }

    return 0;
}


static int klbwnd_num_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_num_on_paint(p_wnd);
        break;
    case KLBUI_onpredraw:
        do_set_value_klbwnd_num(p_wnd, p_num, p_num->value);
        break;
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            int max_len = 6;
            {
                char str[32] = { 0 };
                
                snprintf(str, sizeof(str) - 1, "%d", p_num->max);
                int len1 = strlen(str);

                snprintf(str, sizeof(str) - 1, "%d", p_num->min);
                int len2 = strlen(str);

                max_len = MAX(len1, len2);
            }

            // 1. 设置初始值
            klbshw_decimal_set_value(p_num->p_decimal, p_num->value);
            klbshw_decimal_set_max_len(p_num->p_decimal, max_len);

            // 2. 设置css


            // 3. 绑定响应
            klbshw_decimal_bind(p_num->p_decimal, on_decimal_klbwnd_num, p_wnd);

            // 4. 处理位置 
            int screen_w = 0, screen_h = 0;
            klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

            int menu_w = 0, menu_h = 0;
            klbshw_decimal_wh(p_wnd->p_gui, &menu_w, &menu_h);

            klb_rect_t rect = p_wnd->pos.rect_in_canvas;

            int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
            int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

            klb_wnd_move(p_num->p_decimal, x, y);

            // 5. popup
            klb_gui_popup_wnd(p_wnd->p_gui, p_num->p_decimal);
        }
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

static void do_set_value_klbwnd_num(klb_wnd_t* p_wnd, klbwnd_num_t* p_num, int value)
{
    if (value < p_num->min) { value = p_num->min; }
    if (p_num->max < value) { value = p_num->max; }

    p_num->value = value;

    sdsclear(p_num->title);
    p_num->title = sdscatfmt(p_num->title, "%i", p_num->value);
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_num_set_css(klb_wnd_t* p_wnd, klbwnd_num_css_t* p_css)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->p_css = p_css;
}

void klbwnd_num_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->value = value;
}

int klbwnd_num_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    return p_num->value;
}

void klbwnd_num_set_ranges(klb_wnd_t* p_wnd, int min, int max)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_num->min = min;
    p_num->max = max;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_num_init_attribute(klb_wnd_t* p_wnd, klbwnd_num_t* p_num)
{
    p_num->title = sdsempty();

    p_num->value = 0;
    p_num->min = 0;
    p_num->max = 65536;
}

static void klbwnd_num_quit_attribute(klbwnd_num_t* p_num)
{
    KLB_FREE_BY(p_num->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_num_css_init(klbwnd_num_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_num_css_quit(klbwnd_num_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_num_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_num_on_control;       // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化属性
    klbwnd_num_init_attribute(p_wnd, p_num);

    // 菜单
    p_num->p_decimal = klbui_shwnd_get_decimal(p_gui);
}

void klbwnd_num_quit(klb_wnd_t* p_wnd)
{
    klbwnd_num_t* p_num = (klbwnd_num_t*)p_wnd->ctrl;

    klbwnd_num_quit_attribute(p_num);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_num_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_num_t));

    klbwnd_num_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_num_destroy;

    return p_wnd;
}
