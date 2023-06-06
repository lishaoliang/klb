// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_tab.h"
#include "klbgui/subviews/klbwnd_tab_btn.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_tab_quit_attribute(klbwnd_tab_t* p_tab);
static int klbwnd_tab_on_onpredraw(klb_wnd_t* p_wnd, klbwnd_tab_t* p_tab);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_tab_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    klbwnd_tab_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_tab_on_paint_status(klb_wnd_t* p_wnd, klbwnd_tab_t* p_tab, klbwnd_tab_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    klb_rect_t head_rect = { p_rect->x, p_rect->y, p_rect->w, p_css->btn_css.height };
    klb_wnd_draw_fill_rect2(p_wnd, &head_rect, p_css->head_background);

    // tab按钮下方横线
    int line_h = p_css->head_line_h;
    if (0 < line_h)
    {
        klb_rect_t line_rect = { p_rect->x, p_rect->y + p_css->btn_css.height - line_h, p_rect->w, line_h };
        klb_wnd_draw_fill_rect2(p_wnd, &line_rect, p_css->head_line_background_color);
    }

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
}

static int klbwnd_tab_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_tab_css_t* p_css = p_tab->p_css;

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

    klbwnd_tab_on_paint_status(p_wnd, p_tab, p_css, &p_css->normal, &paint_rect);

    return 0;
}

static int klbwnd_tab_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_tab_on_paint(p_wnd);
        break;
    case KLBUI_onpredraw:
        return klbwnd_tab_on_onpredraw(p_wnd, p_tab);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有

static void klbwnd_tab_resize_btns(klb_wnd_t* p_wnd, klbwnd_tab_t* p_tab)
{
    klbwnd_tab_css_t* p_css = p_tab->p_css;

    // 重新计算 tab按钮 的 位置 及大小
    int w_min = p_css->btn_css.width_min;
    int w_max = p_css->btn_css.width_max;
    int h = p_css->btn_css.height;
    int x = 0;

    for (int i = 0; i < p_tab->btn_count; i++)
    {
        klb_wnd_t* p_btn = p_tab->p_btns[i];

        int size = 0;
        klbwnd_tab_btn_title_size(p_btn, &size, NULL); // 取标题大小

        size += p_css->btn_css.padding.left + p_css->btn_css.padding.right; // + 内边距
        size += 16; // + N像素

        int w = MAX(size, w_min);
        w = MIN(w, w_max);

        // 移动窗口
        klb_wnd_move(p_btn, x, 0);
        klb_wnd_resize(p_btn, w, h);

        x += w;
    }
}

static void klbwnd_tab_show_page(klbwnd_tab_t* p_tab, int idx)
{
    for (int i = 0; i < p_tab->btn_count; i++)
    {
        klb_wnd_t* p_bind = klbwnd_tab_btn_get_bind_wnd(p_tab->p_btns[i]);
        if (i == idx)
        {
            klbwnd_tab_btn_check(p_tab->p_btns[i], true);
            klb_wnd_show(p_bind, true);
        }
        else
        {
            klbwnd_tab_btn_check(p_tab->p_btns[i], false);
            klb_wnd_show(p_bind, false);
        }
    }
}

static int on_command_tab_btn_klbwnd_tab(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_owner = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_owner->ctrl;

    switch (msg)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            int idx = klbwnd_tab_btn_get_index(p_wnd);
            klbwnd_tab_show_page(p_tab, idx);
        }
        break;
    default:
        break;
    }

    return 0;
}

static bool is_tab_btn_klbwnd_tab(klbwnd_tab_t* p_tab, klb_wnd_t* p_dst)
{
    for (int i = 0; i < p_tab->btn_count; i++)
    {
        if (p_dst == p_tab->p_btns[i])
        {
            return true;
        }
    }

    return false;
}

static bool is_bind_wnd_klbwnd_tab(klbwnd_tab_t* p_tab, klb_wnd_t* p_dst)
{
    for (int i = 0; i < p_tab->btn_count; i++)
    {
        klb_wnd_t* p_bind = klbwnd_tab_btn_get_bind_wnd(p_tab->p_btns[i]);
        if (p_dst == p_bind)
        {
            return true;
        }
    }

    return false;
}

static int klbwnd_tab_on_onpredraw(klb_wnd_t* p_wnd, klbwnd_tab_t* p_tab)
{
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_parent;
    klbwnd_tab_css_t* p_css = p_tab->p_css;

    // 动态检查tab页是否与子窗口一致
    int w = p_css->btn_css.width_min;
    int h = p_css->btn_css.height;

    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_t* p_cur = p_next;
        if (!is_tab_btn_klbwnd_tab(p_tab, p_cur) && !is_bind_wnd_klbwnd_tab(p_tab, p_cur))
        {
            // 子窗口
            int index = p_tab->btn_count;

            klb_wnd_t* p_btn = klbwnd_tab_btn_create(p_wnd->p_gui, w * index, 0, w, h);
            klb_wnd_push_child(p_wnd, p_btn);

            klbwnd_tab_btn_set_css(p_btn, &p_css->btn_css);
            klbwnd_tab_btn_set_index(p_btn, index);
            klbwnd_tab_btn_set_bind_wnd(p_btn, p_cur);
            klbwnd_tab_btn_update_title(p_btn);

            klb_wnd_bind_command(p_btn, on_command_tab_btn_klbwnd_tab, p_wnd); // 绑定响应

            p_tab->p_btns[index] = p_btn;
            p_tab->btn_count += 1;
        }

        p_next = p_next->p_next;
    }

    // 重设按钮位置
    klbwnd_tab_resize_btns(p_wnd, p_tab);

    klbwnd_tab_show_page(p_tab, 0);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_tab_set_css(klb_wnd_t* p_wnd, klbwnd_tab_css_t* p_css)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    p_tab->p_css = p_css;
}

void klbwnd_tab_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    p_tab->title = sdscpy(p_tab->title, p_title);
}

const sds klbwnd_tab_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    return p_tab->title;
}

void klbwnd_tab_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    p_tab->value = sdscpy(p_tab->value, p_value);
}

const sds klbwnd_tab_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    return p_tab->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_tab_init_attribute(klbwnd_tab_t* p_tab)
{
    p_tab->title = sdsempty();
    p_tab->value = sdsempty();
}

static void klbwnd_tab_quit_attribute(klbwnd_tab_t* p_tab)
{
    KLB_FREE_BY(p_tab->title, sdsfree);
    KLB_FREE_BY(p_tab->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_tab_css_init(klbwnd_tab_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
 
    klbwnd_tab_btn_css_init(&p_css->btn_css, p_gui);

    p_css->head_background = KLB_ARGB8888(255, 35, 35, 35);
    p_css->head_line_background_color = KLB_ARGB8888(255, 46, 46, 46);
    p_css->head_line_h = 2;
}

void klbwnd_tab_css_quit(klbwnd_tab_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbwnd_tab_btn_css_quit(&p_css->btn_css);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_tab_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_tab_on_control;       // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 
    klbwnd_tab_init_attribute(p_tab);
}

void klbwnd_tab_quit(klb_wnd_t* p_wnd)
{
    klbwnd_tab_t* p_tab = (klbwnd_tab_t*)p_wnd->ctrl;

    klbwnd_tab_quit_attribute(p_tab);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_tab_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_tab_t));

    klbwnd_tab_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_tab_destroy;

    return p_wnd;
}
