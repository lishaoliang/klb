// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_combo.h"
#include "klbgui/shwnd/klbshw_combomenu.h"
#include "klbgui/klbui_util.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_combo_quit_attribute(klbwnd_combo_t* p_combo);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_combo_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    klbwnd_combo_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_combo_on_paint_status(klb_wnd_t* p_wnd, klbwnd_combo_t* p_combo, klbwnd_combo_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    // 标题文本
    klbuicssex_draw_text(p_wnd, p_combo->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);

    // 右侧倒三角
    klb_rect_t rect_triangle = { 0 };
    rect_triangle.h = p_rect->h - 2;
    rect_triangle.w = rect_triangle.h / 2;
    rect_triangle.x = p_rect->x + p_rect->w - rect_triangle.w - 2;
    rect_triangle.y = p_rect->y + 1;

    klbuiutil_draw_triangle_down(p_wnd, &rect_triangle, p_attr->text.color);
}

static int klbwnd_combo_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_combo_css_t* p_css = p_combo->p_css;

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
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_combo_on_paint_status(p_wnd, p_combo, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

static int on_result_combomenu_klbui_combo(void* ptr, bool ok, const sds value, const sds title)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)ptr;
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    if (ok)
    {
        bool change = false;
        if (0 != sdscmp(p_combo->value, value))
        {
            change = true;
        }

        // 用户已经点击了某个选项
        p_combo->value = sdscpy(p_combo->value, value);
        p_combo->title = sdscpy(p_combo->title, title);

        if (change)
        {
            // 内容变更事件 KLBUI_onchange
            klb_wnd_call_command(p_wnd, KLBUI_onchange, NULL, NULL, 0, 0);
        }

        klb_wnd_update(p_wnd);
    }

    return 0;
}

static int klbwnd_combo_on_click(klb_wnd_t* p_wnd, klbwnd_combo_t* p_combo, const klb_point_t* p_pt1)
{
    if (klb_wnd_is_disable(p_wnd))
    {
        return 0;
    }

    // 1. 设置初始值

    // 2. 设置css

    // 3. 绑定响应
    int menu_w = 0, menu_h = 0;
    klbshw_combomenu_bind(p_combo->p_menu, on_result_combomenu_klbui_combo, p_wnd, &p_combo->data, &menu_w, &menu_h);

    // 4. 处理位置 
    int screen_w = 0, screen_h = 0;
    klb_gui_get_wh(p_wnd->p_gui, &screen_w, &screen_h);

    klb_rect_t rect = p_wnd->pos.rect_in_canvas;

    int x = (rect.x + menu_w <= screen_w) ? rect.x : screen_w - menu_w;
    int y = (rect.y + rect.h + menu_h <= screen_h) ? (rect.y + rect.h) : rect.y - menu_h;

    klb_wnd_move(p_combo->p_menu, x, y);
    klb_wnd_resize(p_combo->p_menu, menu_w, menu_h);

    // 5. popup
    klb_gui_popup_wnd(p_wnd->p_gui, p_combo->p_menu);

    return 0;
}

static int klbwnd_combo_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_combo_on_paint(p_wnd);
        break;

    case KLBUI_click:
    case KLBUI_dblclick:
        return klbwnd_combo_on_click(p_wnd, p_combo, p_pt1);
        break;

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_combo_set_css(klb_wnd_t* p_wnd, klbwnd_combo_css_t* p_css)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->p_css = p_css;
}

void klbwnd_combo_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->index = index;
}

int klbwnd_combo_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->index;
}

void klbwnd_combo_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->title = sdscpy(p_combo->title, p_title);
}

const sds klbwnd_combo_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->title;
}

void klbwnd_combo_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_combo->value = sdscpy(p_combo->value, p_value);
}

const sds klbwnd_combo_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    return p_combo->value;
}

void klbwnd_combo_append(klb_wnd_t* p_wnd, klb_map_t* p_array)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;
    
    int count = klb_map_array_size(p_array);
    for (int i = 0; i < count; i++)
    {
        klb_map_append_adt_clone(&p_combo->data, klb_map_idx_to_adt(p_array, i));
    }
}

void klbwnd_combo_clear(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    // 清空待选列表
    klb_map_clear(&p_combo->data);

    sdsclear(p_combo->title);
    sdsclear(p_combo->value);
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_combo_init_attribute(klbwnd_combo_t* p_combo)
{
    p_combo->index = 0;
    p_combo->title = sdsempty();
    p_combo->value = sdsempty();

    klb_map_init(&p_combo->data);
}

static void klbwnd_combo_quit_attribute(klbwnd_combo_t* p_combo)
{
    klb_map_quit(&p_combo->data);

    KLB_FREE_BY(p_combo->title, sdsfree);
    KLB_FREE_BY(p_combo->value, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_combo_css_init(klbwnd_combo_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);
}

void klbwnd_combo_css_quit(klbwnd_combo_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

void klbwnd_combo_css_copy(klbwnd_combo_css_t* p_dst, klbwnd_combo_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_combo_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                       // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_combo_on_control; // 这里指定
    p_wnd->vtable.on_command = NULL;                    // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                        // 由继承者重写
    p_wnd->vtable.on_get = NULL;                        // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_combo_init_attribute(p_combo);

    // 弹出菜单
    p_combo->p_menu = klbui_shwnd_get_combomenu(p_gui);
}

void klbwnd_combo_quit(klb_wnd_t* p_wnd)
{
    klbwnd_combo_t* p_combo = (klbwnd_combo_t*)p_wnd->ctrl;

    klbwnd_combo_quit_attribute(p_combo);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_combo_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_combo_t));

    klbwnd_combo_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_combo_destroy;

    return p_wnd;
}
