// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_shwnd/klbwnd_edit_menu.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbwui/core/klbwui_css.h"
#include "klbmem/klb_mem.h"
#include <string.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_edit_menu_deinit_attribute(klbwnd_edit_menu_t* p_menu);
static void klbwnd_edit_menu_relayout(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu);
static void klbwnd_edit_menu_sync_title(klbwnd_edit_menu_t* p_menu);
static void klbwnd_edit_menu_popup_end(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu, bool ok);


//////////////////////////////////////////////////////////////////////////
// 1080p 按键布局

static klbwnd_edit_menu_layout_t s_klbwnd_edit_menu_layout_1080p[KLBWND_EDITMENU_row][KLBWND_EDITMENU_column] = {
    {
        { { 2, 42, 60, 42 }, "1", "1", "!", KLBWND_EDITMENU_KEY_normal },
        { { 64, 42, 60, 42 }, "2", "2", "@", KLBWND_EDITMENU_KEY_normal },
        { { 126, 42, 60, 42 }, "3", "3", "#", KLBWND_EDITMENU_KEY_normal },
        { { 188, 42, 60, 42 }, "4", "4", "$", KLBWND_EDITMENU_KEY_normal },
        { { 250, 42, 60, 42 }, "5", "5", "%", KLBWND_EDITMENU_KEY_normal },
        { { 312, 42, 60, 42 }, "6", "6", "^", KLBWND_EDITMENU_KEY_normal },
        { { 374, 42, 60, 42 }, "7", "7", "&", KLBWND_EDITMENU_KEY_normal },
        { { 436, 42, 60, 42 }, "8", "8", "*", KLBWND_EDITMENU_KEY_normal },
        { { 498, 42, 60, 42 }, "9", "9", "(", KLBWND_EDITMENU_KEY_normal },
        { { 560, 42, 60, 42 }, "0", "0", ")", KLBWND_EDITMENU_KEY_normal }
    },
    {
        { { 2, 86, 60, 42 }, "q", "Q", "-", KLBWND_EDITMENU_KEY_normal },
        { { 64, 86, 60, 42 }, "w", "W", "=", KLBWND_EDITMENU_KEY_normal },
        { { 126, 86, 60, 42 }, "e", "E", "_", KLBWND_EDITMENU_KEY_normal },
        { { 188, 86, 60, 42 }, "r", "R", "+", KLBWND_EDITMENU_KEY_normal },
        { { 250, 86, 60, 42 }, "t", "T", "[", KLBWND_EDITMENU_KEY_normal },
        { { 312, 86, 60, 42 }, "y", "Y", "]", KLBWND_EDITMENU_KEY_normal },
        { { 374, 86, 60, 42 }, "u", "U", "{", KLBWND_EDITMENU_KEY_normal },
        { { 436, 86, 60, 42 }, "i", "I", "}", KLBWND_EDITMENU_KEY_normal },
        { { 498, 86, 60, 42 }, "o", "O", ";", KLBWND_EDITMENU_KEY_normal },
        { { 560, 86, 60, 42 }, "p", "P", ":", KLBWND_EDITMENU_KEY_normal }
    },
    {
        { { 2, 130, 60, 42 }, "Caps", "Caps", "Caps", KLBWND_EDITMENU_KEY_capslock },
        { { 64, 130, 60, 42 }, "a", "A", "'", KLBWND_EDITMENU_KEY_normal },
        { { 126, 130, 60, 42 }, "s", "S", "\"", KLBWND_EDITMENU_KEY_normal },
        { { 188, 130, 60, 42 }, "d", "D", ",", KLBWND_EDITMENU_KEY_normal },
        { { 250, 130, 60, 42 }, "f", "F", ".", KLBWND_EDITMENU_KEY_normal },
        { { 312, 130, 60, 42 }, "g", "G", "<", KLBWND_EDITMENU_KEY_normal },
        { { 374, 130, 60, 42 }, "h", "H", ">", KLBWND_EDITMENU_KEY_normal },
        { { 436, 130, 60, 42 }, "j", "J", "/", KLBWND_EDITMENU_KEY_normal },
        { { 498, 130, 60, 42 }, "k", "K", "?", KLBWND_EDITMENU_KEY_normal },
        { { 560, 130, 60, 42 }, "l", "L", "\\", KLBWND_EDITMENU_KEY_normal }
    },
    {
        { { 64, 174, 60, 42 }, "z", "Z", "|", KLBWND_EDITMENU_KEY_normal },
        { { 126, 174, 60, 42 }, "x", "X", "..", KLBWND_EDITMENU_KEY_normal },
        { { 188, 174, 60, 42 }, "c", "C", "...", KLBWND_EDITMENU_KEY_normal },
        { { 250, 174, 60, 42 }, "v", "V", "<<", KLBWND_EDITMENU_KEY_normal },
        { { 312, 174, 60, 42 }, "b", "B", ">>", KLBWND_EDITMENU_KEY_normal },
        { { 374, 174, 60, 42 }, "n", "N", "`", KLBWND_EDITMENU_KEY_normal },
        { { 436, 174, 60, 42 }, "m", "M", "~", KLBWND_EDITMENU_KEY_normal },
        { { 2, 174, 60, 42 }, "Shift", "Shift", "Shift", KLBWND_EDITMENU_KEY_symbol },
        { { 498, 174, 122, 42 }, "BS", "BS", "BS", KLBWND_EDITMENU_KEY_backspace },
        { { 0, 0, 0, 0 }, "", "", "", KLBWND_EDITMENU_KEY_none }
    },
    {
        { { 2, 218, 60, 42 }, "GL", "GL", "GL", KLBWND_EDITMENU_KEY_global },
        { { 64, 218, 184, 42 }, "Space", "Space", "Space", KLBWND_EDITMENU_KEY_space },
        { { 250, 218, 122, 42 }, "Clear", "Clear", "Clear", KLBWND_EDITMENU_KEY_clear },
        { { 374, 218, 60, 42 }, "<", "<", "<", KLBWND_EDITMENU_KEY_moveleft },
        { { 436, 218, 60, 42 }, ">", ">", ">", KLBWND_EDITMENU_KEY_moveright },
        { { 498, 218, 122, 42 }, "Enter", "Enter", "Enter", KLBWND_EDITMENU_KEY_enter },
        { { 0, 0, 0, 0 }, "", "", "", KLBWND_EDITMENU_KEY_none },
        { { 0, 0, 0, 0 }, "", "", "", KLBWND_EDITMENU_KEY_none },
        { { 0, 0, 0, 0 }, "", "", "", KLBWND_EDITMENU_KEY_none },
        { { 0, 0, 0, 0 }, "", "", "", KLBWND_EDITMENU_KEY_none }
    }
};


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_edit_menu_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_edit_menu_on_paint_status(klb_wnd_t* p_wnd, klbwnd_edit_menu_t* p_menu, klb_rect_t* p_rect)
{
    klbwnd_edit_menu_css_t* p_css = &p_menu->css;
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

    sds p_show = p_menu->title;
    if (p_menu->is_hide_letter)
    {
        p_show = p_menu->hide_title;
    }

    if (p_menu->show_cursor && !p_menu->is_hide_letter && NULL != p_menu->value)
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

    klbuicssex_draw_text(p_wnd, p_show, &text_rect, &p_attr->border, NULL, &p_attr->text, &p_attr->font);
}

static int klbwnd_edit_menu_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;
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

    klbwnd_edit_menu_on_paint_status(p_wnd, p_menu, &paint_rect);

    return 0;
}

static int klbwnd_edit_menu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)lparam;
    (void)wparam;

    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_edit_menu_on_paint(p_wnd);

    case KLBUI_outwindow:
        klbwnd_edit_menu_popup_end(p_wnd, p_menu, true);
        break;

    default:
        break;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 文本编辑

static void klbwnd_edit_menu_popup_end(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu, bool ok)
{
    if (NULL != p_menu->cb)
    {
        p_menu->cb(p_menu->ptr, p_wnd_menu, ok, p_menu->value);
    }

    klb_gui_popup_end(p_wnd_menu->p_gui, false);
}

static void klbwnd_edit_menu_sync_title(klbwnd_edit_menu_t* p_menu)
{
    const char* p_val = "";
    int len = 0;

    if (NULL != p_menu->value)
    {
        p_val = p_menu->value;
        len = (int)sdslen(p_menu->value);
    }

    p_menu->title = klb_sds_assign(p_menu->title, p_val);

    if (len <= 0)
    {
        p_menu->hide_title = klb_sds_assign(p_menu->hide_title, "");
        return;
    }

    if (NULL == p_menu->hide_title)
    {
        p_menu->hide_title = sdsempty();
    }

    sdsclear(p_menu->hide_title);
    for (int i = 0; i < len; ++i)
    {
        p_menu->hide_title = sdscat(p_menu->hide_title, "*");
    }
}

static void klbwnd_edit_menu_append(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu, const char* p_str)
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

    klbwnd_edit_menu_sync_title(p_menu);
    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_edit_menu_dec(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu)
{
    if (NULL == p_menu->value)
    {
        return;
    }

    int char_num = (int)sdslen(p_menu->value);
    if (char_num <= 0)
    {
        return;
    }

    int del_pos = char_num - 1 - p_menu->cursor_pos;
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

    klbwnd_edit_menu_sync_title(p_menu);
    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_edit_menu_clear(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu)
{
    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }

    sdsclear(p_menu->value);
    p_menu->cursor_pos = 0;
    klbwnd_edit_menu_sync_title(p_menu);
    klb_wnd_update(p_wnd_menu);
}

static void klbwnd_edit_menu_cursor_moveleft(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu)
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

static void klbwnd_edit_menu_cursor_moveright(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu)
{
    if (p_menu->cursor_pos > 0)
    {
        p_menu->cursor_pos -= 1;
    }

    klb_wnd_update(p_wnd_menu);
}


//////////////////////////////////////////////////////////////////////////
// 按键 command

static int on_click_btn_klbwnd_edit_menu(klb_wnd_t* p_wnd_btn, klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu, int type)
{
    if (KLBWND_EDITMENU_KEY_normal == type)
    {
        const sds value = klbwnd_button_get_value(p_wnd_btn);
        if (NULL != value && 0 < sdslen(value))
        {
            klbwnd_edit_menu_append(p_wnd_menu, p_menu, value);
        }
    }
    else if (KLBWND_EDITMENU_KEY_backspace == type)
    {
        klbwnd_edit_menu_dec(p_wnd_menu, p_menu);
    }
    else if (KLBWND_EDITMENU_KEY_capslock == type)
    {
        if (KLBSHW_EDIT_MENU_lowercase == p_menu->page)
        {
            p_menu->page = KLBSHW_EDIT_MENU_uppercase;
        }
        else
        {
            p_menu->page = KLBSHW_EDIT_MENU_lowercase;
        }

        klbwnd_edit_menu_relayout(p_wnd_menu, p_menu);
    }
    else if (KLBWND_EDITMENU_KEY_symbol == type)
    {
        if (KLBSHW_EDIT_MENU_symbol == p_menu->page)
        {
            p_menu->page = KLBSHW_EDIT_MENU_lowercase;
        }
        else
        {
            p_menu->page = KLBSHW_EDIT_MENU_symbol;
        }

        klbwnd_edit_menu_relayout(p_wnd_menu, p_menu);
    }
    else if (KLBWND_EDITMENU_KEY_space == type)
    {
        klbwnd_edit_menu_append(p_wnd_menu, p_menu, " ");
    }
    else if (KLBWND_EDITMENU_KEY_moveleft == type)
    {
        klbwnd_edit_menu_cursor_moveleft(p_wnd_menu, p_menu);
    }
    else if (KLBWND_EDITMENU_KEY_moveright == type)
    {
        klbwnd_edit_menu_cursor_moveright(p_wnd_menu, p_menu);
    }
    else if (KLBWND_EDITMENU_KEY_clear == type)
    {
        klbwnd_edit_menu_clear(p_wnd_menu, p_menu);
    }

    return 0;
}

static int on_command_btn_klbwnd_edit_menu(klb_wnd_t* p_wnd_btn, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    (void)p_pt1;
    (void)p_pt2;
    (void)wparam;

    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd_btn->p_udata;
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd_menu->ctrl;
    int type = klbwnd_button_get_index(p_wnd_btn);

    if (KLBUI_mousedown == msg)
    {
        if (KLBUI_MOUSE_left == lparam)
        {
            if (KLBWND_EDITMENU_KEY_enter != type)
            {
                return on_click_btn_klbwnd_edit_menu(p_wnd_btn, p_wnd_menu, p_menu, type);
            }
        }
    }
    else if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        if (KLBWND_EDITMENU_KEY_enter == type)
        {
            klbwnd_edit_menu_popup_end(p_wnd_menu, p_menu, true);
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 子键布局

static const char* page_title_klbwnd_edit_menu(klbwnd_edit_menu_t* p_menu, int m, int n)
{
    if (KLBSHW_EDIT_MENU_uppercase == p_menu->page)
    {
        return p_menu->layout[m][n].p_uppercase;
    }
    else if (KLBSHW_EDIT_MENU_symbol == p_menu->page)
    {
        return p_menu->layout[m][n].p_symbol;
    }

    return p_menu->layout[m][n].p_lowercase;
}

static void klbwnd_edit_menu_relayout(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu)
{
    for (int m = 0; m < KLBWND_EDITMENU_row; ++m)
    {
        for (int n = 0; n < KLBWND_EDITMENU_column; ++n)
        {
            klb_wnd_t* p_wnd_btn = p_menu->p_wnds[m][n];
            klb_rect_t rect = p_menu->layout[m][n].rect;

            if (0 < rect.w && 0 < rect.h)
            {
                klb_wnd_move(p_wnd_btn, rect.x, rect.y);
                klb_wnd_resize(p_wnd_btn, rect.w, rect.h);

                klbwnd_button_set_index(p_wnd_btn, p_menu->layout[m][n].type);
                klbwnd_button_set_css(p_wnd_btn, &p_menu->css.btn_css);

                const char* p_title = page_title_klbwnd_edit_menu(p_menu, m, n);
                klbwnd_button_set_title(p_wnd_btn, p_title);
                klbwnd_button_set_value(p_wnd_btn, p_title);

                int type = p_menu->layout[m][n].type;
                if (KLBWND_EDITMENU_KEY_moveleft == type || KLBWND_EDITMENU_KEY_moveright == type)
                {
                    klb_wnd_enable(p_wnd_btn, !p_menu->is_hide_letter);
                }
                else if (KLBWND_EDITMENU_KEY_global == type)
                {
                    klb_wnd_enable(p_wnd_btn, false);
                }
                else
                {
                    klb_wnd_enable(p_wnd_btn, true);
                }

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

static int klbwnd_edit_menu_init_subwnd(klb_wnd_t* p_wnd_menu, klbwnd_edit_menu_t* p_menu, klb_gui_t* p_gui)
{
    memcpy(p_menu->layout, s_klbwnd_edit_menu_layout_1080p, sizeof(s_klbwnd_edit_menu_layout_1080p));

    for (int m = 0; m < KLBWND_EDITMENU_row; ++m)
    {
        for (int n = 0; n < KLBWND_EDITMENU_column; ++n)
        {
            klb_wnd_t* p_wnd_btn = klbwnd_button_create(p_gui, 0, 0, 32, 32);

            klb_wnd_bind_command(p_wnd_btn, on_command_btn_klbwnd_edit_menu, p_wnd_menu);
            klb_wnd_show(p_wnd_btn, false);
            klb_wnd_push_child(p_wnd_menu, p_wnd_btn);

            p_menu->p_wnds[m][n] = p_wnd_btn;
        }
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_edit_menu_init_attribute(klb_wnd_t* p_wnd, klbwnd_edit_menu_t* p_menu)
{
    (void)p_wnd;

    p_menu->title = sdsempty();
    p_menu->value = sdsempty();
    p_menu->page = KLBSHW_EDIT_MENU_lowercase;

    p_menu->is_hide_letter = false;
    p_menu->hide_title = sdsempty();

    p_menu->cursor_pos = 0;
    p_menu->show_cursor = true;
    p_menu->cursor_color = KLB_ARGB8888(255, 128, 255, 0);

    p_menu->temp_str = sdsempty();
}

static void klbwnd_edit_menu_deinit_attribute(klbwnd_edit_menu_t* p_menu)
{
    KLB_FREE_BY(p_menu->title, sdsfree);
    KLB_FREE_BY(p_menu->value, sdsfree);
    KLB_FREE_BY(p_menu->hide_title, sdsfree);
    KLB_FREE_BY(p_menu->temp_str, sdsfree);
}

static void klbwnd_edit_menu_fg_deinit(klbwnd_edit_menu_fg_t* p_fg)
{
    KLB_FREE_BY(p_fg->move_left_image, sdsfree);
    KLB_FREE_BY(p_fg->move_right_image, sdsfree);
    KLB_FREE_BY(p_fg->backspace_image, sdsfree);
    KLB_FREE_BY(p_fg->global_image, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_edit_menu_css_init(klbwnd_edit_menu_t* p_menu, klb_gui_t* p_gui)
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

void klbwnd_edit_menu_css_deinit(klbwnd_edit_menu_t* p_menu)
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

    klbwnd_edit_menu_fg_deinit(&p_menu->css.btn_attr.normal_foreground);
    klbwnd_edit_menu_fg_deinit(&p_menu->css.btn_attr.focus_foreground);
    klbwnd_edit_menu_fg_deinit(&p_menu->css.btn_attr.disable_foreground);
}


//////////////////////////////////////////////////////////////////////////
// export

void klbwnd_edit_menu_set_css(klb_wnd_t* p_wnd, klbwnd_edit_menu_t* p_menu)
{
    klbwnd_edit_menu_t* p_self = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    if (NULL != p_menu)
    {
        for (int m = 0; m < KLBWND_EDITMENU_row; ++m)
        {
            for (int n = 0; n < KLBWND_EDITMENU_column; ++n)
            {
                klbwnd_button_set_css(p_menu->p_wnds[m][n], &p_self->css.btn_css);
            }
        }
    }
}

void klbwnd_edit_menu_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    p_menu->value = klb_sds_assign(p_menu->value, p_value);
    if (NULL == p_menu->value)
    {
        p_menu->value = sdsempty();
    }

    p_menu->cursor_pos = 0;
    klbwnd_edit_menu_sync_title(p_menu);
}

const sds klbwnd_edit_menu_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    return p_menu->value;
}

int klbwnd_edit_menu_wh(klb_wnd_t* p_wnd, int* p_out_w, int* p_out_h)
{
    (void)p_wnd;

    if (NULL != p_out_w)
    {
        *p_out_w = 622;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = 262;
    }

    return 0;
}

int klbwnd_edit_menu_bind(klb_wnd_t* p_wnd, klbshw_edit_menu_cb cb, void* ptr)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    p_menu->cb = cb;
    p_menu->ptr = ptr;

    return 0;
}

int klbwnd_edit_menu_layout(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    memcpy(p_menu->layout, s_klbwnd_edit_menu_layout_1080p, sizeof(s_klbwnd_edit_menu_layout_1080p));
    p_menu->css.string_height = 42;
    p_menu->css.string_offset = 8;

    klbwnd_edit_menu_relayout(p_wnd, p_menu);

    return 0;
}

int klbwnd_edit_menu_set_hide_letter(klb_wnd_t* p_wnd, bool is_hide_letter)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    p_menu->is_hide_letter = is_hide_letter;

    return 0;
}

int klbwnd_edit_menu_set_page_letter(klb_wnd_t* p_wnd, int page)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    if (KLBSHW_EDIT_MENU_uppercase == page)
    {
        p_menu->page = KLBSHW_EDIT_MENU_uppercase;
    }
    else if (KLBSHW_EDIT_MENU_symbol == page)
    {
        p_menu->page = KLBSHW_EDIT_MENU_symbol;
    }
    else
    {
        p_menu->page = KLBSHW_EDIT_MENU_lowercase;
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit / create

void klbwnd_edit_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_edit_menu_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    klbwnd_edit_menu_init_attribute(p_wnd, p_menu);
    klbwnd_edit_menu_init_subwnd(p_wnd, p_menu, p_gui);
}

void klbwnd_edit_menu_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_edit_menu_t* p_menu = (klbwnd_edit_menu_t*)p_wnd->ctrl;

    klbwnd_edit_menu_deinit_attribute(p_menu);
}

klb_wnd_t* klbwnd_edit_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_edit_menu_t));

    klbwnd_edit_menu_init(p_wnd, p_gui, x, y, w, h);
    p_wnd->vtable.destroy = klbwnd_edit_menu_destroy;

    return p_wnd;
}

// end
