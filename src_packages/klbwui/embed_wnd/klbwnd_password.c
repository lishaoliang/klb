// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_password.h"
#include "klbwui/embed_shwnd/klbshw_edit_menu.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_util.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_password_deinit_attribute(klbwnd_password_t* p_passwd);
static void klbwnd_password_sync_hide_title(klbwnd_password_t* p_passwd);
static int klbwnd_password_on_focusdelay(klb_wnd_t* p_wnd, klbwnd_password_t* p_passwd);
static void klbwnd_password_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_password_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_password_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static bool klbwnd_password_has_sds(const sds text)
{
    return (NULL != text && 0 < sdslen(text));
}

static void klbwnd_password_draw_background_and_border(klb_wnd_t* p_wnd, klbwnd_password_attributes_t* p_attr, klb_rect_t* p_rect)
{
    if (klbwnd_password_has_sds(p_attr->stretch_image))
    {
        klbuicss_background_t bg = p_attr->background;
        bg.image = p_attr->stretch_image;
        bg.image_mode = KLBUICSS_bgimg_mode_scale9;
        klbwuicss_draw_background(p_wnd, p_rect, &bg);
    }
    else
    {
        klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

        if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
        {
            klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
        }
    }
}

static sds klbwnd_password_paint_title(const klbwnd_password_t* p_passwd)
{
    if (p_passwd->is_hide_letter)
    {
        return p_passwd->hide_title;
    }

    return p_passwd->title;
}

static int klbwnd_password_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_password_css_t* p_css = p_passwd->p_css;

    if (NULL == p_css)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;

    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbwnd_password_attributes_t* p_attr = NULL;

    if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    {
        p_attr = &p_css->disable;
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        p_attr = &p_css->focus;
    }
    else
    {
        p_attr = &p_css->normal;
    }

    klbwnd_password_draw_background_and_border(p_wnd, p_attr, &paint_rect);

    sds paint_title = klbwnd_password_paint_title(p_passwd);
    if (klbwnd_password_has_sds(paint_title))
    {
        klbuicssex_draw_text(p_wnd, paint_title, &paint_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }

    if (p_css->btn_visibility && (KLB_WND_STATUS_FOCUS & p_wnd->state.status))
    {
        int offset_x = p_css->btn_interval;

        if (klbwnd_password_has_sds(p_attr->btn_clear_image))
        {
            klb_rect_t rect_clear = { 0, 0, p_css->btn_w, p_css->btn_h };
            rect_clear.x = paint_rect.x + paint_rect.w - p_css->btn_w - offset_x;
            rect_clear.y = paint_rect.y + (paint_rect.h - p_css->btn_h) / 2;

            klb_wnd_draw_image(p_wnd, &rect_clear, p_attr->btn_clear_image, NULL);

            offset_x += p_css->btn_w;
            offset_x += p_css->btn_interval;

            rect_clear.x -= p_css->btn_interval / 2;
            rect_clear.y = paint_rect.y;
            rect_clear.w += p_css->btn_interval;
            rect_clear.h = paint_rect.h;
            p_passwd->rect_image[KLBWND_PASSWORD_btn_clear] = rect_clear;
        }

        if (klbwnd_password_has_sds(p_attr->btn_symbol_image))
        {
            klb_rect_t rect_symbol = { 0, 0, p_css->btn_w, p_css->btn_h };
            rect_symbol.x = paint_rect.x + paint_rect.w - p_css->btn_w - offset_x;
            rect_symbol.y = paint_rect.y + (paint_rect.h - p_css->btn_h) / 2;

            klb_wnd_draw_image(p_wnd, &rect_symbol, p_attr->btn_symbol_image, NULL);

            rect_symbol.x -= p_css->btn_interval / 2;
            rect_symbol.y = paint_rect.y;
            rect_symbol.w += p_css->btn_interval;
            rect_symbol.h = paint_rect.h;
            p_passwd->rect_image[KLBWND_PASSWORD_btn_symbol] = rect_symbol;
        }
    }

    return 0;
}

static int on_menu_popup_end_klbwnd_password(void* ptr, klb_wnd_t* p_wnd_menu, bool ok, const sds value)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    if (ok)
    {
        const char* p_old = (NULL != p_passwd->title) ? p_passwd->title : "";
        const char* p_new = (NULL != value) ? value : "";
        bool change = (0 != strcmp(p_old, p_new));

        klbwnd_password_set_title(p_wnd, value);

        if (change)
        {
            klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
        }

        klb_wnd_update(p_wnd);
    }

    return 0;
}

static int klbwnd_password_on_click(klb_wnd_t* p_wnd, klbwnd_password_t* p_passwd, const klb_point_t* p_pt1)
{
    klbwnd_password_css_t* p_css = p_passwd->p_css;

    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    if (NULL != p_css && p_css->btn_visibility && NULL != p_pt1)
    {
        if (klb_pt_in_rect(&p_passwd->rect_image[KLBWND_PASSWORD_btn_symbol], p_pt1->x, p_pt1->y))
        {
            p_passwd->is_hide_letter = !p_passwd->is_hide_letter;
            klbwnd_password_on_focusdelay(p_wnd, p_passwd);
            klb_wnd_tip_update(p_wnd);
            klb_wnd_update(p_wnd);
            return 0;
        }
        else if (klb_pt_in_rect(&p_passwd->rect_image[KLBWND_PASSWORD_btn_clear], p_pt1->x, p_pt1->y))
        {
            klbwnd_password_set_title(p_wnd, "");
            klbwnd_password_on_focusdelay(p_wnd, p_passwd);
            klb_wnd_tip_update(p_wnd);
            klb_wnd_update(p_wnd);
            return 0;
        }
    }

    if (NULL == p_passwd->p_menu)
    {
        return 0;
    }

    klbshw_edit_menu_bind(p_passwd->p_menu, on_menu_popup_end_klbwnd_password, p_wnd);

    int screen_w = 0;
    int screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int menu_w = 0;
    int menu_h = 0;
    klbshw_edit_menu_wh(p_passwd->p_menu, &menu_w, &menu_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_passwd->p_menu, x, y);
    klb_wnd_resize(p_passwd->p_menu, menu_w, menu_h);

    klbshw_edit_menu_set_page_letter(p_passwd->p_menu, KLBSHW_EDIT_MENU_lowercase);
    klbshw_edit_menu_set_hide_letter(p_passwd->p_menu, p_passwd->is_hide_letter);
    klbshw_edit_menu_layout(p_passwd->p_menu);

    const char* p_value = "";
    if (NULL != p_passwd->title)
    {
        p_value = p_passwd->title;
    }
    klbshw_edit_menu_set_value(p_passwd->p_menu, p_value);

    klb_gui_popup_wnd(p_wnd->p_gui, p_passwd->p_menu);

    return 0;
}

static int klbwnd_password_on_focusdelay(klb_wnd_t* p_wnd, klbwnd_password_t* p_passwd)
{
    klbwnd_password_css_t* p_css = p_passwd->p_css;

    if (NULL == p_css || !klb_wnd_is_dyntip(p_wnd))
    {
        return 0;
    }

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;
    klbuiutil_remove_margin(&rect, &p_css->margin);

    int font_size = p_css->normal.font.size;
    if (font_size < p_css->focus.font.size)
    {
        font_size = p_css->focus.font.size;
    }
    if (font_size < p_css->disable.font.size)
    {
        font_size = p_css->disable.font.size;
    }

    sds tip_title = klbwnd_password_paint_title(p_passwd);
    if (klbwnd_password_has_sds(tip_title))
    {
        klbwnd_password_update_tip_dynamic(p_wnd, &rect, tip_title, font_size);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }

    return 0;
}

static int klbwnd_password_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_password_on_paint(p_wnd);
        break;

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_password_on_click(p_wnd, p_passwd, p_pt1);
        break;

    case KLBUI_focusdelay:
        return klbwnd_password_on_focusdelay(p_wnd, p_passwd);
        break;

    default:
        break;
    }

    return 0;
}

static void klbwnd_password_update_tip_dynamic(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, sds title, int font_size)
{
    if (NULL == p_wnd || NULL == p_rect || NULL == title)
    {
        return;
    }

    int txt_w = 0;
    klb_wnd_text_size2(p_wnd, title, (int)sdslen(title), font_size, &txt_w, NULL);

    if (p_rect->w < txt_w)
    {
        klb_wnd_set_tip_dynamic(p_wnd, title);
    }
    else
    {
        klb_wnd_set_tip_dynamic(p_wnd, "");
    }
}

static void klbwnd_password_sync_hide_title(klbwnd_password_t* p_passwd)
{
    int len = 0;

    if (NULL != p_passwd->title)
    {
        len = (int)sdslen(p_passwd->title);
    }

    if (len <= 0)
    {
        KLB_FREE_BY(p_passwd->hide_title, sdsfree);
        return;
    }

    char* p_stars = KLB_MALLOC(char, (size_t)len + 1, 0);
    memset(p_stars, '*', (size_t)len);
    p_stars[len] = '\0';

    p_passwd->hide_title = klb_sds_assign(p_passwd->hide_title, p_stars);
    KLB_FREE(p_stars);
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_password_set_css(klb_wnd_t* p_wnd, klbwnd_password_css_t* p_css)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    p_passwd->p_css = p_css;
}

void klbwnd_password_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    p_passwd->index = index;
}

int klbwnd_password_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    return p_passwd->index;
}

void klbwnd_password_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    p_passwd->title = klb_sds_assign(p_passwd->title, p_title);
    klbwnd_password_sync_hide_title(p_passwd);
}

const sds klbwnd_password_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    return p_passwd->title;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_password_init_attribute(klb_wnd_t* p_wnd, klbwnd_password_t* p_passwd)
{
    klb_wnd_dyntip(p_wnd, true);

    p_passwd->index = 0;
    p_passwd->title = NULL;
    p_passwd->is_hide_letter = true;
    p_passwd->hide_title = NULL;
    p_passwd->p_menu = NULL;
}

static void klbwnd_password_deinit_attribute(klbwnd_password_t* p_passwd)
{
    KLB_FREE_BY(p_passwd->title, sdsfree);
    KLB_FREE_BY(p_passwd->hide_title, sdsfree);
    p_passwd->p_menu = NULL;
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_password_attributes_init(klbwnd_password_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;

    p_attr->stretch_image = NULL;
    p_attr->btn_symbol_image = NULL;
    p_attr->btn_clear_image = NULL;
}

static void klbwnd_password_attributes_deinit(klbwnd_password_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
    KLB_FREE_BY(p_attr->stretch_image, sdsfree);
    KLB_FREE_BY(p_attr->btn_symbol_image, sdsfree);
    KLB_FREE_BY(p_attr->btn_clear_image, sdsfree);
}

static void klbwnd_password_attributes_copy(klbwnd_password_attributes_t* p_dst, const klbwnd_password_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;

    p_dst->stretch_image = klb_sds_assign(p_dst->stretch_image, p_src->stretch_image);
    p_dst->btn_symbol_image = klb_sds_assign(p_dst->btn_symbol_image, p_src->btn_symbol_image);
    p_dst->btn_clear_image = klb_sds_assign(p_dst->btn_clear_image, p_src->btn_clear_image);
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_password_css_init(klbwnd_password_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_password_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_password_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_password_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_left;
    p_css->focus.text.align = KLBUICSS_text1_left;
    p_css->disable.text.align = KLBUICSS_text1_left;

    p_css->btn_visibility = true;
    p_css->btn_w = 20;
    p_css->btn_h = 20;
    p_css->btn_interval = 4;
}

void klbwnd_password_css_deinit(klbwnd_password_css_t* p_css)
{
    klbwnd_password_attributes_deinit(&p_css->normal);
    klbwnd_password_attributes_deinit(&p_css->focus);
    klbwnd_password_attributes_deinit(&p_css->disable);
}

void klbwnd_password_css_copy(klbwnd_password_css_t* p_dst, klbwnd_password_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_password_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_password_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_password_attributes_copy(&p_dst->disable, &p_src->disable);

    p_dst->btn_visibility = p_src->btn_visibility;
    p_dst->btn_w = p_src->btn_w;
    p_dst->btn_h = p_src->btn_h;
    p_dst->btn_interval = p_src->btn_interval;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_password_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_password_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    p_wnd->state.style = KLB_WND_STYLE_FOCUS_DELAY;

    klbwnd_password_init_attribute(p_wnd, p_passwd);

    p_passwd->p_menu = klbui_shwnd_get_edit_menu(p_gui);
}

void klbwnd_password_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_password_t* p_passwd = (klbwnd_password_t*)p_wnd->ctrl;

    klbwnd_password_deinit_attribute(p_passwd);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_password_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_password_t));

    klbwnd_password_init(p_wnd, p_gui, x, y, w, h);

    p_wnd->vtable.destroy = klbwnd_password_destroy;

    return p_wnd;
}

// end
