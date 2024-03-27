// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_picture.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_picture_quit_attribute(klbwnd_picture_t* p_pic);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_picture_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    klbwnd_picture_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_picture_on_paint_status(klb_wnd_t* p_wnd, klbwnd_picture_t* p_pic, klbwnd_picture_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 图片
    if (0 < sdslen(p_pic->image))
    {
        klb_wnd_draw_image(p_wnd, p_rect, p_pic->image, NULL);
    }

	// 边框
	klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
}

static int klbwnd_picture_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_picture_css_t* p_css = p_pic->p_css;

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
        klbwnd_picture_on_paint_status(p_wnd, p_pic, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_picture_on_paint_status(p_wnd, p_pic, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_picture_on_paint_status(p_wnd, p_pic, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_picture_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_picture_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_picture_set_css(klb_wnd_t* p_wnd, klbwnd_picture_css_t* p_css)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    p_pic->p_css = p_css;
}

void klbwnd_picture_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    p_pic->title = sdscpy(p_pic->title, p_title);
}

const sds klbwnd_picture_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    return p_pic->title;
}

void klbwnd_picture_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    p_pic->value = sdscpy(p_pic->value, p_value);
}

const sds klbwnd_picture_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    return p_pic->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_picture_init_attribute(klbwnd_picture_t* p_pic)
{
    p_pic->title = sdsempty();
    p_pic->value = sdsempty();
    p_pic->image = sdsempty();
}

static void klbwnd_picture_quit_attribute(klbwnd_picture_t* p_pic)
{
    KLB_FREE_BY(p_pic->title, sdsfree);
    KLB_FREE_BY(p_pic->value, sdsfree);
    KLB_FREE_BY(p_pic->image, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_picture_css_init(klbwnd_picture_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.border.width.top = 0;
    p_css->normal.border.width.right = 0;
    p_css->normal.border.width.bottom = 0;
    p_css->normal.border.width.left = 0;

    p_css->focus.border.width.top = 0;
    p_css->focus.border.width.right = 0;
    p_css->focus.border.width.bottom = 0;
    p_css->focus.border.width.left = 0;

    p_css->disable.border.width.top = 0;
    p_css->disable.border.width.right = 0;
    p_css->disable.border.width.bottom = 0;
    p_css->disable.border.width.left = 0;

}

void klbwnd_picture_css_quit(klbwnd_picture_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

void klbwnd_picture_css_copy(klbwnd_picture_css_t* p_dst, klbwnd_picture_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_picture_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_picture_on_control;   // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_NOCOMMAND | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    // 初始化内部属性
    klbwnd_picture_init_attribute(p_pic);
}

void klbwnd_picture_quit(klb_wnd_t* p_wnd)
{
    klbwnd_picture_t* p_pic = (klbwnd_picture_t*)p_wnd->ctrl;

    klbwnd_picture_quit_attribute(p_pic);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_picture_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_picture_t));

    klbwnd_picture_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_picture_destroy;

    return p_wnd;
}
