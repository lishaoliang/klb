// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_menu_2rd.h"
#include "klbgui/subviews/klbwnd_menu_item.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_menu_2rd_quit_attribute(klbwnd_menu_2rd_t* p_menu);
static void klbwnd_menu_2rd_relayout(klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_menu_2rd_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    klbwnd_menu_2rd_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_menu_2rd_on_paint_status(klb_wnd_t* p_wnd, klbwnd_menu_2rd_t* p_menu, klbwnd_menu_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
}

static int klbwnd_menu_2rd_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;
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
        klbwnd_menu_2rd_on_paint_status(p_wnd, p_menu, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int klbwnd_menu_2rd_suggestw(klb_wnd_t* p_wnd, klbwnd_menu_2rd_t* p_menu)
{
    int item_h = 32;
    int font_h = (NULL != p_menu->p_css) ? p_menu->p_css->normal.font.size : 24;
    int w = 0;

    int array_size = klb_map_array_size(p_menu->p_data_ref);
    for (int i = 0; i < array_size; i++)
    {
        klb_map_t* p_item_map = klb_map_idx_to_map(p_menu->p_data_ref, i);
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

static int klbwnd_menu_2rd_suggesth(klb_wnd_t* p_wnd, klbwnd_menu_2rd_t* p_menu)
{
    if (NULL != p_menu->p_data_ref)
    {
        int item_h = 32;
        int array_size = klb_map_array_size(p_menu->p_data_ref);

        return item_h * array_size + 2;
    }

    return 0;
}

static int klbwnd_menu_2rd_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onunload:
        {
            p_menu->p_data_ref = NULL;
        }
        break;

    case KLBUI_onpaint:
        return klbwnd_menu_2rd_on_paint(p_wnd);
        break;

    case KLBUI_onpredraw:
        klbwnd_menu_2rd_relayout(p_wnd);
        break;

    case KLBUI_outwindow:
        klb_gui_popup_end(p_wnd->p_gui, true);
        break;

    case KLBUI_suggestw:
        return klbwnd_menu_2rd_suggestw(p_wnd, p_menu);
        break;

    case KLBUI_suggesth:
        return klbwnd_menu_2rd_suggesth(p_wnd, p_menu);
        break;

    case KLBUI_meminfo:
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

// 本菜单项事件响应
static int on_command_item_klbwnd_menu_2rd(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd_menu->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        // 保存值
        p_menu->value = sdscpy(p_menu->value, klbwnd_menu_item_get_value(p_wnd));

        // 响应
        klb_wnd_on_command(p_wnd_menu, KLBUI_onchange, NULL, NULL, 0, 0);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_menu_2rd_set_css(klb_wnd_t* p_wnd, klbwnd_menu_css_t* p_css)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    p_menu->p_css = p_css;

    for (int i = 0; i < KLBWND_MENU_ITEM_max; i++)
    {
        klbwnd_menu_item_set_css(p_menu->p_items[i], &p_css->css_item);
    }
}

void klbwnd_menu_2rd_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    p_menu->value = sdscpy(p_menu->value, p_value);
}

const sds klbwnd_menu_2rd_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    return p_menu->value;
}

void klbwnd_menu_set_data_ref(klb_wnd_t* p_wnd, klb_map_t* p_data_ref)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    p_menu->p_data_ref = p_data_ref;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_menu_2rd_init_attribute(klb_wnd_t* p_wnd, klbwnd_menu_2rd_t* p_menu)
{
    p_menu->value = sdsempty();

    p_menu->p_data_ref = NULL;
}

static void klbwnd_menu_2rd_quit_attribute(klbwnd_menu_2rd_t* p_menu)
{
    KLB_FREE_BY(p_menu->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// 子窗口

// 初始化子窗口
static void klbwnd_menu_2rd_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;
    
    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int item_w = w;
    int item_h = 32;

    for (int i = 0; i <  KLBWND_MENU_ITEM_max; i++)
    {
        klb_wnd_t* p_item = klbwnd_menu_item_create(p_wnd->p_gui, 0, 0, item_w, item_h);

        klb_wnd_push_child(p_wnd, p_item);
        klb_wnd_hide(p_item, true);

        klb_wnd_bind_command(p_item, on_command_item_klbwnd_menu_2rd, p_wnd);

        p_menu->p_items[i] = p_item;
    }
}

static void klbwnd_menu_2rd_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int item_w = w - 2;
    int item_h = 32;

    int x = 1;
    int y = 1;

    int array_size = klb_map_array_size(p_menu->p_data_ref);

    for (int i = 0; i < KLBWND_MENU_ITEM_max; i++)
    {
        klb_wnd_t* p_item = p_menu->p_items[i];

        if (y + item_h <= h && i < array_size)
        {
            klb_map_t* p_item_map = klb_map_idx_to_map(p_menu->p_data_ref, i);
            klb_map_iter_t* p_iter = klb_map_begin(p_item_map);

            const char* p_key = klb_map_key(p_iter);

            klbwnd_menu_item_set_value(p_item, p_key);
            klbwnd_menu_item_set_title(p_item, klb_adt_to_string(klb_map_data(p_iter)));

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
// init / quit

void klbwnd_menu_2rd_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_menu_2rd_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_CONTINUE;

    // 初始化内部变量
    klbwnd_menu_2rd_init_attribute(p_wnd, p_menu);

    // 初始化子窗口
    klbwnd_menu_2rd_init_subwnds(p_wnd);
}

void klbwnd_menu_2rd_quit(klb_wnd_t* p_wnd)
{
    klbwnd_menu_2rd_t* p_menu = (klbwnd_menu_2rd_t*)p_wnd->ctrl;

    klbwnd_menu_2rd_quit_attribute(p_menu);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_menu_2rd_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_menu_2rd_t));

    klbwnd_menu_2rd_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_menu_2rd_destroy;

    return p_wnd;
}
