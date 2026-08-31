// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_decimal_menu_ip.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbwui/core/klbwui_css.h"
#include "klbmem/klb_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_decimal_menu_ip_deinit_attribute(klbwnd_decimal_menu_ip_t* p_menu);
static void klbwnd_decimal_menu_ip_relayout(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu);
static void klbwnd_decimal_menu_ip_sync_title(klbwnd_decimal_menu_ip_t* p_menu);
static void klbwnd_decimal_menu_ip_popup_end(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu, bool ok);
static void normalize_value_klbwnd_decimal_menu_ip(klbwnd_decimal_menu_ip_t* p_menu);


//////////////////////////////////////////////////////////////////////////
// 1080p 按键布局 (5 行 x 4 列)

static klbwnd_decimal_menu_ip_layout_t s_klbwnd_decimal_menu_ip_layout_1080p[KLBWND_DECMENU_IP_row][KLBWND_DECMENU_IP_column] = {
    {
        { { 2, 42, 122, 42 }, "7", KLBWND_DECMENU_IP_KEY_normal },
        { { 126, 42, 122, 42 }, "8", KLBWND_DECMENU_IP_KEY_normal },
        { { 250, 42, 122, 42 }, "9", KLBWND_DECMENU_IP_KEY_normal },
        { { 0, 0, 0, 0 }, "", KLBWND_DECMENU_IP_KEY_none }
    },
    {
        { { 2, 86, 122, 42 }, "4", KLBWND_DECMENU_IP_KEY_normal },
        { { 126, 86, 122, 42 }, "5", KLBWND_DECMENU_IP_KEY_normal },
        { { 250, 86, 122, 42 }, "6", KLBWND_DECMENU_IP_KEY_normal },
        { { 0, 0, 0, 0 }, "", KLBWND_DECMENU_IP_KEY_none }
    },
    {
        { { 2, 130, 122, 42 }, "1", KLBWND_DECMENU_IP_KEY_normal },
        { { 126, 130, 122, 42 }, "2", KLBWND_DECMENU_IP_KEY_normal },
        { { 250, 130, 122, 42 }, "3", KLBWND_DECMENU_IP_KEY_normal },
        { { 0, 0, 0, 0 }, "", KLBWND_DECMENU_IP_KEY_none }
    },
    {
        { { 2, 174, 122, 42 }, "0", KLBWND_DECMENU_IP_KEY_normal },
        { { 126, 174, 60, 42 }, "<", KLBWND_DECMENU_IP_KEY_moveleft },
        { { 188, 174, 60, 42 }, ">", KLBWND_DECMENU_IP_KEY_moveright },
        { { 250, 174, 122, 42 }, "BS", KLBWND_DECMENU_IP_KEY_backspace }
    },
    {
        { { 2, 218, 122, 42 }, ".", KLBWND_DECMENU_IP_KEY_point },
        { { 126, 218, 122, 42 }, "Clear", KLBWND_DECMENU_IP_KEY_clear },
        { { 250, 218, 122, 42 }, "Enter", KLBWND_DECMENU_IP_KEY_enter },
        { { 0, 0, 0, 0 }, "", KLBWND_DECMENU_IP_KEY_none }
    }
};


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_decimal_menu_ip_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_decimal_menu_ip_on_paint_status(klb_wnd_t* p_wnd, klbwnd_decimal_menu_ip_t* p_menu, klb_rect_t* p_rect)
{
    klbwnd_decimal_menu_ip_css_t* p_css = &p_menu->css;
    klbuicssex_attributes_t* p_attr = &p_css->normal;

    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    int text_h = p_css->string_height;
    klb_rect_t text_rect = { p_rect->x, p_rect->y, p_rect->w, text_h };
    klbwuicss_draw_background(p_wnd, &text_rect, &p_css->string_background);

    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }

    int offset = p_css->string_offset;
    text_rect.x += offset;
    text_rect.w -= offset * 2;

    if (p_menu->show_cursor && NULL != p_menu->value)
    {
        int val_len = (int)sdslen(p_menu->value);
        int prefix_len = val_len - p_menu->cursor_pos;
        if (prefix_len < 0)
        {
            prefix_len = 0;
        }
        else if (prefix_len > val_len)
        {
            prefix_len = val_len;
        }

        int need_w = 0;
        klb_wnd_text_size2(p_wnd, p_menu->value, prefix_len, p_attr->font.size, &need_w, NULL);

        int line_x = text_rect.x + need_w;
        int line_y1 = text_rect.y + text_h / 6;
        int line_y2 = text_rect.y + text_h - text_h / 6;
        if (line_x < text_rect.x + text_rect.w)
        {
            klb_wnd_draw_line2(p_wnd, line_x, line_y1, line_x, line_y2, p_menu->cursor_color);
        }
    }

    klbuicssex_draw_text(p_wnd, p_menu->title, &text_rect, &p_attr->border, NULL, &p_attr->text, &p_attr->font);
}

static int klbwnd_decimal_menu_ip_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klb_rect_t paint_rect = *p_rect;
    paint_rect.x += p_menu->css.margin.left;
    paint_rect.y += p_menu->css.margin.top;
    paint_rect.w -= (p_menu->css.margin.left + p_menu->css.margin.right);
    paint_rect.h -= (p_menu->css.margin.top + p_menu->css.margin.bottom);

    klbwnd_decimal_menu_ip_on_paint_status(p_wnd, p_menu, &paint_rect);

    return 0;
}

static int klbwnd_decimal_menu_ip_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_decimal_menu_ip_on_paint(p_wnd);

    case KLBUI_outwindow:
        klbwnd_decimal_menu_ip_popup_end(p_wnd, p_menu, true);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 文本编辑

static int value_int_klbwnd_decimal_menu_ip(klbwnd_decimal_menu_ip_t* p_menu)
{
    if (NULL == p_menu->value)
    {
        return 0;
    }

    return atoi(p_menu->value);
}

static void normalize_value_klbwnd_decimal_menu_ip(klbwnd_decimal_menu_ip_t* p_menu)
{
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", value_int_klbwnd_decimal_menu_ip(p_menu));
    buf[sizeof(buf) - 1] = '\0';
    p_menu->value = klb_sds_assign(p_menu->value, buf);
    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }
}

static void klbwnd_decimal_menu_ip_popup_end(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu, bool ok)
{
    if (NULL != p_menu->cb)
    {
        p_menu->cb(p_menu->ptr, p_wnd_menu, ok, value_int_klbwnd_decimal_menu_ip(p_menu));
    }

    klb_gui_popup_end(p_wnd_menu->p_gui, false);
}

static void klbwnd_decimal_menu_ip_sync_title(klbwnd_decimal_menu_ip_t* p_menu)
{
    const char* p_val = "";

    if (NULL != p_menu->value)
    {
        p_val = p_menu->value;
    }

    p_menu->title = klb_sds_assign(p_menu->title, p_val);
}

static void klbwnd_decimal_menu_ip_append(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu, const char* p_str)
{
    if (NULL == p_str || '\0' == p_str[0])
    {
        return;
    }

    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }

    int val_len = (int)sdslen(p_menu->value);

    if (val_len < p_menu->max_len)
    {
        if (0 == val_len || 0 == p_menu->cursor_pos)
        {
            p_menu->value = sdscat(p_menu->value, p_str);
        }
        else if (p_menu->cursor_pos >= val_len)
        {
            sds old_value = sdsnew(p_menu->value);
            sdsclear(p_menu->value);
            p_menu->value = sdscat(p_menu->value, p_str);
            p_menu->value = sdscatsds(p_menu->value, old_value);
            KLB_FREE_BY(old_value, sdsfree);
        }
        else
        {
            int remain_pos = val_len - p_menu->cursor_pos;
            sds first_part = sdsnew(p_menu->value);
            sds last_part = sdsnew(p_menu->value);

            sdsrange(first_part, 0, remain_pos - 1);
            sdsrange(last_part, remain_pos, -1);

            sdsclear(p_menu->value);
            p_menu->value = sdscatsds(p_menu->value, first_part);
            p_menu->value = sdscat(p_menu->value, p_str);
            p_menu->value = sdscatsds(p_menu->value, last_part);

            KLB_FREE_BY(first_part, sdsfree);
            KLB_FREE_BY(last_part, sdsfree);
        }

        normalize_value_klbwnd_decimal_menu_ip(p_menu);
        klbwnd_decimal_menu_ip_sync_title(p_menu);
        klb_wnd_update(p_wnd_menu);
    }
    else if (0 == value_int_klbwnd_decimal_menu_ip(p_menu))
    {
        p_menu->value = klb_sds_assign(p_menu->value, p_str);
        normalize_value_klbwnd_decimal_menu_ip(p_menu);
        klbwnd_decimal_menu_ip_sync_title(p_menu);
        klb_wnd_update(p_wnd_menu);
    }
}

static void klbwnd_decimal_menu_ip_dec(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu)
{
    int char_num = 0;
    int del_pos = 0;

    if (NULL == p_menu->value)
    {
        return;
    }

    char_num = (int)sdslen(p_menu->value);
    if (char_num <= 0)
    {
        return;
    }

    del_pos = char_num - 1 - p_menu->cursor_pos;
    if (del_pos < 0)
    {
        return;
    }

    sds first_part = sdsnew(p_menu->value);
    sds last_part = sdsnew(p_menu->value);

    if (0 == del_pos)
    {
        sdsrange(last_part, 1, -1);
        sdsclear(p_menu->value);
        p_menu->value = sdscatsds(p_menu->value, last_part);
    }
    else if (del_pos >= char_num - 1)
    {
        sdsrange(first_part, 0, char_num - 2);
        sdsclear(p_menu->value);
        p_menu->value = sdscatsds(p_menu->value, first_part);
    }
    else
    {
        sdsrange(first_part, 0, del_pos - 1);
        sdsrange(last_part, del_pos + 1, -1);
        sdsclear(p_menu->value);
        p_menu->value = sdscatsds(p_menu->value, first_part);
        p_menu->value = sdscatsds(p_menu->value, last_part);
    }

    KLB_FREE_BY(first_part, sdsfree);
    KLB_FREE_BY(last_part, sdsfree);

    klbwnd_decimal_menu_ip_sync_title(p_menu);
    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_decimal_menu_ip_clear(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu)
{
    p_menu->value = klb_sds_assign(p_menu->value, "0");
    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }

    p_menu->cursor_pos = 0;
    klbwnd_decimal_menu_ip_sync_title(p_menu);
    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_decimal_menu_ip_cursor_moveleft(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu)
{
    int max_cursor_pos = 0;
    if (NULL != p_menu->value)
    {
        max_cursor_pos = (int)sdslen(p_menu->value);
    }

    p_menu->cursor_pos += 1;
    if (p_menu->cursor_pos > max_cursor_pos)
    {
        p_menu->cursor_pos = max_cursor_pos;
    }

    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_decimal_menu_ip_cursor_moveright(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu)
{
    if (p_menu->cursor_pos > 0)
    {
        p_menu->cursor_pos -= 1;
    }

    klb_wnd_update(p_wnd_menu);
}


//////////////////////////////////////////////////////////////////////////
// 按键 command

static int on_click_btn_klbwnd_decimal_menu_ip(klb_wnd_t* p_wnd_btn, klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu, int type)
{
    int cur = value_int_klbwnd_decimal_menu_ip(p_menu);

    if (KLBWND_DECMENU_IP_KEY_normal == type)
    {
        const sds value = klbwnd_button_get_value(p_wnd_btn);
        if (NULL != value && 0 < sdslen(value))
        {
            klbwnd_decimal_menu_ip_append(p_wnd_menu, p_menu, value);
        }

        if (NULL != p_menu->cb_click_number)
        {
            p_menu->cb_click_number(p_menu->ptr, p_wnd_menu, true, value_int_klbwnd_decimal_menu_ip(p_menu));
        }
    }
    else if (KLBWND_DECMENU_IP_KEY_point == type)
    {
        if (NULL != p_menu->cb_click_point)
        {
            p_menu->cb_click_point(p_menu->ptr, p_wnd_menu, true, cur);
        }
        else
        {
            klbwnd_decimal_menu_ip_append(p_wnd_menu, p_menu, ".");
        }
    }
    else if (KLBWND_DECMENU_IP_KEY_backspace == type)
    {
        if (NULL != p_menu->cb_click_backspace)
        {
            p_menu->cb_click_backspace(p_menu->ptr, p_wnd_menu, true, cur);
        }
        else
        {
            klbwnd_decimal_menu_ip_dec(p_wnd_menu, p_menu);
        }
    }
    else if (KLBWND_DECMENU_IP_KEY_moveleft == type)
    {
        if (NULL != p_menu->cb_click_moveleft)
        {
            p_menu->cb_click_moveleft(p_menu->ptr, p_wnd_menu, true, cur);
        }
        else
        {
            klbwnd_decimal_menu_ip_cursor_moveleft(p_wnd_menu, p_menu);
        }
    }
    else if (KLBWND_DECMENU_IP_KEY_moveright == type)
    {
        if (NULL != p_menu->cb_click_moveright)
        {
            p_menu->cb_click_moveright(p_menu->ptr, p_wnd_menu, true, cur);
        }
        else
        {
            klbwnd_decimal_menu_ip_cursor_moveright(p_wnd_menu, p_menu);
        }
    }
    else if (KLBWND_DECMENU_IP_KEY_clear == type)
    {
        if (NULL != p_menu->cb_click_clear)
        {
            p_menu->cb_click_clear(p_menu->ptr, p_wnd_menu, true, cur);
        }
        else
        {
            klbwnd_decimal_menu_ip_clear(p_wnd_menu, p_menu);
        }
    }

    return 0;
}

static int on_command_btn_klbwnd_decimal_menu_ip(klb_wnd_t* p_wnd_btn, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)wparam;

    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd_btn->p_udata;
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd_menu->ctrl;
    int type = klbwnd_button_get_index(p_wnd_btn);

    if (KLBUI_mousedown == msg)
    {
        if (KLBUI_MOUSE_left == lparam)
        {
            if (KLBWND_DECMENU_IP_KEY_enter != type)
            {
                return on_click_btn_klbwnd_decimal_menu_ip(p_wnd_btn, p_wnd_menu, p_menu, type);
            }
        }
    }
    else if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        if (KLBWND_DECMENU_IP_KEY_enter == type)
        {
            klbwnd_decimal_menu_ip_popup_end(p_wnd_menu, p_menu, true);
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 子键布局

static void klbwnd_decimal_menu_ip_relayout(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu)
{
    for (int m = 0; m < KLBWND_DECMENU_IP_row; ++m)
    {
        for (int n = 0; n < KLBWND_DECMENU_IP_column; ++n)
        {
            klb_wnd_t* p_wnd_btn = p_menu->p_wnds[m][n];
            klb_rect_t rect = p_menu->layout[m][n].rect;

            if (0 < rect.w && 0 < rect.h)
            {
                const char* p_title = p_menu->layout[m][n].p_num;

                klb_wnd_move(p_wnd_btn, rect.x, rect.y);
                klb_wnd_resize(p_wnd_btn, rect.w, rect.h);

                klbwnd_button_set_index(p_wnd_btn, p_menu->layout[m][n].type);
                klbwnd_button_set_css(p_wnd_btn, &p_menu->css.btn_css);

                klbwnd_button_set_title(p_wnd_btn, p_title);
                klbwnd_button_set_value(p_wnd_btn, p_title);

                klb_wnd_enable(p_wnd_btn, true);
                klb_wnd_show(p_wnd_btn, true);
            }
            else
            {
                klb_wnd_show(p_wnd_btn, false);
            }
        }
    }

    klb_wnd_update(p_wnd_menu);
}

static int klbwnd_decimal_menu_ip_init_subwnd(klb_wnd_t* p_wnd_menu, klbwnd_decimal_menu_ip_t* p_menu, klb_gui_t* p_gui)
{
    memcpy(p_menu->layout, s_klbwnd_decimal_menu_ip_layout_1080p, sizeof(s_klbwnd_decimal_menu_ip_layout_1080p));

    for (int m = 0; m < KLBWND_DECMENU_IP_row; ++m)
    {
        for (int n = 0; n < KLBWND_DECMENU_IP_column; ++n)
        {
            klb_wnd_t* p_wnd_btn = klbwnd_button_create(p_gui, 0, 0, 32, 32);

            klb_wnd_bind_command(p_wnd_btn, on_command_btn_klbwnd_decimal_menu_ip, p_wnd_menu);
            klb_wnd_show(p_wnd_btn, false);
            klb_wnd_push_child(p_wnd_menu, p_wnd_btn);

            p_menu->p_wnds[m][n] = p_wnd_btn;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_decimal_menu_ip_init_attribute(klb_wnd_t* p_wnd, klbwnd_decimal_menu_ip_t* p_menu)
{
    (void)p_wnd;

    p_menu->title = sdsempty();
    p_menu->value = sdsnew("0");
    p_menu->max_len = 6;

    p_menu->cursor_pos = 0;
    p_menu->show_cursor = true;
    p_menu->cursor_color = KLB_ARGB8888(255, 128, 255, 0);

    klbwnd_decimal_menu_ip_sync_title(p_menu);
}

static void klbwnd_decimal_menu_ip_deinit_attribute(klbwnd_decimal_menu_ip_t* p_menu)
{
    KLB_FREE_BY(p_menu->title, sdsfree);
    KLB_FREE_BY(p_menu->value, sdsfree);
}

static void klbwnd_decimal_menu_ip_fg_deinit(klbwnd_decimal_menu_ip_fg_t* p_fg)
{
    KLB_FREE_BY(p_fg->move_left_image, sdsfree);
    KLB_FREE_BY(p_fg->move_right_image, sdsfree);
    KLB_FREE_BY(p_fg->backspace_image, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_decimal_menu_ip_css_init(klbwnd_decimal_menu_ip_t* p_menu, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_menu->css.margin = p_default->margin;
    p_menu->css.padding = p_default->padding;

    klbuicssex_attributes_init(&p_menu->css.normal, &p_default->normal);
    p_menu->css.normal.background.color = KLB_ARGB8888(255, 48, 48, 48);

    p_menu->css.string_background.image = NULL;
    p_menu->css.string_background.color = p_default->normal.background.color;
    p_menu->css.string_background.image = klb_sds_assign(p_menu->css.string_background.image, p_default->normal.background.image);
    p_menu->css.string_background.image_mode = p_default->normal.background.image_mode;
    p_menu->css.string_background.image_flags = p_default->normal.background.image_flags;

    p_menu->css.normal.border.color.top = KLB_ARGB8888(255, 81, 81, 81);
    p_menu->css.normal.border.color.right = KLB_ARGB8888(255, 81, 81, 81);
    p_menu->css.normal.border.color.bottom = KLB_ARGB8888(255, 81, 81, 81);
    p_menu->css.normal.border.color.left = KLB_ARGB8888(255, 81, 81, 81);

    klbwnd_button_css_init(&p_menu->css.btn_css, p_gui);

    p_menu->css.string_height = 42;
    p_menu->css.string_offset = 8;
}

void klbwnd_decimal_menu_ip_css_deinit(klbwnd_decimal_menu_ip_t* p_menu)
{
    klbuicssex_attributes_quit(&p_menu->css.normal);
    KLB_FREE_BY(p_menu->css.string_background.image, sdsfree);
    klbwnd_button_css_deinit(&p_menu->css.btn_css);

    KLB_FREE_BY(p_menu->css.btn_attr.normal_stretch_image, sdsfree);
    KLB_FREE_BY(p_menu->css.btn_attr.focus_stretch_image, sdsfree);
    KLB_FREE_BY(p_menu->css.btn_attr.disable_stretch_image, sdsfree);
    KLB_FREE_BY(p_menu->css.btn_attr.check_normal_stretch_image, sdsfree);
    KLB_FREE_BY(p_menu->css.btn_attr.check_focus_stretch_image, sdsfree);
    KLB_FREE_BY(p_menu->css.btn_attr.check_disable_stretch_image, sdsfree);

    klbwnd_decimal_menu_ip_fg_deinit(&p_menu->css.btn_attr.normal_foreground);
    klbwnd_decimal_menu_ip_fg_deinit(&p_menu->css.btn_attr.focus_foreground);
    klbwnd_decimal_menu_ip_fg_deinit(&p_menu->css.btn_attr.disable_foreground);
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_decimal_menu_ip_set_css(klb_wnd_t* p_wnd, klbwnd_decimal_menu_ip_t* p_menu)
{
    klbwnd_decimal_menu_ip_t* p_self = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    if (NULL != p_menu)
    {
        for (int m = 0; m < KLBWND_DECMENU_IP_row; ++m)
        {
            for (int n = 0; n < KLBWND_DECMENU_IP_column; ++n)
            {
                klbwnd_button_set_css(p_menu->p_wnds[m][n], &p_self->css.btn_css);
            }
        }
    }
}

void klbwnd_decimal_menu_ip_set_ranges(klb_wnd_t* p_wnd, int max_len)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    p_menu->max_len = max_len;
}

void klbwnd_decimal_menu_ip_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", value);
    buf[sizeof(buf) - 1] = '\0';
    p_menu->value = klb_sds_assign(p_menu->value, buf);
    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }

    p_menu->cursor_pos = 0;
    klbwnd_decimal_menu_ip_sync_title(p_menu);
}

int klbwnd_decimal_menu_ip_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    return value_int_klbwnd_decimal_menu_ip(p_menu);
}

int klbwnd_decimal_menu_ip_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h)
{
    (void)p_wnd;

    if (NULL != p_out_w)
    {
        *p_out_w = 374;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = 262;
    }

    return 0;
}

int klbwnd_decimal_menu_ip_bind(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    p_menu->cb = cb;
    p_menu->ptr = ptr;

    return 0;
}

static int bind_extra_klbwnd_decimal_menu_ip(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb* p_slot, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    *p_slot = cb;
    p_menu->ptr = ptr;
    return 0;
}

int klbwnd_decimal_menu_ip_bind_click_point(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_point, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_click_number(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_number, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_click_moveleft(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_moveleft, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_click_moveright(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_moveright, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_click_backspace(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_backspace, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_click_clear(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_click_clear, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_get_cursor_pos(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_get_cursor_pos, cb, ptr);
}

int klbwnd_decimal_menu_ip_bind_set_show_cursor(klb_wnd_t* p_wnd, klbshw_decimal_menu_ip_cb cb, void* ptr)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;
    return bind_extra_klbwnd_decimal_menu_ip(p_wnd, &p_menu->cb_set_show_cursor, cb, ptr);
}

int klbwnd_decimal_menu_ip_layout(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    memcpy(p_menu->layout, s_klbwnd_decimal_menu_ip_layout_1080p, sizeof(s_klbwnd_decimal_menu_ip_layout_1080p));
    p_menu->css.string_height = 42;
    p_menu->css.string_offset = 8;

    klbwnd_decimal_menu_ip_relayout(p_wnd, p_menu);

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

void klbwnd_decimal_menu_ip_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_decimal_menu_ip_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    klbwnd_decimal_menu_ip_init_attribute(p_wnd, p_menu);
    klbwnd_decimal_menu_ip_init_subwnd(p_wnd, p_menu, p_gui);
}

void klbwnd_decimal_menu_ip_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_decimal_menu_ip_t* p_menu = (klbwnd_decimal_menu_ip_t*)p_wnd->ctrl;

    klbwnd_decimal_menu_ip_deinit_attribute(p_menu);
}

klb_wnd_t* klbwnd_decimal_menu_ip_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_decimal_menu_ip_t));

    klbwnd_decimal_menu_ip_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_decimal_menu_ip_destroy;

    return p_wnd;
}

// end
