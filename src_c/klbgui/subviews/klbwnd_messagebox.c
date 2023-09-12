// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_messagebox.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_messagebox_quit_attribute(klbwnd_messagebox_t* p_msgbox);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_messagebox_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_messagebox_on_paint_status(klb_wnd_t* p_wnd, klbwnd_messagebox_t* p_msgbox, klbwnd_messagebox_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 31, 31, 31));

    // 标题栏区域
    int title_h = 42;
    klb_rect_t title_rect = { p_rect->x, p_rect->y, p_rect->w, title_h };
    {
        // 标题栏背景
        klb_wnd_draw_fill_rect2(p_wnd, &title_rect, KLB_ARGB8888(255, 45, 45, 48));

        // 标题栏下边线
        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y + title_h, p_rect->x + p_rect->w - 1, p_rect->y + title_h, KLB_ARGB8888(255, 120, 120, 120));
    }

    // 边框
    {
        // 外边框
        klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 81, 81, 81));

        // 上边框
        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y, p_rect->x + p_rect->w - 1, p_rect->y, KLB_ARGB8888(255, 180, 180, 180));
        
        // 左边框
        klb_wnd_draw_line2(p_wnd, p_rect->x, p_rect->y, p_rect->x, p_rect->y + p_rect->h - 1, KLB_ARGB8888(255, 180, 180, 180));
    }

    // 标题栏文本
    {
        title_rect.x += 2;
        title_rect.y += 2;
        title_rect.w -= 4;
        title_rect.h -= 4;

        klbuicssex_draw_text(p_wnd, p_msgbox->title, &title_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);
    }
}

static int klbwnd_messagebox_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_messagebox_css_t* p_css = p_msgbox->p_css;

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
    //    klbwnd_messagebox_on_paint_status(p_wnd, p_msgbox, p_css, &p_css->disable, &paint_rect);
    //}
    //else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    //{
    //    klbwnd_messagebox_on_paint_status(p_wnd, p_msgbox, p_css, &p_css->focus, &paint_rect);
    //}
    //else
    {
        klbwnd_messagebox_on_paint_status(p_wnd, p_msgbox, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_messagebox_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_messagebox_on_paint(p_wnd);
        break;

    case KLBUI_outwindow:
        //klb_gui_messagebox_end(p_wnd->p_gui);
        break;

    default:
        break;
    }

    return 0;
}

/////////////////////////////////////////////
// 子窗口响应函数

static int on_btnex_close_klbwnd_messagebox(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_msgbox = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd_msgbox->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klb_gui_messagebox_end(p_wnd->p_gui);
    }

    return 0;
}

static int on_btn_ok_klbwnd_messagebox(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_msgbox = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd_msgbox->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klb_gui_messagebox_end(p_wnd->p_gui);
    }

    return 0;
}

static int on_btn_cancel_klbwnd_messagebox(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_msgbox = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd_msgbox->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        klb_gui_messagebox_end(p_wnd->p_gui);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_messagebox_set_css(klb_wnd_t* p_wnd, klbwnd_messagebox_css_t* p_css)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_msgbox->p_css = p_css;

    if (NULL != p_css)
    {
        klbwnd_btnex_set_css(p_msgbox->p_btnex_close, &p_css->css_btnex);
        klbwnd_button_set_css(p_msgbox->p_btn_ok, &p_css->css_btn);
        klbwnd_button_set_css(p_msgbox->p_btn_cancel, &p_css->css_btn);

        klbwnd_static_set_css(p_msgbox->p_sta_txt, &p_css->css_sta);
        klbwnd_picture_set_css(p_msgbox->p_pic_image, &p_css->css_pic);
    }
}

void klbwnd_messagebox_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_msgbox->title = sdscpy(p_msgbox->title, p_title);
}

const sds klbwnd_messagebox_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    return p_msgbox->title;
}

void klbwnd_messagebox_set_body_image(klb_wnd_t* p_wnd, const char* p_image)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
}

void klbwnd_messagebox_set_body_text(klb_wnd_t* p_wnd, const char* p_text)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klbwnd_static_set_title(p_msgbox->p_sta_txt, p_text);
}

int klbwnd_messagebox_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    return p_msgbox->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_messagebox_init_attribute(klb_wnd_t* p_wnd, klbwnd_messagebox_t* p_msgbox)
{
    p_msgbox->title = sdsnew("Message Box");
    p_msgbox->value = KLBWND_MESSAGEBOX_ok;
}

static void klbwnd_messagebox_quit_attribute(klbwnd_messagebox_t* p_msgbox)
{
    KLB_FREE_BY(p_msgbox->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_messagebox_css_init(klbwnd_messagebox_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    klbwnd_button_css_init(&p_css->css_btn, p_gui);
    klbwnd_btnex_css_init(&p_css->css_btnex, p_gui);
    klbwnd_static_css_init(&p_css->css_sta, p_gui);
    klbwnd_picture_css_init(&p_css->css_pic, p_gui);

    p_css->css_btnex.padding.top = 6;
    p_css->css_btnex.padding.bottom = 6;
    p_css->css_btnex.normal.border.width.top = 0;
    p_css->css_btnex.normal.border.width.right = 0;
    p_css->css_btnex.normal.border.width.bottom = 0;
    p_css->css_btnex.normal.border.width.left = 0;

    p_css->css_btnex.focus.border.width = p_css->css_btnex.normal.border.width;

    p_css->css_btnex.normal.background.color = KLB_ARGB8888(255, 45, 45, 48);
    p_css->css_btnex.focus.background.color = KLB_ARGB8888(255, 60, 60, 60);
}

void klbwnd_messagebox_css_quit(klbwnd_messagebox_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);

    klbwnd_button_css_quit(&p_css->css_btn);
    klbwnd_btnex_css_quit(&p_css->css_btnex);
    klbwnd_static_css_quit(&p_css->css_sta);
    klbwnd_picture_css_quit(&p_css->css_pic);
}

//////////////////////////////////////////////////////////////////////////
// 子窗口

// 初始化子窗口
static void klbwnd_messagebox_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;
    klbwnd_messagebox_css_t* p_css = p_msgbox->p_css;

    int btn_w = 120;
    int btn_h = 32;

    int pic_w = 40;
    int pic_h = 40;

    // button close
    {
        p_msgbox->p_btnex_close = klbwnd_btnex_create(p_gui, 0, 0, 36, 28);

        klb_wnd_push_child(p_wnd, p_msgbox->p_btnex_close);
        klb_wnd_bind_command(p_msgbox->p_btnex_close, on_btnex_close_klbwnd_messagebox, p_wnd);

        klbwnd_btnex_set_type(p_msgbox->p_btnex_close, KLBWND_BTNEX_line_x);
    }

    // button ok
    {
        p_msgbox->p_btn_ok = klbwnd_button_create(p_gui, 0, 0, btn_w, btn_h);

        klbwnd_button_set_title(p_msgbox->p_btn_ok, "Ok");
        
        klb_wnd_push_child(p_wnd, p_msgbox->p_btn_ok);
        klb_wnd_bind_command(p_msgbox->p_btn_ok, on_btn_ok_klbwnd_messagebox, p_wnd);
    }

    // button cancel
    {
        p_msgbox->p_btn_cancel = klbwnd_button_create(p_gui, 0, 0, btn_w, btn_h);

        klbwnd_button_set_title(p_msgbox->p_btn_cancel, "Cancel");

        klb_wnd_push_child(p_wnd, p_msgbox->p_btn_cancel);
        klb_wnd_bind_command(p_msgbox->p_btn_cancel, on_btn_cancel_klbwnd_messagebox, p_wnd);
    }

    //  文本框
    {
        p_msgbox->p_sta_txt = klbwnd_static_create(p_gui, 0, 0, btn_w, btn_h);
        klb_wnd_push_child(p_wnd, p_msgbox->p_sta_txt);
    }

    // 图片框
    {
        p_msgbox->p_pic_image = klbwnd_picture_create(p_gui, 0, 0, pic_w, pic_h);
        klb_wnd_push_child(p_wnd, p_msgbox->p_pic_image);

        klb_wnd_hide(p_msgbox->p_pic_image, true);
    }

    // css
    if (NULL != p_css)
    {
        klbwnd_button_set_css(p_msgbox->p_btn_ok, &p_css->css_btn);
        klbwnd_button_set_css(p_msgbox->p_btn_cancel, &p_css->css_btn);

        klbwnd_static_set_css(p_msgbox->p_sta_txt, &p_css->css_sta);
        klbwnd_picture_set_css(p_msgbox->p_pic_image, &p_css->css_pic);
    }
}

// 重新布局
static void klbwnd_messagebox_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klb_rect_t rect = p_wnd->pos.rect_in_parent;
    int w = rect.w;
    int h = rect.h;

    int title_h = 42;

    int btn_w = 120;
    int btn_h = 32;

    int pic_w = 40;
    int pic_h = 40;

    // 右上角 关闭按钮
    {
        int x = w - p_msgbox->p_btnex_close->pos.rect_in_parent.w - 10;
        int y = (title_h - p_msgbox->p_btnex_close->pos.rect_in_parent.h) / 2;

        klb_wnd_move(p_msgbox->p_btnex_close, x, y);
    }

    // 图片框
    //{
    //    int pic_x = 5;
    //    int pic_y = (h - title_h - pic_h - 10) / 2;

    //    klb_wnd_move(p_msgbox->p_pic_image, pic_x, pic_y); // 图片框
    //}

    // 静态文本框
    {
        int sta_w = w - 10;
        int sta_h = 32;
        int sta_x = 5;
        int sta_y = (h - title_h - sta_h - 10) / 2;

        klb_wnd_resize(p_msgbox->p_sta_txt, sta_w, sta_h);
        klb_wnd_move(p_msgbox->p_sta_txt, sta_x, sta_y);
    }

    // 右下角按钮
    {
        int x = w - btn_w - 10;
        int y = h - btn_h - 10;
        klb_wnd_move(p_msgbox->p_btn_cancel, x, y); // button cancel

        x -= (btn_w + 6);
        klb_wnd_move(p_msgbox->p_btn_ok, x, y); // button ok
    }
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_messagebox_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_messagebox_on_control;// 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    // 初始化内部变量
    klbwnd_messagebox_init_attribute(p_wnd, p_msgbox);

    // 初始化子窗口
    klbwnd_messagebox_init_subwnds(p_wnd);

    // 重新布局
    klbwnd_messagebox_relayout(p_wnd);
}

void klbwnd_messagebox_quit(klb_wnd_t* p_wnd)
{
    klbwnd_messagebox_t* p_msgbox = (klbwnd_messagebox_t*)p_wnd->ctrl;

    klbwnd_messagebox_quit_attribute(p_msgbox);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_messagebox_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_messagebox_t));

    klbwnd_messagebox_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_messagebox_destroy;

    return p_wnd;
}
