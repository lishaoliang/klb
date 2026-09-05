// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_div.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义

static void klbwnd_div_deinit_attribute(klbwnd_div_t* p_div);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_div_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_div_deinit(p_wnd);

    KLB_FREE(p_wnd);
}

static int klbwnd_div_on_paint(klb_wnd_t* p_wnd)
{
    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 仅排版布局, 无绘图
    return 0;
}

static int klbwnd_div_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (KLBUI_onpaint == msg)
    {
        return klbwnd_div_on_paint(p_wnd);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_div_set_css(klb_wnd_t* p_wnd, klbwnd_div_css_t* p_css)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    p_div->p_css = p_css;
}

void klbwnd_div_set_index(klb_wnd_t* p_wnd, int index)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    p_div->index = index;
}

int klbwnd_div_get_index(klb_wnd_t* p_wnd)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    return p_div->index;
}

void klbwnd_div_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    p_div->title = klb_sds_assign(p_div->title, p_title);
}

const sds klbwnd_div_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    return p_div->title;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_div_init_attribute(klbwnd_div_t* p_div)
{
    p_div->index = 0;
    p_div->title = NULL;
}

static void klbwnd_div_deinit_attribute(klbwnd_div_t* p_div)
{
    KLB_FREE_BY(p_div->title, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_div_css_init(klbwnd_div_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;
}

void klbwnd_div_css_deinit(klbwnd_div_css_t* p_css)
{
}

void klbwnd_div_css_copy(klbwnd_div_css_t* p_dst, klbwnd_div_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;
}


//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_div_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;
    p_wnd->vtable.on_control = klbwnd_div_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS | KLB_WND_STYLE_NOCOMMAND;

    klbwnd_div_init_attribute(p_div);
}

void klbwnd_div_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_div_t* p_div = (klbwnd_div_t*)p_wnd->ctrl;

    klbwnd_div_deinit_attribute(p_div);
}


//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_div_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    // step1. malloc
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_div_t));

    // step2. 初始化基础部分    
    klbwnd_div_init(p_wnd, p_gui, x, y, w, h);

    // step3. 重写部分函数
    p_wnd->vtable.destroy = klbwnd_div_destroy;

    return p_wnd;
}

// end
