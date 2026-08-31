// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_view.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_view_deinit_attribute(klbwnd_view_t* p_view);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_view_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    klbwnd_view_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_view_on_paint_status(klb_wnd_t* p_wnd, klbwnd_view_t* p_view, klbwnd_view_css_t* p_css, klbwnd_view_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景: color / image / scale9 / color_key
    klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);

    // 有背景图时图内自带边框; 纯色才画 border
    if (NULL == p_attr->background.image || 0 == sdslen(p_attr->background.image))
    {
        klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);
    }
}

static int klbwnd_view_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_view_css_t* p_css = p_view->p_css;

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

    klbwnd_view_on_paint_status(p_wnd, p_view, p_css, &p_css->normal, &paint_rect);

    return 0;
}


static int klbwnd_view_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_view_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_view_set_css(klb_wnd_t* p_wnd, klbwnd_view_css_t* p_css)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    p_view->p_css = p_css;
}

void klbwnd_view_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    p_view->index = index;
}

int klbwnd_view_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    return p_view->index;
}

void klbwnd_view_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    p_view->title = klb_sds_assign(p_view->title, p_title);
}

const sds klbwnd_view_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    return p_view->title;
}

//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_view_init_attribute(klbwnd_view_t* p_view)
{
    p_view->index = 0;
    p_view->title = NULL;
}

static void klbwnd_view_deinit_attribute(klbwnd_view_t* p_view)
{
    KLB_FREE_BY(p_view->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_view_attributes_init(klbwnd_view_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_view_attributes_deinit(klbwnd_view_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_view_attributes_copy(klbwnd_view_attributes_t* p_dst, const klbwnd_view_attributes_t* p_src)
{
    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_view_css_init(klbwnd_view_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_view_attributes_init(&p_css->normal, &p_default->normal);
}

void klbwnd_view_css_deinit(klbwnd_view_css_t* p_css)
{
    klbwnd_view_attributes_deinit(&p_css->normal);
}

void klbwnd_view_css_copy(klbwnd_view_css_t* p_dst, klbwnd_view_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_view_attributes_copy(&p_dst->normal, &p_src->normal);
}

//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_view_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                       // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_view_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                    // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                        // 由继承者重写
    p_wnd->vtable.on_get = NULL;                        // 由继承者重写

    p_wnd->p_gui = p_gui;

    // Fixed Bug. [2026] 部分页面可能采取 区域有叠加的方式来完成, 此时页面样式必须为 不可聚焦(KLB_WND_STYLE_NOFOCUS)
    // 这里默认成 不可以聚焦样式(KLB_WND_STYLE_NOFOCUS)
    // 若特定页面有需求可以 自行定义样式属性: ['style-nofocus'] = false,

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    // 初始化内部属性
    klbwnd_view_init_attribute(p_view);
}

void klbwnd_view_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_view_t* p_view = (klbwnd_view_t*)p_wnd->ctrl;

    klbwnd_view_deinit_attribute(p_view);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_view_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_view_t));

    // step2. 初始化基础部分    
    klbwnd_view_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbwnd_view_destroy;

    return p_wnd;
}

// end
