// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_vscrollbar.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_vscrollbar_quit_attribute(klbwnd_vscrollbar_t* p_vsc);
static void klbwnd_vscrollbar_relayout(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc);
static void klbwnd_vscrollbar_update_value(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int value);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_vscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    klbwnd_vscrollbar_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_vscrollbar_on_paint_status(klb_wnd_t* p_wnd, klbwnd_vscrollbar_t* p_vsc, klbwnd_vscrollbar_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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
}

static int klbwnd_vscrollbar_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_vscrollbar_css_t* p_css = p_vsc->p_css;

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

    //if (KLB_WND_STATUS_DISABLE & p_wnd->state.status)
    //{
    //    klbwnd_vscrollbar_on_paint_status(p_wnd, p_vsc, p_css, &p_css->disable, &paint_rect);
    //}
    //else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    //{
    //    klbwnd_vscrollbar_on_paint_status(p_wnd, p_vsc, p_css, &p_css->focus, &paint_rect);
    //}
    //else
    {
        klbwnd_vscrollbar_on_paint_status(p_wnd, p_vsc, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_vscrollbar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_vscrollbar_on_paint(p_wnd);
	case KLBUI_click:
	case KLBUI_dblclick:
		{
			int y1 = p_vsc->p_up->pos.rect_in_parent.h;
			int y2 = p_vsc->p_down->pos.rect_in_parent.h;
			int h = p_wnd->pos.rect_in_parent.h;

			int offy = p_pt1->y - p_wnd->pos.rect_in_canvas.y;

			if (y1 < offy && offy < h - y2)
			{
				int v = p_vsc->min + (offy - y1) * (p_vsc->max - p_vsc->min + 1) / (h - y1 - y2);

				klbwnd_vscrollbar_update_value(p_wnd, p_vsc, v);
			}
		}
		break;
    case KLBUI_onresize:
        {
            klbwnd_vscrollbar_relayout(p_wnd, p_vsc);
        }
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

static void klbwnd_vscrollbar_update_value(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc, int value)
{
	if (value < p_vsc->min) { value = p_vsc->min; }
	if (p_vsc->max < value) { value = p_vsc->max; }

	if (value != p_vsc->value)
	{
		p_vsc->value = value;

		// on_command
		if (p_wnd_vsc->vtable.on_command)
		{
			klb_point_t pt = { 0 };
			p_wnd_vsc->vtable.on_command(p_wnd_vsc, KLBUI_onchange, &pt, &pt, 0, 0);
		}

		klbwnd_vscrollbar_relayout(p_wnd_vsc, p_vsc);
		klb_wnd_update(p_wnd_vsc);
	}
}

static void klbwnd_vscrollbar_relayout(klb_wnd_t* p_wnd_vsc, klbwnd_vscrollbar_t* p_vsc)
{
    int w = p_wnd_vsc->pos.rect_in_parent.w;
    int h = p_wnd_vsc->pos.rect_in_parent.h;

    int w_up = w - 2, h_up = w - 2;

    klb_wnd_move(p_vsc->p_up, 1, 1);
    klb_wnd_resize(p_vsc->p_up, w_up, h_up);

    int w_down = w - 2, h_down = w - 2;

    klb_wnd_move(p_vsc->p_down, 1,  h - h_down - 2);
    klb_wnd_resize(p_vsc->p_down, w_down, h_down);

    klb_rect_t r = { 0, 0, w, h };
    r.y += (h_up + 2);
    r.h -= (h_up + 2);
    //r.y += (h_down + 2);
    r.h -= (h_down + 2);

    int w_middle = w - 2, h_middle = 42;
    int sy = r.y;

	if (p_vsc->value <= p_vsc->min)
	{
		sy = r.y + 1;
	}
	else if(p_vsc->max <= p_vsc->value)
	{
		sy = r.y + r.h - h_middle;
	}
	else
	{
		sy = r.y + (p_vsc->value - p_vsc->min + 1) * (r.h - h_middle + 1) / (p_vsc->max - p_vsc->min + 1);
	}

    klb_wnd_move(p_vsc->p_middle, 1, sy);
    klb_wnd_resize(p_vsc->p_middle, w_middle, h_middle);

	// 需要更新屏幕坐标
	klb_wnd_update_canvas_rect(p_wnd_vsc);
}

// 向上按钮响应
static int on_command_bnt_up_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_click == e || KLBUI_dblclick == e)
    {
		klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value - p_vsc->step);
    }

    return 0;
}

// 向下按钮响应
static int on_command_bnt_down_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    if (KLBUI_click == e || KLBUI_dblclick == e)
    {
		klbwnd_vscrollbar_update_value(p_wnd_vsc, p_vsc, p_vsc->value + p_vsc->step);
    }

    return 0;
}

// 中间滑块响应
static int on_command_bnt_middle_klbwnd_vscrollbar(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_vsc = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd_vsc->ctrl;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_vscrollbar_set_css(klb_wnd_t* p_wnd, klbwnd_vscrollbar_css_t* p_css)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_vsc->p_css = p_css;

    klbwnd_btnex_set_css(p_vsc->p_up, &p_css->css_btnex);
    klbwnd_btnex_set_css(p_vsc->p_down, &p_css->css_btnex);
    klbwnd_btnex_set_css(p_vsc->p_middle, &p_css->css_btnex);
}

void klbwnd_vscrollbar_set_value(klb_wnd_t* p_wnd, int value)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    if (value < p_vsc->min) { value = p_vsc->min; }
    if (p_vsc->max < value) { value = p_vsc->max; }

    p_vsc->value = value;

    klbwnd_vscrollbar_relayout(p_wnd, p_vsc);
}

int klbwnd_vscrollbar_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    return p_vsc->value;
}

void klbwnd_vscrollbar_set_ranges(klb_wnd_t* p_wnd, int min, int max, int step)
{
	klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

	p_vsc->min = min;
	p_vsc->max = max;
	p_vsc->step = step;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_vscrollbar_init_attribute(klbwnd_vscrollbar_t* p_vsc)
{
    p_vsc->min = 0;
    p_vsc->max = 10;
	p_vsc->step = 1;
    p_vsc->value = 0;
}

static void klbwnd_vscrollbar_quit_attribute(klbwnd_vscrollbar_t* p_vsc)
{

}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_vscrollbar_css_init(klbwnd_vscrollbar_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_btnex_css_init(&p_css->css_btnex, p_gui);
}

void klbwnd_vscrollbar_css_quit(klbwnd_vscrollbar_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);

    klbwnd_btnex_css_quit(&p_css->css_btnex);
}

void klbwnd_vscrollbar_css_copy(klbwnd_vscrollbar_css_t* p_dst, klbwnd_vscrollbar_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);

    klbwnd_btnex_css_copy(&p_dst->css_btnex, &p_src->css_btnex);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_vscrollbar_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int h_up = w;
    int h_down = w;

    // 向上
    {
        p_vsc->p_up = klbwnd_btnex_create(p_gui, 1, 1, w - 2, h_up);
        klb_wnd_push_child(p_wnd, p_vsc->p_up);

        klbwnd_btnex_set_type(p_vsc->p_up, KLBWND_BTNEX_triangle_up);
        klb_wnd_bind_command(p_vsc->p_up, on_command_bnt_up_klbwnd_vscrollbar, p_wnd);
    }

    // 向下
    {
        p_vsc->p_down = klbwnd_btnex_create(p_gui, 1, h - h_up - 1, w - 2, h_up);
        klb_wnd_push_child(p_wnd, p_vsc->p_down);

        klbwnd_btnex_set_type(p_vsc->p_down, KLBWND_BTNEX_triangle_down);
        klb_wnd_bind_command(p_vsc->p_down, on_command_bnt_down_klbwnd_vscrollbar, p_wnd);
    }

    // 中间滑块
    {
        p_vsc->p_middle = klbwnd_btnex_create(p_gui, 1, (h - 80) / 2, w - 2, 80);
        klb_wnd_push_child(p_wnd, p_vsc->p_middle);

        klbwnd_btnex_set_type(p_vsc->p_middle, KLBWND_BTNEX_rectangle);
        klb_wnd_bind_command(p_vsc->p_middle, on_command_bnt_middle_klbwnd_vscrollbar, p_wnd);
    }
}

void klbwnd_vscrollbar_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                               // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_vscrollbar_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                            // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                                // 由继承者重写
    p_wnd->vtable.on_get = NULL;                                // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    // 初始化内部方属性
    klbwnd_vscrollbar_init_attribute(p_vsc);

    // 初始化子窗口
    klbwnd_vscrollbar_init_subwnds(p_wnd);

    // 初始化布局
    klbwnd_vscrollbar_relayout(p_wnd, p_vsc);
}

void klbwnd_vscrollbar_quit(klb_wnd_t* p_wnd)
{
    klbwnd_vscrollbar_t* p_vsc = (klbwnd_vscrollbar_t*)p_wnd->ctrl;

    klbwnd_vscrollbar_quit_attribute(p_vsc);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_vscrollbar_t));

    klbwnd_vscrollbar_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_vscrollbar_destroy;

    return p_wnd;
}
