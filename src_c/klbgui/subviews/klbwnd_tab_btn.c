// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_tab_btn.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_tab_btn_quit_attribute(klbwnd_tab_btn_t* p_btn);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_tab_btn_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    klbwnd_tab_btn_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_tab_btn_on_paint_status(klb_wnd_t* p_wnd, klbwnd_tab_btn_t* p_btn, klbwnd_tab_btn_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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

    // 选中
    if (0 < p_css->check_height)
    {
        klb_rect_t check_rect = { p_rect->x, p_rect->y + p_rect->h - p_css->check_height, p_rect->w, p_css->check_height };

        if (p_btn->is_check)
        {
            klb_wnd_draw_fill_rect2(p_wnd, &check_rect, p_css->check_background_color);
        }
    }

    // 标题文本
    klbuicssex_draw_text(p_wnd, p_btn->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
}

static int klbwnd_tab_btn_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_tab_btn_css_t* p_css = p_btn->p_css;

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
        klbwnd_tab_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_tab_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_tab_btn_on_paint_status(p_wnd, p_btn, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_tab_btn_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_tab_btn_on_paint(p_wnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_tab_btn_set_css(klb_wnd_t* p_wnd, klbwnd_tab_btn_css_t* p_css)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->p_css = p_css;
}

void klbwnd_tab_btn_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->title = sdscpy(p_btn->title, p_title);
}

const sds klbwnd_tab_btn_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    return p_btn->title;
}

void klbwnd_tab_btn_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->value = sdscpy(p_btn->value, p_value);
}

const sds klbwnd_tab_btn_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    return p_btn->value;
}

//////////////////////////////////////////////////////////////////////////

void klbwnd_tab_btn_set_index(klb_wnd_t* p_wnd, int idx)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->index = idx;
}

int klbwnd_tab_btn_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    return p_btn->index;
}

void klbwnd_tab_btn_set_bind_wnd(klb_wnd_t* p_wnd, klb_wnd_t* p_dst)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->p_bind_wnd = p_dst;
}

klb_wnd_t* klbwnd_tab_btn_get_bind_wnd(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    return p_btn->p_bind_wnd;
}

void klbwnd_tab_btn_check(klb_wnd_t* p_wnd, bool check)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_btn->is_check = check;

    klb_wnd_update(p_wnd);
}

void klbwnd_tab_btn_update_title(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    if (NULL == p_btn->p_bind_wnd)
    {
        return;
    }
    klb_map_t in = { 0 };
    klb_map_init(&in);
    klb_map_set_idx_string(&in, 0, "title");

    klb_map_t* p_out = klb_wnd_get(p_btn->p_bind_wnd, &in);
    if (NULL != p_out)
    {
        const char* p_tile = klb_map_idx_to_string(p_out, 0);
        if (NULL != p_tile)
        {
            p_btn->title = sdscpy(p_btn->title, p_tile);
        }

        klb_map_destroy(p_out);
    }

    klb_map_quit(&in);
}

int klbwnd_tab_btn_title_size(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    if (NULL != p_btn->p_css)
    {
        // 按常规状态测算标题长度
        klb_wnd_text_size2(p_wnd, p_btn->title, sdslen(p_btn->title), p_btn->p_css->normal.font.size, p_out_w, p_out_h);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_tab_btn_init_attribute(klb_wnd_t* p_wnd, klbwnd_tab_btn_t* p_btn)
{
    p_btn->title = sdsempty();
    p_btn->value = sdsempty();
}

static void klbwnd_tab_btn_quit_attribute(klbwnd_tab_btn_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE_BY(p_btn->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_tab_btn_css_init(klbwnd_tab_btn_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.border.width.top = 0;
    p_css->normal.border.width.right = 0;
    p_css->normal.border.width.bottom = 0;
    p_css->normal.border.width.left = 0;

    p_css->normal.background.color = KLB_ARGB8888(255, 46, 46, 46);

    p_css->focus.border.width.top = 0;
    p_css->focus.border.width.right = 0;
    p_css->focus.border.width.bottom = 0;
    p_css->focus.border.width.left = 0;

    p_css->check_height = 2;
    p_css->check_background_color = KLB_ARGB8888(255, 120, 120, 120);
    p_css->uncheck_background_color = p_default->normal.background.color;

    p_css->height = 32;
    p_css->width_min = 200;
    p_css->width_max = 800;
}

void klbwnd_tab_btn_css_quit(klbwnd_tab_btn_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

void klbwnd_tab_btn_css_copy(klbwnd_tab_btn_css_t* p_dst, klbwnd_tab_btn_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_tab_btn_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_tab_btn_on_control;   // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化内部变量
    klbwnd_tab_btn_init_attribute(p_wnd, p_btn);
}

void klbwnd_tab_btn_quit(klb_wnd_t* p_wnd)
{
    klbwnd_tab_btn_t* p_btn = (klbwnd_tab_btn_t*)p_wnd->ctrl;

    klbwnd_tab_btn_quit_attribute(p_btn);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_tab_btn_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_tab_btn_t));

    klbwnd_tab_btn_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_tab_btn_destroy;

    return p_wnd;
}
