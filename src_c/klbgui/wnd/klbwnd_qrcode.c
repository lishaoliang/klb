// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_qrcode.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"
#include "qrencode.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_qrcode_quit_attribute(klbwnd_qrcode_t* p_qrcode);
static void quit_qrcode_list_klbwnd_qrcode(klbwnd_qrcode_t* p_qrcode);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_qrcode_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    klbwnd_qrcode_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_qrcode_draw_QRcode(klb_wnd_t* p_wnd, klbwnd_qrcode_t* p_qrcode, QRcode *qrcode, klb_rect_t* p_rect)
{
    klb_rect_t rect_wnd = *p_rect;
    klb_rect_t rect_dst = { 0 };

    // 绘图颜色表
    uint32_t color[8];
    int col = 0;

    color[0] = KLB_ARGB8888(255, 255, 255, 255);
    color[1] = KLB_ARGB8888(255, 0, 0, 0);
    color[2] = KLB_ARGB8888(255, 192, 192, 255);
    color[3] = KLB_ARGB8888(255, 0, 0, 64);
    color[4] = KLB_ARGB8888(255, 255, 255, 192);
    color[5] = KLB_ARGB8888(255, 64, 64, 0);
    color[6] = KLB_ARGB8888(255, 255, 192, 192);
    color[7] = KLB_ARGB8888(255, 64, 0, 0);

    // 起始位置
    int ox = 0;
    int oy = 0;

    // 像素宽基本单元, 需求宽度为 : qrcode->width * size
    // 假设 为 21 * 21 的正方形, 每个bit位表示的图形为 size * size 的单位矩形
    int size = 4; 

    // 居中, 且将二维码最大化
    {
        int w = MIN(rect_wnd.w, rect_wnd.h);
        size = (0 < qrcode->width) ? (w / qrcode->width) : size; // 最合适的像素基本单元

        // 居中
        ox = (rect_wnd.w - size * qrcode->width) / 2;
        oy = (rect_wnd.h - size * qrcode->width) / 2;
    }

    //
    klb_rect_t rect = { 0 };

    int colorize = 0;
    int width = qrcode->width;

    unsigned char* p = qrcode->data;
    for (int y = 0; y < width; y++) {
        for (int x = 0; x < width; x++) {
            rect.x = ox + x * size;
            rect.y = oy + y * size;
            rect.w = size;
            rect.h = size;
            if (!colorize) {
                col = 0;
            }
            else {
                if (*p & 0x80) {
                    col = 6;
                }
                else if (*p & 0x02) {
                    col = 4;
                }
                else {
                    col = 2;
                }
            }
            col += (*p & 1);

            // 绘制矩形
            {
                rect_dst.x = rect_wnd.x + rect.x;
                rect_dst.y = rect_wnd.y + rect.y;
                rect_dst.w = rect.w;
                rect_dst.h = rect.h;

                klb_wnd_draw_fill_rect2(p_wnd, &rect_dst, color[col]);
            }

            p++;
        }
    }
}

static void klbwnd_qrcode_on_paint_status(klb_wnd_t* p_wnd, klbwnd_qrcode_t* p_qrcode, klbwnd_qrcode_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 纯色背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);

    // 边框
    klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);

    // 标题文本
    //klbuicssex_draw_text(p_wnd, p_qrcode->title, p_rect, &p_attr->border, &p_css->padding, &p_attr->text, &p_attr->font);

    // 绘制二维码, 只添加了一个二维码
    if (NULL != p_qrcode->p_qrcode_list && NULL != p_qrcode->p_qrcode_list->code)
    {
        klb_rect_t rect_qrcode = *p_rect;
        rect_qrcode.x += p_attr->border.width.left;
        rect_qrcode.y += p_attr->border.width.top;
        rect_qrcode.w -= (p_attr->border.width.left + p_attr->border.width.right);
        rect_qrcode.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

        klbwnd_qrcode_draw_QRcode(p_wnd, p_qrcode, p_qrcode->p_qrcode_list->code, &rect_qrcode);
    }
}

static int klbwnd_qrcode_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_qrcode_css_t* p_css = p_qrcode->p_css;

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
        klbwnd_qrcode_on_paint_status(p_wnd, p_qrcode, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_qrcode_on_paint_status(p_wnd, p_qrcode, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_qrcode_on_paint_status(p_wnd, p_qrcode, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}


static int klbwnd_qrcode_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_qrcode_on_paint(p_wnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 

static void init_qrcode_list_klbwnd_qrcode(klbwnd_qrcode_t* p_qrcode, sds txt)
{
    if (NULL != p_qrcode->p_qrcode_list)
    {
        return;
    }

    // step1. 初始化
    QRinput_Struct* p_qrstruct = QRinput_Struct_new();

    // step2. 追加"QRinput"文本
    // 这里只添加一个二维码
    {
        QRinput* p_qrinput = QRinput_new();
        QRinput_append(p_qrinput, QR_MODE_8, sdslen(txt), (const unsigned char *)txt);
        QRinput_Struct_appendInput(p_qrstruct, p_qrinput); // 添加进去后, 由"Struct"来管理
    }

    // step3. 追加结束后, 插入二维码头
    QRinput_Struct_insertStructuredAppendHeaders(p_qrstruct);

    // step4. 生成二维图像列表
    p_qrcode->p_qrcode_list = QRcode_encodeInputStructured(p_qrstruct);

    // step5. 释放
    KLB_FREE_BY(p_qrstruct, QRinput_Struct_free);
}

static void quit_qrcode_list_klbwnd_qrcode(klbwnd_qrcode_t* p_qrcode)
{
    KLB_FREE_BY(p_qrcode->p_qrcode_list, QRcode_List_free);
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_qrcode_set_css(klb_wnd_t* p_wnd, klbwnd_qrcode_css_t* p_css)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_qrcode->p_css = p_css;
}

void klbwnd_qrcode_set_title(klb_wnd_t* p_wnd, const char* p_title)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_qrcode->title = sdscpy(p_qrcode->title, p_title);
}

const sds klbwnd_qrcode_get_title(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    return p_qrcode->title;
}

void klbwnd_qrcode_set_value(klb_wnd_t* p_wnd, const char* p_value)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    bool is_change = false;
    if (0 != strcmp(p_qrcode->value, p_value))
    {
        quit_qrcode_list_klbwnd_qrcode(p_qrcode);

        is_change = true;
    }

    p_qrcode->value = sdscpy(p_qrcode->value, p_value);

    if (is_change && 0 < sdslen(p_qrcode->value))
    {
        init_qrcode_list_klbwnd_qrcode(p_qrcode, p_qrcode->value);
    }
}

const sds klbwnd_qrcode_get_value(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    return p_qrcode->value;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_qrcode_init_attribute(klb_wnd_t* p_wnd, klbwnd_qrcode_t* p_qrcode)
{
    p_qrcode->title = sdsempty();
    p_qrcode->value = sdsempty();
}

static void klbwnd_qrcode_quit_attribute(klbwnd_qrcode_t* p_qrcode)
{
    KLB_FREE_BY(p_qrcode->title, sdsfree);
    KLB_FREE_BY(p_qrcode->value, sdsfree);

    quit_qrcode_list_klbwnd_qrcode(p_qrcode);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_qrcode_css_init(klbwnd_qrcode_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal.text.align = KLBUICSS_text1_center;
    p_css->focus.text.align = KLBUICSS_text1_center;
    p_css->disable.text.align = KLBUICSS_text1_center;
}

void klbwnd_qrcode_css_quit(klbwnd_qrcode_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

//////////////////////////////////////////////////////////////////////////
// init / quit

void klbwnd_qrcode_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_qrcode_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW;

    // 初始化内部变量
    klbwnd_qrcode_init_attribute(p_wnd, p_qrcode);
}

void klbwnd_qrcode_quit(klb_wnd_t* p_wnd)
{
    klbwnd_qrcode_t* p_qrcode = (klbwnd_qrcode_t*)p_wnd->ctrl;

    klbwnd_qrcode_quit_attribute(p_qrcode);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_qrcode_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_qrcode_t));

    klbwnd_qrcode_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_qrcode_destroy;

    return p_wnd;
}
