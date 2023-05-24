// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_wnd_in.h"
#include "klbgui/klb_gui_in.h"
#include <string.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////


int klb_wnd_push_child(klb_wnd_t* p_parent, klb_wnd_t* p_wnd)
{
    assert(NULL != p_parent);

    // 父窗口
    p_wnd->p_parent = p_parent;

    // 使用父窗口环境
    p_wnd->p_gui = p_parent->p_gui;

    if (NULL == p_parent->p_child)
    {
        p_parent->p_child = p_wnd;
    }
    else
    {
        klb_wnd_t* p_next = p_parent->p_child;
        while (NULL != p_next->p_next)
        {
            p_next = p_next->p_next;
        }

        assert(NULL != p_next);

        p_next->p_next = p_wnd;
        p_wnd->p_prev = p_next;
    }

    // 标记更新屏幕坐标
	klb_wnd_update_canvas_rect(p_wnd);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static void on_destroy_tree_klb_wnd(klb_wnd_t* p_wnd)
{
	// step1. 递归所有子窗口
	klb_wnd_t* p_next = p_wnd->p_child;
	while (NULL != p_next)
	{
		klb_wnd_t* p_cur = p_next;
		p_next = p_next->p_next;

		on_destroy_tree_klb_wnd(p_cur);
	}

	// step2. 销毁当前本窗口
	KLB_FREE_WND(p_wnd);
}

void klb_wnd_destroy_tree(klb_wnd_t* p_wnd)
{
	on_destroy_tree_klb_wnd(p_wnd);
}

klb_gui_t* klb_wnd_get_gui(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    return p_top->p_gui;
}

klb_canvas_t* klb_wnd_get_canvas(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    return p_top->p_gui->p_canvas;
}

uint32_t klb_wnd_get_style(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);
    return p_wnd->state.style;
}

void klb_wnd_set_style(klb_wnd_t* p_wnd, uint32_t style)
{
    assert(NULL != p_wnd);
    p_wnd->state.style = style;
}

void klb_wnd_show(klb_wnd_t* p_wnd, bool show)
{
    assert(NULL != p_wnd);
    if (show)
    {
        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_HIDE);
    }
    else
    {
        p_wnd->state.status |= KLB_WND_STATUS_HIDE;
    }

    // 更新
    klb_wnd_update(p_wnd);
}

/// @brief 基于父窗口移动(相对坐标)
void klb_wnd_move(klb_wnd_t* p_wnd, int x, int y)
{
    assert(NULL != p_wnd);

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
}

/// @brief 重置控件大小
void klb_wnd_resize(klb_wnd_t* p_wnd, int w, int h)
{
    assert(NULL != p_wnd);

    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;
}

void klb_wnd_update(klb_wnd_t* p_wnd)
{
    assert(NULL != p_wnd);

    if (NULL != p_wnd->p_gui)
    {
        klb_gui_update_wnd(p_wnd->p_gui, p_wnd);
    }
}

void klb_wnd_update_canvas_rect(klb_wnd_t* p_wnd)
{
    p_wnd->state.status |= KLB_WND_STATUS_CANVAS_RECT;
}

klb_wnd_t* klb_wnd_get_top(klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_top = p_wnd;

    while (p_top && p_top->p_parent)
    {
        p_top = p_top->p_parent;
    }

    return p_top;
}

//////////////////////////////////////////////////////////////////////////


int klb_wnd_set_top(klb_wnd_t* p_wnd, klb_gui_t* p_gui)
{
    assert(NULL == p_wnd->p_parent);

    p_wnd->state.style |= KLB_WND_STYLE_TOP;
    p_wnd->p_gui = p_gui;

    return 0;
}

bool klb_wnd_is_top(klb_wnd_t* p_wnd)
{
    return (p_wnd->state.style & KLB_WND_STYLE_TOP) ? true : false;
}

void klb_wnd_set_focus(klb_wnd_t* p_wnd, bool focus)
{
    if (focus)
    {
        p_wnd->state.status |= KLB_WND_STATUS_FOCUS;
    }
    else
    {
        p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_FOCUS);
    }
}

//////////////////////////////////////////////////////////////////////////

void klb_wnd_calculate_canvas_rect(klb_wnd_t* p_wnd, int offset_x, int offset_y)
{
    // step1. 更新屏幕坐标
    p_wnd->pos.rect_in_canvas.x = p_wnd->pos.rect_in_parent.x + offset_x;
    p_wnd->pos.rect_in_canvas.y = p_wnd->pos.rect_in_parent.y + offset_y;
    p_wnd->pos.rect_in_canvas.w = p_wnd->pos.rect_in_parent.w;
    p_wnd->pos.rect_in_canvas.h = p_wnd->pos.rect_in_parent.h;

    // step2. 取消更新屏幕坐标标记
    p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_CANVAS_RECT);

    // step3. 递归更新子窗口
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_calculate_canvas_rect(p_next, p_wnd->pos.rect_in_canvas.x, p_wnd->pos.rect_in_canvas.y);

        p_next = p_next->p_next;
    }
}

static int klb_wnd_on_paint(klb_wnd_t* p_wnd)
{
	// step1. 优先使用绑定绘图函数
	if (p_wnd->vtable.on_paint)
	{
		return p_wnd->vtable.on_paint(p_wnd);
	}

	// step2. 再使用 on_control 的 KLBUI_onpaint 事件
    if (p_wnd->vtable.on_control)
    {
		klb_point_t pt = { 0 };
        return p_wnd->vtable.on_control(p_wnd, KLBUI_onpaint, &pt, &pt, 0, 0);
    }

    return 0;
}

static int klb_wnd_on_draw(klb_wnd_t* p_wnd)
{
    // step1. 更新屏幕坐标
    if (KLB_WND_STATUS_CANVAS_RECT & p_wnd->state.status)
    {
        // 需要重新计算窗口基于屏幕的位置
        int off_x = 0, off_y = 0;

        klb_wnd_t* p_parent = p_wnd->p_parent;
        if (NULL != p_parent)
        {
            off_x = p_parent->pos.rect_in_canvas.x;
            off_y = p_parent->pos.rect_in_canvas.y;
        }

        // 更新自身及所有子窗口
        klb_wnd_calculate_canvas_rect(p_wnd, off_x, off_y);
    }

    // step2. 检查隐藏
    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // step3. 绘制自身
    klb_wnd_on_paint(p_wnd);

    // step4. 递归子窗口
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_on_draw(p_next);

        p_next = p_next->p_next;
    }

    return 0;
}

int klb_wnd_draw(klb_wnd_t* p_wnd)
{
    klb_wnd_on_draw(p_wnd);

    return 0;
}

//////////////////////////////////////////////////////////////////////////

klb_wnd_t* klb_wnd_pt_in(klb_wnd_t* p_wnd, int x, int y)
{
    if (p_wnd->state.status & KLB_WND_STATUS_HIDE)
    {
        return NULL; // 窗口处于隐藏, 则不再寻找
    }

    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_t* p_in = klb_wnd_pt_in(p_next, x, y);
        if (NULL != p_in)
        {
            return p_in;
        }

        p_next = p_next->p_next;
    }

    if (!(p_wnd->state.style & KLB_WND_STYLE_NOFOCUS) &&    // 可聚焦样式 
        !(p_wnd->state.status & KLB_WND_STATUS_HIDE) &&     // 显示
        klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, x, y))   // 在窗口区域内部
    {
        return p_wnd;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// command

int klb_wnd_bind_command(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* p_obj)
{
    if (p_wnd->state.style & KLB_WND_STYLE_NOCOMMAND)
    {
        return 2; // 不能绑定 on_command 响应
    }

    p_wnd->p_udata = p_obj;
    p_wnd->vtable.on_command = on_command;

    return 0;
}

int klb_wnd_bind_paint(klb_wnd_t* p_wnd, klb_wnd_on_paint_cb on_paint)
{
    p_wnd->vtable.on_paint = on_paint;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// get / set

int klb_wnd_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    if (p_wnd && p_wnd->vtable.on_set)
    {
        return p_wnd->vtable.on_set(p_wnd, p_map);
    }

    return 1;
}

klb_map_t* klb_wnd_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    if (p_wnd && p_wnd->vtable.on_get)
    {
        return p_wnd->vtable.on_get(p_wnd, p_map);
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// 绘图

int klb_wnd_set_draw_color(klb_wnd_t* p_wnd, uint32_t color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_draw_color(p_canvas, color);

    return ret;
}

uint32_t klb_wnd_get_draw_color(klb_wnd_t* p_wnd)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    return klb_canvas_get_draw_color(p_canvas);
}

int klb_wnd_set_font_height(klb_wnd_t* p_wnd, int h)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_set_font_height(p_canvas, h);

    return ret;
}

int klb_wnd_get_font_height(klb_wnd_t* p_wnd)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    return klb_canvas_get_font_height(p_canvas);
}

int klb_wnd_draw_clear(klb_wnd_t* p_wnd, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }
    
    ret |= klb_canvas_draw_clear(p_canvas);

    return ret;
}

int klb_wnd_draw_point(klb_wnd_t* p_wnd, int x, int y, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_point(p_canvas, x, y);

    return ret;
}

int klb_wnd_draw_points(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_points(p_canvas, p_points, count);

    return ret;
}

int klb_wnd_draw_line(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_line(p_canvas, x1, y1, x2, y2);

    return ret;
}

int klb_wnd_draw_lines(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_lines(p_canvas, p_points, count);

    return ret;
}

int klb_wnd_draw_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_rect(p_canvas, p_rect);

    return ret;
}

int klb_wnd_draw_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_rects(p_canvas, p_rects, count);

    return ret;
}

int klb_wnd_draw_fill_rect(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }
    ret |= klb_canvas_draw_fill_rect(p_canvas, p_rect);

    return ret;
}

int klb_wnd_draw_fill_rects(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t* p_color)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        int ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    ret |= klb_canvas_draw_fill_rects(p_canvas, p_rects, count);

    return ret;
}

int klb_wnd_draw_text(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t* p_color, int* p_font_h)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_color)
    {
        ret = klb_canvas_set_draw_color(p_canvas, *p_color);
    }

    if (NULL != p_font_h)
    {
        ret |= klb_canvas_set_font_height(p_canvas, *p_font_h);
    }

    ret |= klb_canvas_draw_text(p_canvas, p_rect, p_utf8, utf8_len);

    return ret;
}

int klb_wnd_text_size(klb_wnd_t* p_wnd, const char* p_utf8, int utf8_len, int* p_font_h, int* p_out_w, int* p_out_h)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = 0;

    if (NULL != p_font_h)
    {
        ret |= klb_canvas_set_font_height(p_canvas, *p_font_h);
    }

    ret |= klb_canvas_text_size(p_canvas, p_utf8, utf8_len, p_out_w, p_out_h);

    return ret;
}

int klb_wnd_draw_image(klb_wnd_t* p_wnd, const klb_rect_t* p_dst_rect, const char* p_path, const klb_rect_t* p_src_rect)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    int ret = klb_canvas_draw_image(p_canvas, p_dst_rect, p_path);

    return ret;
}

int klb_wnd_image_size(klb_wnd_t* p_wnd, const char* p_path, int* p_out_w, int* p_out_h)
{
    klb_canvas_t* p_canvas = klb_wnd_get_canvas(p_wnd);

    return klb_canvas_image_size(p_canvas, p_path, p_out_w, p_out_h);
}


int klb_wnd_draw_clear2(klb_wnd_t* p_wnd, uint32_t color)
{
    return klb_wnd_draw_clear(p_wnd, &color);
}

int klb_wnd_draw_point2(klb_wnd_t* p_wnd, int x, int y, uint32_t color)
{
    return klb_wnd_draw_point(p_wnd, x, y, &color);
}

int klb_wnd_draw_points2(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color)
{
    return klb_wnd_draw_points(p_wnd, p_points, count, &color);
}

int klb_wnd_draw_line2(klb_wnd_t* p_wnd, int x1, int y1, int x2, int y2, uint32_t color)
{
    return klb_wnd_draw_line(p_wnd, x1, y1, x2, y2, &color);
}

int klb_wnd_draw_lines2(klb_wnd_t* p_wnd, const klb_point_t* p_points, int count, uint32_t color)
{
    return klb_wnd_draw_lines(p_wnd, p_points, count, &color);
}

int klb_wnd_draw_rect2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color)
{
    return klb_wnd_draw_rect(p_wnd, p_rect, &color);
}

int klb_wnd_draw_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color)
{
    return klb_wnd_draw_rects(p_wnd, p_rects, count, &color);
}

int klb_wnd_draw_fill_rect2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, uint32_t color)
{
    return klb_wnd_draw_fill_rect(p_wnd, p_rect, &color);
}

int klb_wnd_draw_fill_rects2(klb_wnd_t* p_wnd, const klb_rect_t* p_rects, int count, uint32_t color)
{
    return klb_wnd_draw_fill_rects(p_wnd, p_rects, count, &color);
}

int klb_wnd_text_size2(klb_wnd_t* p_wnd, const char* p_utf8, int utf8_len, int font_h, int* p_out_w, int* p_out_h)
{
    return klb_wnd_text_size(p_wnd, p_utf8, utf8_len, &font_h, p_out_w, p_out_h);
}

int klb_wnd_draw_text2(klb_wnd_t* p_wnd, const klb_rect_t* p_rect, const char* p_utf8, int utf8_len, uint32_t color, int font_h)
{
    return klb_wnd_draw_text(p_wnd, p_rect, p_utf8, utf8_len, &color, &font_h);
}
