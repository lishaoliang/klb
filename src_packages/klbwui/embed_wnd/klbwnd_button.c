// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_button.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_util.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_button_deinit_attribute(klbwnd_button_t* p_btn);
static void klbwnd_button_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_button_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    klbwnd_button_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_button_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_button_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    // 有背景图时图内自带边框; 纯色才画 border
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static int klbwnd_button_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_button_css_t* p_css = p_btn->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_button_attributes_t* p_attr = NULL;

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        if (klb_wnd_is_check(p_wnd))
        {
            p_attr = &p_css->check_disable;
        }
        else
        {
            p_attr = &p_css->disable;
        }
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        if (klb_wnd_is_check(p_wnd))
        {
            p_attr = &p_css->check_focus;
        }
        else
        {
            p_attr = &p_css->focus;
        }
    }
    else
    {
        if (klb_wnd_is_check(p_wnd))
        {
            p_attr = &p_css->check_normal;
        }
        else
        {
            p_attr = &p_css->normal;
        }
    }

    klbwnd_button_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    klbuicssex_draw_text(p_wnd, p_btn->title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);

    return 0;
}

static int klbwnd_button_on_focusdelay(klb_wnd_t* p_wnd, klbwnd_button_t* p_btn)
{
    klbwnd_button_css_t* p_css = p_btn->p_css;

    if (NULL == p_css || !klb_wnd_is_dyntip(p_wnd))
    {
        return 0;
    }

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;
    klbuiutil_remove_margin(&rect, &p_css->margin);

    int font_size = p_css->normal.font.size;
    if (font_size < p_css->focus.font.size) { font_size = p_css->focus.font.size; }
    if (font_size < p_css->disable.font.size) { font_size = p_css->disable.font.size; }

    if (0 < sdslen(p_btn->title))
    {
        klbwnd_button_update_tip_dynamic(p_wnd, &rect, p_btn->title, font_size);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }

    return 0;
}

static int klbwnd_button_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_button_on_paint(p_wnd);
        break;

    case KLBUI_focusdelay:
        klbwnd_button_on_focusdelay(p_wnd, p_btn);
        break;

    default:
        break;
    }

    return 0;
}

static void klbwnd_button_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size)
{
    if (NULL == p_wnd || NULL == p_rect || NULL == title)
    {
        return;
    }

    int txt_w = 0;
    klb_wnd_text_size2(p_wnd, title, sdslen(title), font_size, &txt_w, NULL);

    if (p_rect->w < txt_w)
    {
        klb_wnd_set_tip_dynamic(p_wnd, title);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_button_set_css(klb_wnd_t* p_wnd, klbwnd_button_css_t* p_css)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    p_btn->p_css = p_css;
}

void klbwnd_button_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    p_btn->index = index;
}

int klbwnd_button_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    return p_btn->index;
}

void klbwnd_button_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    p_btn->title = sdscpy(p_btn->title, p_title);
}

const sds klbwnd_button_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    return p_btn->title;
}

void klbwnd_button_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    p_btn->value = sdscpy(p_btn->value, p_value);
}

const sds klbwnd_button_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    return p_btn->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_button_init_attribute(klb_wnd_t* p_wnd, klbwnd_button_t* p_btn)
{
    klb_wnd_dyntip(p_wnd, true);

    p_btn->index = 0;
    p_btn->title = sdsempty();
    p_btn->value = sdsempty();
}

static void klbwnd_button_deinit_attribute(klbwnd_button_t* p_btn)
{
    KLB_FREE_BY(p_btn->title, sdsfree);
    KLB_FREE_BY(p_btn->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_button_attributes_init(klbwnd_button_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_button_attributes_deinit(klbwnd_button_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_button_attributes_copy(klbwnd_button_attributes_t* p_dst, const klbwnd_button_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_button_css_init(klbwnd_button_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_button_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_button_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_button_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_button_attributes_init(&p_css->check_normal, &p_default->normal);
    klbwnd_button_attributes_init(&p_css->check_focus, &p_default->focus);
    klbwnd_button_attributes_init(&p_css->check_disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_center;
    p_css->focus.text.align = KLBUICSS_text1_center;
    p_css->disable.text.align = KLBUICSS_text1_center;

    p_css->check_normal.text.align = KLBUICSS_text1_center;
    p_css->check_focus.text.align = KLBUICSS_text1_center;
    p_css->check_disable.text.align = KLBUICSS_text1_center;

    p_css->check_normal.background.color = KLB_ARGB8888(255, 32, 32, 61);
    p_css->check_focus.background.color = KLB_ARGB8888(255, 32, 32, 61);
    p_css->check_disable.background.color = KLB_ARGB8888(255, 32, 32, 61);
}

void klbwnd_button_css_deinit(klbwnd_button_css_t* p_css)
{
    klbwnd_button_attributes_deinit(&p_css->normal);
    klbwnd_button_attributes_deinit(&p_css->focus);
    klbwnd_button_attributes_deinit(&p_css->disable);

    klbwnd_button_attributes_deinit(&p_css->check_normal);
    klbwnd_button_attributes_deinit(&p_css->check_focus);
    klbwnd_button_attributes_deinit(&p_css->check_disable);
}

void klbwnd_button_css_copy(klbwnd_button_css_t* p_dst, klbwnd_button_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_button_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_button_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_button_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_button_attributes_copy(&p_dst->check_normal, &p_src->check_normal);
    klbwnd_button_attributes_copy(&p_dst->check_focus, &p_src->check_focus);
    klbwnd_button_attributes_copy(&p_dst->check_disable, &p_src->check_disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_button_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_button_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = KLB_WND_STYLE_FOCUS_DELAY;

    klbwnd_button_init_attribute(p_wnd, p_btn);
}

void klbwnd_button_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_button_t* p_btn = (klbwnd_button_t*)p_wnd->ctrl;

    klbwnd_button_deinit_attribute(p_btn);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_button_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_button_t));

    klbwnd_button_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_button_destroy;

    return p_wnd;
}

// end
