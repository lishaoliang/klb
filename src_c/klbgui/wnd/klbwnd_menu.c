// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_menu.h"
#include "klbgui/subviews/klbwnd_menu_item.h"
#include "klbgui/subviews/klbwnd_menu_2rd.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_menu_quit_attribute(klbwnd_menu_t* p_menu);
static void klbwnd_menu_relayout(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_menu_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    klbwnd_menu_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_menu_on_paint_status(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu, klbwnd_menu_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
}

static int klbwnd_menu_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_menu_css_t* p_css = p_menu->p_css;

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

    {
        klbwnd_menu_on_paint_status(p_wnd, p_menu, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int klbwnd_menu_suggestw(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu)
{
    int item_h = 32;
    int font_h = (NULL != p_menu->p_css) ? p_menu->p_css->normal.font.size : 24;
    int w = 0;

    int array_size = klb_map_array_size(&p_menu->data);
    for (int i = 0; i < array_size; i++)
    {
        klb_map_t* p_item_map = klb_map_idx_to_map(&p_menu->data, i);
        klb_map_iter_t* p_iter = klb_map_begin(p_item_map);

        const char* p_title = klb_adt_to_string(klb_map_data(p_iter));

        int txt_w = 0;
        klb_wnd_text_size2(p_wnd, p_title, strlen(p_title), font_h, &txt_w, NULL);

        if (w < txt_w)
        {
            w = txt_w;
        }
    }

    w += item_h + 4;

    w = MAX(w, KLBWND_MENU_WIDTH_min);
    w = MIN(w, KLBWND_MENU_WIDTH_max);

    return w;
}

static int klbwnd_menu_suggesth(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu)
{
    int item_h = 32;
    int array_size = klb_map_array_size(&p_menu->data);
    if (0 < array_size)
    {
        return item_h * array_size + 2;
    }

    return 0;
}

static int klbwnd_menu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onload:
        {
            p_menu->is_popup_2rd = false;
        }
        break;

    case KLBUI_onpaint:
        return klbwnd_menu_on_paint(p_wnd);
        break;

    case KLBUI_onpredraw:
        klbwnd_menu_relayout(p_wnd);
        break;

    case KLBUI_outwindow:
        klb_gui_popup_end(p_wnd->p_gui, true);
        break;

    case KLBUI_suggestw:
        return klbwnd_menu_suggestw(p_wnd, p_menu);
        break;

    case KLBUI_suggesth:
        return klbwnd_menu_suggesth(p_wnd, p_menu);
        break;

    case KLBUI_meminfo:
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 

static void popup_end_2rd_klbwnd_menu(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu)
{
    if (p_menu->is_popup_2rd)
    {
        klb_gui_popup_end(p_wnd->p_gui, false);

        p_menu->is_popup_2rd = false;
    }
}

static void popup_2rd_klbwnd_menu(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu, const klb_rect_t* p_rect, const char* p_key)
{
    if (p_menu->is_popup_2rd)
    {
        if (0 == strcmp(p_menu->key_2rd, p_key))
        {
            return; // 已经弹出相同key的子菜单
        }

        popup_end_2rd_klbwnd_menu(p_wnd, p_menu);
    }

    // 弹出菜单
    klb_wnd_t* p_2rd = p_menu->p_menu_2rd;
    klb_map_t* p_array = klb_map_to_map(&p_menu->data_2rd, p_key);
    klbwnd_menu_set_data_ref(p_2rd, p_array);

    int menu_w = klb_wnd_suggestw(p_2rd);
    int menu_h = klb_wnd_suggesth(p_2rd);

    int screen_w = 0, screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    int x = 0;
    int y = p_rect->y;

    if (p_rect->x + p_rect->w + menu_w <= screen_w)
    {
        x = p_rect->x + p_rect->w;  // 在右侧显示
    }
    else
    {
        x = p_rect->x - menu_w; // 在左侧显示
    }

    if (x < 0) { x = 0; };
    if (screen_h < y + menu_h) { y = screen_h - menu_h; };

    klb_wnd_resize(p_2rd, menu_w, menu_h);
    klb_wnd_move(p_2rd, x, y);

    klb_gui_popup_wnd(p_wnd->p_gui, p_2rd);

    // 更新记录
    p_menu->is_popup_2rd = true;
    p_menu->key_2rd = sdscpy(p_menu->key_2rd, p_key);
}

// 本菜单项事件响应
static int on_command_item_klbwnd_menu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd_menu->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        // 左键点击, 没有子菜单的选项, 则为选择值
        if (!klbwnd_menu_item_has_2rd(p_wnd))
        {
            // 保存值
            p_menu->value = sdscpy(p_menu->value, klbwnd_menu_item_get_value(p_wnd));

            // 关闭所有弹窗
            klb_gui_popup_end(p_wnd_menu->p_gui, true);

            // 响应
            klb_wnd_on_command(p_wnd_menu, KLBUI_onchange, NULL, NULL, 0, 0);
        }
    }
    else if(KLBUI_focus == msg)
    {
        if (klbwnd_menu_item_has_2rd(p_wnd))
        {
            // 有二级菜单, 则弹出
            popup_2rd_klbwnd_menu(p_wnd_menu, p_menu, &(p_wnd->pos.rect_in_canvas), klbwnd_menu_item_get_value(p_wnd));
        }
        else
        {
            // 无二级菜单, 则关闭二级菜单
            popup_end_2rd_klbwnd_menu(p_wnd_menu, p_menu);
        }
    }

    return 0;
}


// 二级菜单事件响应
static int on_command_2rd_klbwnd_menu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd_menu->ctrl;

    if (KLBUI_onchange == msg)
    {
        // 保存值
        p_menu->value = sdscpy(p_menu->value, klbwnd_menu_2rd_get_value(p_wnd));

        // 关闭所有弹窗
        klb_gui_popup_end(p_wnd_menu->p_gui, true);

        // 响应
        klb_wnd_on_command(p_wnd_menu, KLBUI_onchange, NULL, NULL, 0, 0);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_menu_set_css(klb_wnd_t* p_wnd, klbwnd_menu_css_t* p_css)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    p_menu->p_css = p_css;

    for (int i = 0; i < KLBWND_MENU_ITEM_max; i++)
    {
        klbwnd_menu_item_set_css(p_menu->p_items[i], &p_css->css_item);
    }

    klbwnd_menu_2rd_set_css(p_menu->p_menu_2rd, p_css);
}

void klbwnd_menu_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    p_menu->value = sdscpy(p_menu->value, p_value);
}

const sds klbwnd_menu_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    return p_menu->value;
}

void klbwnd_menu_append(klb_wnd_t* p_wnd, klb_map_t* p_array)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    int count = klb_map_array_size(p_array);
    for (int i = 0; i < count; i++)
    {
        klb_map_append_adt_clone(&p_menu->data, klb_map_idx_to_adt(p_array, i));
    }
}

void klbwnd_menu_append_2rd(klb_wnd_t* p_wnd, klb_map_t* p_2rd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    klb_map_iter_t* p_iter = klb_map_begin(p_2rd);
    while (NULL != p_iter)
    {
        klb_map_set_adt_clone(&p_menu->data_2rd, klb_map_key(p_iter), klb_map_data(p_iter));

        p_iter = klb_map_next(p_iter);
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_menu_init_attribute(klb_wnd_t* p_wnd, klbwnd_menu_t* p_menu)
{
    p_menu->value = sdsempty();

    klb_map_init(&p_menu->data);
    klb_map_init(&p_menu->data_2rd);

    p_menu->is_popup_2rd = false;
    p_menu->key_2rd = sdsempty();
}

static void klbwnd_menu_quit_attribute(klbwnd_menu_t* p_menu)
{
    KLB_FREE_BY(p_menu->value, sdsfree);
    KLB_FREE_BY(p_menu->key_2rd, sdsfree);

    klb_map_quit(&p_menu->data);
    klb_map_quit(&p_menu->data_2rd);
}

//////////////////////////////////////////////////////////////////////////
// 子窗口

// 初始化子窗口
static void klbwnd_menu_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;
    
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int item_w = w;
    int item_h = 32;

    for (int i = 0; i <  KLBWND_MENU_ITEM_max; i++)
    {
        klb_wnd_t* p_item = klbwnd_menu_item_create(p_wnd->p_gui, 0, 0, item_w, item_h);

        klb_wnd_push_child(p_wnd, p_item);
        klb_wnd_hide(p_item, true);

        klb_wnd_bind_command(p_item, on_command_item_klbwnd_menu, p_wnd);

        p_menu->p_items[i] = p_item;
    }
}

static void klbwnd_menu_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int item_w = w - 2;
    int item_h = 32;

    int x = 1;
    int y = 1;

    int array_size = klb_map_array_size(&p_menu->data);

    for (int i = 0; i < KLBWND_MENU_ITEM_max; i++)
    {
        klb_wnd_t* p_item = p_menu->p_items[i];

        if (y + item_h <= h && i < array_size)
        {
            klb_map_t* p_item_map = klb_map_idx_to_map(&p_menu->data, i);
            klb_map_iter_t* p_iter = klb_map_begin(p_item_map);

            const char* p_key = klb_map_key(p_iter);

            klbwnd_menu_item_set_value(p_item, p_key);
            klbwnd_menu_item_set_title(p_item, klb_adt_to_string(klb_map_data(p_iter)));

            //是否拥有子菜单
            if (NULL != klb_map_to_map(&p_menu->data_2rd, p_key))
            {
                klbwnd_menu_item_set_has_2rd(p_item, true);
            }
            else
            {
                klbwnd_menu_item_set_has_2rd(p_item, false);
            }

            klb_wnd_move(p_item, x, y);
            klb_wnd_resize(p_item, item_w, item_h);
            klb_wnd_hide(p_item, false);

            y += item_h;
        }
        else
        {
            klb_wnd_hide(p_item, true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_menu_css_init(klbwnd_menu_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);

    p_css->normal.border.color.top = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.left = KLB_ARGB8888(255, 160, 160, 160);

    klbwnd_menu_item_css_init(&p_css->css_item, p_gui);
}

void klbwnd_menu_css_quit(klbwnd_menu_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);

    klbwnd_menu_item_css_quit(&p_css->css_item);
}

void klbwnd_menu_css_copy(klbwnd_menu_css_t* p_dst, klbwnd_menu_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);

    klbwnd_menu_item_css_copy(&p_dst->css_item, &p_src->css_item);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_menu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_menu_on_control;      // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    // 初始化内部变量
    klbwnd_menu_init_attribute(p_wnd, p_menu);

    // 初始化子窗口
    klbwnd_menu_init_subwnds(p_wnd);

    // 初始化子菜单
    p_menu->p_menu_2rd = klbwnd_menu_2rd_create(p_gui, 0, 0, 240, 320);
    klb_wnd_bind_command(p_menu->p_menu_2rd, on_command_2rd_klbwnd_menu, p_wnd);
}

void klbwnd_menu_quit(klb_wnd_t* p_wnd)
{
    klbwnd_menu_t* p_menu = (klbwnd_menu_t*)p_wnd->ctrl;

    // 销毁子菜单
    if (NULL != p_menu->p_menu_2rd)
    {
        klb_wnd_destroy_tree(p_menu->p_menu_2rd);
        p_menu->p_menu_2rd = NULL;
    }

    // 退出属性
    klbwnd_menu_quit_attribute(p_menu);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_menu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_menu_t));

    klbwnd_menu_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_menu_destroy;

    return p_wnd;
}
