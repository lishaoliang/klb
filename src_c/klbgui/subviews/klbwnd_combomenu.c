// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_combomenu.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_combomenu_quit_attribute(klbwnd_combomenu_t* p_menu);
static void klbwnd_combomenu_relayout(klb_wnd_t* p_wnd);
static void update_items_value_title_klbwnd_combomenu(klb_wnd_t* p_wnd_menu, klbwnd_combomenu_t* p_menu, int pos);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_combomenu_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    klbwnd_combomenu_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_combomenu_on_paint_status(klb_wnd_t* p_wnd, klbwnd_combomenu_t* p_menu, klbwnd_combomenu_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
}

static int klbwnd_combomenu_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_combomenu_css_t* p_css = p_menu->p_css;

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

    klbwnd_combomenu_on_paint_status(p_wnd, p_menu, p_css, &p_css->normal, &paint_rect);

    return 0;
}

// 鼠标点击对话框外部
static int klbwnd_combomenu_on_outwindow(klb_wnd_t* p_wnd, klbwnd_combomenu_t* p_menu, const klb_point_t* p_pt1)
{
    if (NULL != p_menu->cb_result)
    {
        p_menu->cb_result(p_menu->ptr, false, NULL, NULL);
    }

    klb_gui_popup_end(p_wnd->p_gui, false);

    return 0;
}

// 鼠标滚轮事件
static int klbwnd_combomenu_on_mousewheel(klb_wnd_t* p_wnd, klbwnd_combomenu_t* p_menu, const klb_point_t* p_pt1, int lparam)
{
    int v = KLBUI_MOUSEWHEEL_value(lparam);

    if (v <= 0 || !klb_wnd_is_show(p_menu->p_vscrollbar))
    {
        return 0;
    }

    if (NULL != p_pt1 && !klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, p_pt1->x, p_pt1->y))
    {
        return 0; // 窗口之外
    }

    if (KLBUI_MOUSEWHEEL_is_up(lparam))
    {
        if (0 == klbwnd_vscrollbar_up(p_menu->p_vscrollbar, v))
        {
            int pos = klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar);
            update_items_value_title_klbwnd_combomenu(p_wnd, p_menu, pos);
            klb_wnd_update(p_wnd);
        }
    }
    else if (KLBUI_MOUSEWHEEL_is_down(lparam))
    {
        if (0 == klbwnd_vscrollbar_down(p_menu->p_vscrollbar, v))
        {
            int pos = klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar);
            update_items_value_title_klbwnd_combomenu(p_wnd, p_menu, pos);
            klb_wnd_update(p_wnd);
        }
    }

    return 0;
}

// 统计控件消耗的内存
static int klbwnd_combomenu_on_meminfo(klb_wnd_t* p_wnd, klbwnd_combomenu_t* p_menu)
{
    int mems = 0;

    // 子项目
    for (int i = 0; i < KLBWND_COMBOMENU_item_max; i++)
    {
        mems += klb_wnd_on_control(p_menu->p_items[i], KLBUI_meminfo, NULL, NULL, 0, 0);
    }

    // 滚动条
    mems += klb_wnd_on_control(p_menu->p_vscrollbar, KLBUI_meminfo, NULL, NULL, 0, 0);

    // 自身
    mems += sizeof(klb_wnd_t);
    mems += sizeof(klbwnd_combomenu_t);

    return mems;
}

static int klbwnd_combomenu_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_combomenu_on_paint(p_wnd);
        break;

    case KLBUI_outwindow: // 在窗口之外点击, 结束 popup
        return klbwnd_combomenu_on_outwindow(p_wnd, p_menu, p_pt1);
        break;

    case KLBUI_onresize: // 控件尺寸变化, 重新布局
        {
            klbwnd_combomenu_relayout(p_wnd);
        }
        break;

    case KLBUI_mousewheel:
        return klbwnd_combomenu_on_mousewheel(p_wnd, p_menu, p_pt1, lparam);
        break;

    case KLBUI_meminfo: // 统计控件消耗的内存
        return klbwnd_combomenu_on_meminfo(p_wnd, p_menu);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 子窗口响应

// 子按钮项响应函数
static int on_command_item_klbwnd_combomenu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd_menu->ctrl;

    if (KLBUI_click == msg)
    {
        if (NULL != p_menu->cb_result)
        {
            const sds value = klbwnd_combomenu_item_get_value(p_wnd);
            const sds title = klbwnd_combomenu_item_get_title(p_wnd);

            p_menu->cb_result(p_menu->ptr, true, value, title);
        }

        // 结束
        klb_gui_popup_end(p_wnd->p_gui, false);
    }

    return 0;
}

static void update_items_value_title_klbwnd_combomenu(klb_wnd_t* p_wnd_menu, klbwnd_combomenu_t* p_menu, int pos)
{
    for (int i = 0; i < p_menu->item_count; i++)
    {
        klb_wnd_t* p_item = p_menu->p_items[i];

        if (pos < p_menu->array_size)
        {
            klb_map_t* p_data = klb_map_idx_to_map(p_menu->p_ref_data_array, pos);
            klb_map_iter_t* p_iter = klb_map_begin(p_data);

            klbwnd_combomenu_item_set_value(p_item, klb_map_key(p_iter));
            klbwnd_combomenu_item_set_title(p_item, klb_adt_to_string(klb_map_data(p_iter)));

            klb_wnd_hide(p_item, false);

            pos += 1;
        }
        else
        {
            klb_wnd_hide(p_item, true);
        }
    }
}

// 滚动条响应函数
static int on_command_vscrollbar_klbwnd_combomenu(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_menu = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd_menu->ctrl;

    if (KLBUI_onchange == msg)
    {
        int pos = klbwnd_vscrollbar_get_value(p_menu->p_vscrollbar);

        update_items_value_title_klbwnd_combomenu(p_wnd_menu, p_menu, pos);

        klb_wnd_update(p_wnd_menu);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_combomenu_set_css(klb_wnd_t* p_wnd, klbwnd_combomenu_css_t* p_css)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    p_menu->p_css = p_css;

    if (NULL != p_css)
    {
        for (int i = 0; i < KLBWND_COMBOMENU_item_max; i++)
        {
            klbwnd_combomenu_item_set_css(p_menu->p_items[i], &p_css->css_item);
        }

        klbwnd_vscrollbar_set_css(p_menu->p_vscrollbar, &p_css->css_vscrollbar);
    }
}

int klbwnd_combomenu_bind(klb_wnd_t* p_wnd, klbshw_combomenu_result_cb cb_result, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    p_menu->cb_result = cb_result;
    p_menu->ptr = ptr;

    p_menu->p_ref_data_array = p_data_array;
    p_menu->array_size = klb_map_array_size(p_menu->p_ref_data_array);

    // 统计高度
    int need_h = 0;
    {
        int count = MIN(p_menu->array_size, KLBWND_COMBOMENU_item_max);

        need_h = count * 32 + 2;
    }

    p_menu->need_w = 240;
    p_menu->need_h = need_h;

    if (NULL != p_out_w)
    {
        *p_out_w = p_menu->need_w;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = p_menu->need_h;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_combomenu_init_attribute(klb_wnd_t* p_wnd, klbwnd_combomenu_t* p_menu)
{

}

static void klbwnd_combomenu_quit_attribute(klbwnd_combomenu_t* p_menu)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_combomenu_css_init(klbwnd_combomenu_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);

    p_css->normal.border.color.top = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.right = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.bottom = KLB_ARGB8888(255, 160, 160, 160);
    p_css->normal.border.color.left = KLB_ARGB8888(255, 160, 160, 160);

    klbwnd_combomenu_item_css_init(&p_css->css_item, p_gui);
    klbwnd_vscrollbar_css_init(&p_css->css_vscrollbar, p_gui);

    p_css->css_vscrollbar.normal.border.width.top = 0;
    p_css->css_vscrollbar.normal.border.width.right = 0;
    p_css->css_vscrollbar.normal.border.width.bottom = 0;

    p_css->css_vscrollbar.focus.border.width.top = 0;
    p_css->css_vscrollbar.focus.border.width.right = 0;
    p_css->css_vscrollbar.focus.border.width.bottom = 0;

    p_css->css_vscrollbar.disable.border.width.top = 0;
    p_css->css_vscrollbar.disable.border.width.right = 0;
    p_css->css_vscrollbar.disable.border.width.bottom = 0;
}

void klbwnd_combomenu_css_quit(klbwnd_combomenu_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);

    klbwnd_combomenu_item_css_quit(&p_css->css_item);
    klbwnd_vscrollbar_css_quit(&p_css->css_vscrollbar);
}

//////////////////////////////////////////////////////////////////////////
// 子窗口

// 初始化子窗口
static void klbwnd_combomenu_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;
    klbwnd_combomenu_css_t* p_css = p_menu->p_css;

    // 按钮项
    for (int i = 0; i < KLBWND_COMBOMENU_item_max; i++)
    {
        klb_wnd_t* p_item = klbwnd_combomenu_item_create(p_gui, 0, 0, 80, 32);

        if (NULL != p_css)
        {
            klbwnd_combomenu_item_set_css(p_item, &p_css->css_item);
        }

        klb_wnd_push_child(p_wnd, p_item);
        klb_wnd_hide(p_item, true);
        klb_wnd_bind_command(p_item, on_command_item_klbwnd_combomenu, p_wnd);

        p_menu->p_items[i] = p_item;
    }

    // 垂直滚动条
    {
        p_menu->p_vscrollbar = klbwnd_vscrollbar_create(p_gui, 0, 0, 20, 80);

        if (NULL != p_css)
        {
            klbwnd_vscrollbar_set_css(p_menu->p_vscrollbar, &p_css->css_vscrollbar);
        }

        klb_wnd_push_child(p_wnd, p_menu->p_vscrollbar);
        klb_wnd_hide(p_menu->p_vscrollbar, true);
        klb_wnd_bind_command(p_menu->p_vscrollbar, on_command_vscrollbar_klbwnd_combomenu, p_wnd);

        klbwnd_vscrollbar_enable_mousewheel(p_menu->p_vscrollbar, false);
    }

    p_menu->item_count = 0;
}

// 重新布局
static void klbwnd_combomenu_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    klb_rect_t rect = p_wnd->pos.rect_in_parent;
    int total_h = rect.h;

    int w = rect.w - 2;
    int h = 32;
    int x = 1;
    int y = 1;

    // 最多可以显示选项
    int item_count = total_h / h;
    item_count = MIN(item_count, KLBWND_COMBOMENU_item_max);
    p_menu->item_count = item_count;

    int data_count = p_menu->array_size;    // 数据条目
    bool is_show_vscrollbar = (data_count <= item_count) ? false : true;    // 是否显示滚动条

    if (is_show_vscrollbar)
    {
        w -= (20 + x);
    }

    for (int i = 0; i < KLBWND_COMBOMENU_item_max; i++)
    {
        klb_wnd_t* p_item = p_menu->p_items[i];

        if (y + h <= total_h)
        {
            klb_wnd_move(p_item, x, y);
            klb_wnd_resize(p_item, w, h);

            klb_wnd_hide(p_item, false); // 显示

            y += h;
        }
        else
        {
            klb_wnd_hide(p_item, true); // 隐藏
        } 
    }

    // 滚动条
    if (is_show_vscrollbar)
    {
        klbwnd_vscrollbar_set_ranges(p_menu->p_vscrollbar, 0, data_count - item_count / 2, 1);
        klbwnd_vscrollbar_set_value(p_menu->p_vscrollbar, 0);

        klb_wnd_move(p_menu->p_vscrollbar, x + w, 1);
        klb_wnd_resize(p_menu->p_vscrollbar, 20, total_h - 2);

        klb_wnd_hide(p_menu->p_vscrollbar, false); // 显示

        update_items_value_title_klbwnd_combomenu(p_wnd, p_menu, 0);
    }
    else
    {
        klb_wnd_hide(p_menu->p_vscrollbar, true); // 隐藏

        update_items_value_title_klbwnd_combomenu(p_wnd, p_menu, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_combomenu_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_combomenu_on_control; // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_PEEK_EVENT | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    // 初始化内部变量
    klbwnd_combomenu_init_attribute(p_wnd, p_menu);

    // 子窗口
    klbwnd_combomenu_init_subwnds(p_wnd);
}

void klbwnd_combomenu_quit(klb_wnd_t* p_wnd)
{
    klbwnd_combomenu_t* p_menu = (klbwnd_combomenu_t*)p_wnd->ctrl;

    klbwnd_combomenu_quit_attribute(p_menu);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_combomenu_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_combomenu_t));

    klbwnd_combomenu_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_combomenu_destroy;

    return p_wnd;
}
