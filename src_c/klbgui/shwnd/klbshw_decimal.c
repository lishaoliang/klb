// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/shwnd/klbshw_decimal.h"
#include "klbgui/wnd/klbwnd_btnex.h"
#include "klbgui/subviews/klbwnd_btnex_paint.h"
#include "klbgui/wnd/klbwnd_static.h"
#include "klbutil/klb_color.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "klbgui/klb_gui.h"


#define KLBSHW_DECIMAL_row          4       ///< 行数
#define KLBSHW_DECIMAL_column       3       ///< 列数


/// @struct klbshw_decimal_css_t
/// @brief  10进制输入软键盘的CSS
typedef struct klbshw_decimal_css_t_
{
    klbuicss_margin_t       margin;         ///< 外边距
    klbuicss_padding_t      padding;        ///< 内边距

    klbuicssex_attributes_t normal;         ///< normal 常规状态参数

    klbwnd_static_css_t     sta;            ///< 静态框样式
    klbwnd_btnex_css_t      btnex;          ///< 按钮样式
}klbshw_decimal_css_t;


/// @struct klbshw_decimal_t
/// @brief  10进制输入软键盘
typedef struct klbshw_decimal_t_
{
    klbshw_decimal_css_t    css;            ///< 样式

    sds                     value;          ///< 当前值
    int                     max_len;        ///< 最大长度

    // 子控件
    struct
    {
        klb_wnd_t*          p_static;       ///< 静态文本
        klb_wnd_t*          p_btns[KLBSHW_DECIMAL_row][KLBSHW_DECIMAL_column];  ///< 按钮
    };

    // 绑定结束之后的回调函数
    struct
    {
        klbshw_decimal_cb   cb;
        void*               ptr;
    };
}klbshw_decimal_t;

//////////////////////////////////////////////////////////////////////////

typedef struct klbshw_decimal_title_t_
{
    const char* p_title;
    int         type;
}klbshw_decimal_title_t;


static klbshw_decimal_title_t g_klbshw_decimal_title[KLBSHW_DECIMAL_row][KLBSHW_DECIMAL_column] = {
    { { "7", 0 },{ "8", 0 },{ "9", 0 } },
    { { "4", 0 },{ "5", 0 },{ "6", 0 } },
    { { "1", 0 },{ "2", 0 },{ "3", 0 } },
    { { "0", 0 },{ "C", KLBWND_BTNEX_SHW_DECIMAL_clear },{ "E", KLBWND_BTNEX_SHW_DECIMAL_enter } },
};

//////////////////////////////////////////////////////////////////////////
// 前置定义

static klb_wnd_t* klbshw_decimal_create(klb_gui_t* p_gui, int x, int y, int w, int h);
static void klbshw_decimal_quit_attribute(klbshw_decimal_t* p_dec);
static void klbshw_decimal_popup_end(klb_wnd_t* p_wnd_menu, klbshw_decimal_t* p_dec, bool ok);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbshw_decimal_destroy(klb_wnd_t* p_wnd)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    klbshw_decimal_quit_attribute(p_dec);

    KLB_FREE(p_wnd);
}

static void klbshw_decimal_on_paint_status(klb_wnd_t* p_wnd, klbshw_decimal_t* p_date, klbshw_decimal_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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

static int klbshw_decimal_on_paint(klb_wnd_t* p_wnd)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbshw_decimal_css_t* p_css = &p_dec->css;

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    // 移除外边距
    paint_rect.x += p_css->margin.left;
    paint_rect.y += p_css->margin.top;
    paint_rect.w -= (p_css->margin.left + p_css->margin.right);
    paint_rect.h -= (p_css->margin.top + p_css->margin.bottom);

    klbshw_decimal_on_paint_status(p_wnd, p_dec, p_css, &p_css->normal, &paint_rect);

    return 0;
}

static int klbshw_decimal_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbshw_decimal_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;
            if (!klb_pt_in_rect(p_rect, p_pt1->x, p_pt1->y))
            {
                klbshw_decimal_popup_end(p_wnd, p_dec, false);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}
//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法




//////////////////////////////////////////////////////////////////////////
// 内部私有函数

// 结束
static void klbshw_decimal_popup_end(klb_wnd_t* p_wnd, klbshw_decimal_t* p_dec, bool ok)
{
    if (p_dec->cb)
    {
        int value = atoi(p_dec->value);

        p_dec->cb(p_dec->ptr, p_wnd, ok, value);
    }

    // 结束弹出
    klb_gui_popup_end(p_wnd->p_gui, true);
}

// 追加
static void klbshw_decimal_append(klb_wnd_t* p_wnd_dec, klbshw_decimal_t* p_dec, const char* p_str)
{
    if (sdslen(p_dec->value) < p_dec->max_len)
    {
        // 追加
        p_dec->value = sdscat(p_dec->value, p_str);

        int value = atoi(p_dec->value);
        sdsclear(p_dec->value);
        p_dec->value = sdscatfmt(p_dec->value, "%i", value);

        // 刷新
        klbwnd_static_set_title(p_dec->p_static, p_dec->value);
        klb_wnd_update(p_dec->p_static);
    }
}

// 清空
static void klbshw_decimal_clear(klb_wnd_t* p_wnd_dec, klbshw_decimal_t* p_dec)
{
    // 清空
    sdsclear(p_dec->value);
    p_dec->value = sdscatfmt(p_dec->value, "%i", 0);

    // 刷新
    klbwnd_static_set_title(p_dec->p_static, p_dec->value);
    klb_wnd_update(p_dec->p_static);
}

// 常规按钮 [0,9]
static int on_click_btn_normal_klbshw_decimal(klb_wnd_t* p_wnd_btn, klb_wnd_t* p_wnd_dec, klbshw_decimal_t* p_dec)
{
    const sds value = klbwnd_btnex_get_value(p_wnd_btn);

    if (0 < sdslen(value))
    {
        klbshw_decimal_append(p_wnd_dec, p_dec, value);
    }

    return 0;
}

// 清空
static int on_click_btn_clear_klbshw_decimal(klb_wnd_t* p_wnd_btn, klb_wnd_t* p_wnd_dec, klbshw_decimal_t* p_dec)
{
    klbshw_decimal_clear(p_wnd_dec, p_dec);

    return 0;
}

// Enter
static int on_click_btn_enter_klbshw_decimal(klb_wnd_t* p_wnd_btn, klb_wnd_t* p_wnd_dec, klbshw_decimal_t* p_dec)
{
    klbshw_decimal_popup_end(p_wnd_dec, p_dec, true);

    return 0;
}

// 按钮响应
static int on_command_btn_klbshw_decimal(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_cal = p_wnd->p_udata;
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd_cal->ctrl;

    switch (e)
    {
    case KLBUI_click:
    case KLBUI_dblclick:
        {
            int t = klbwnd_btnex_get_type(p_wnd);
            if (KLBWND_BTNEX_SHW_DECIMAL_clear == t)
            {
                return on_click_btn_clear_klbshw_decimal(p_wnd, p_wnd_cal, p_dec);
            }
            else if(KLBWND_BTNEX_SHW_DECIMAL_enter == t)
            {
                return on_click_btn_enter_klbshw_decimal(p_wnd, p_wnd_cal, p_dec);
            }
            else
            {
                return on_click_btn_normal_klbshw_decimal(p_wnd, p_wnd_cal, p_dec);
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

/// @brief 重新布局
static void klbshw_decimal_relayout(klb_wnd_t* p_wnd)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    int offx = 2;
    int offy = 2;

    int item_w = w / KLBSHW_DECIMAL_column - offx;
    int item_h = h / (KLBSHW_DECIMAL_row + 1) - offy;

    int left_x = (w - (item_w * KLBSHW_DECIMAL_column + offx * (KLBSHW_DECIMAL_column - 1))) / 2;
    int sx = left_x;
    int sy = (h - (item_h * (KLBSHW_DECIMAL_row + 1) + offy * ((KLBSHW_DECIMAL_row + 1) - 1))) / 2;

    // 静态文本
    klb_wnd_move(p_dec->p_static, 1, 1);
    klb_wnd_resize(p_dec->p_static, (w - 2), item_h + 1 + offy);

    // 按钮
    sx = left_x;
    sy += item_h + offy;

    for (int m = 0; m < KLBSHW_DECIMAL_row; m++)
    {
        for (int n = 0; n < KLBSHW_DECIMAL_column; n++)
        {
            klb_wnd_t* p_btn = p_dec->p_btns[m][n];

            klb_wnd_move(p_btn, sx, sy);
            klb_wnd_resize(p_btn, item_w, item_h);

            sx += item_w + offx;
        }

        sx = left_x;
        sy += item_h + offy;
    }
}

static int klbshw_decimal_init_subwnd(klb_wnd_t* p_wnd, klbshw_decimal_t* p_dec, klb_gui_t* p_gui)
{
    // 静态文本框
    p_dec->p_static = klbwnd_static_create(p_gui, 0, 0, 64, 32);
    klb_wnd_push_child(p_wnd, p_dec->p_static);
    klbwnd_static_set_css(p_dec->p_static, &p_dec->css.sta);
    klbwnd_static_set_title(p_dec->p_static, "0");

    // 按钮
    for (int m = 0; m < KLBSHW_DECIMAL_row; m++)
    {
        for (int n = 0; n < KLBSHW_DECIMAL_column; n++)
        {
            klb_wnd_t* p_btn = klbwnd_btnex_create(p_gui, 0, 0, 32, 32);
            klb_wnd_push_child(p_wnd, p_btn);

            klbwnd_btnex_set_type(p_btn, g_klbshw_decimal_title[m][n].type);
            klbwnd_btnex_set_title(p_btn, g_klbshw_decimal_title[m][n].p_title);
            klbwnd_btnex_set_value(p_btn, g_klbshw_decimal_title[m][n].p_title);

            klbwnd_btnex_set_css(p_btn, &p_dec->css.btnex);
            klb_wnd_bind_command(p_btn, on_command_btn_klbshw_decimal, p_wnd);

            p_dec->p_btns[m][n] = p_btn;
        }
    }

    // 重新布局
    klbshw_decimal_relayout(p_wnd);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出

/// @brief 获取10进制输入软键盘
klb_wnd_t* klbui_shwnd_get_decimal(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_decimal);

    if (NULL == p_wnd)
    {
        int w = 0, h = 0;
        klbshw_decimal_wh(p_gui, &w, &h);

        p_wnd = klbshw_decimal_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_decimal, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

/// @brief 获取所需要的宽高
int klbshw_decimal_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{
    if (p_out_w) { *p_out_w = 136; }
    if (p_out_h) { *p_out_h = 218; }

    return 0;
}

int klbshw_decimal_bind(klb_wnd_t* p_wnd, klbshw_decimal_cb cb, void* ptr)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    p_dec->cb = cb;
    p_dec->ptr = ptr;

    return 0;
}

void klbshw_decimal_set_max_len(klb_wnd_t* p_wnd, int max_len)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    p_dec->max_len = max_len;
}

void klbshw_decimal_set_value(klb_wnd_t* p_wnd, int value)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    sdsclear(p_dec->value);
    p_dec->value = sdscatfmt(p_dec->value, "%i", value);
    klbwnd_static_set_title(p_dec->p_static, p_dec->value);
}

int klbshw_decimal_get_value(klb_wnd_t* p_wnd)
{
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    return atoi(p_dec->value);
}


//////////////////////////////////////////////////////////////////////////
// init attribute 初始化属性默认值

static void klbshw_decimal_init_attribute(klb_wnd_t* p_wnd, klbshw_decimal_t* p_dec)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);
    klb_gui_t* p_gui = p_wnd->p_gui;

    p_dec->value = sdsempty();

    p_dec->css.margin = p_default->margin;
    p_dec->css.padding = p_default->padding;

    klbuicssex_attributes_init(&p_dec->css.normal, &p_default->normal);

    klbwnd_static_css_init(&p_dec->css.sta, p_gui);
    klbwnd_btnex_css_init(&p_dec->css.btnex, p_gui);

    p_dec->css.sta.normal.background.color = KLB_ARGB8888(255, 42, 42, 42);
    p_dec->css.sta.normal.text.align = KLBUICSS_text1_right;
    p_dec->css.sta.padding.left = 4;
    p_dec->css.sta.padding.right = 8;


    p_dec->css.btnex.normal.text.align = KLBUICSS_text1_center;
    p_dec->css.btnex.focus.text.align = KLBUICSS_text1_center;
    p_dec->css.btnex.disable.text.align = KLBUICSS_text1_center;

    p_dec->max_len = 6;
}

static void klbshw_decimal_quit_attribute(klbshw_decimal_t* p_dec)
{
    KLB_FREE_BY(p_dec->value, sdsfree);

    klbuicssex_attributes_quit(&p_dec->css.normal);
    klbwnd_static_css_quit(&p_dec->css.sta);
    klbwnd_btnex_css_quit(&p_dec->css.btnex);
}

//////////////////////////////////////////////////////////////////////////
// create

static klb_wnd_t* klbshw_decimal_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_decimal_t));
    klbshw_decimal_t* p_dec = (klbshw_decimal_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbshw_decimal_destroy;
    p_wnd->vtable.on_control = klbshw_decimal_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    // css
    klbshw_decimal_init_attribute(p_wnd, p_dec);

    // 子窗口
    klbshw_decimal_init_subwnd(p_wnd, p_dec, p_gui);

    return p_wnd;
}
