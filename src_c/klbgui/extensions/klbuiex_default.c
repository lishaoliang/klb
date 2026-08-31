#include "klbgui/extensions/klbuiex_default.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_color.h"
#include "klbutil/klb_map.h"


#define KLBUIEX_DEFAULT   "KLBUIEX-default"


typedef struct klbuiex_default_t_
{
    klb_gui_t*          p_gui;          ///< gui

    klbui_default_t     def;            ///< 默认值

    klb_map_t           func_map;       ///< 属性函数表
}klbuiex_default_t;


typedef void(*klbuiex_default_cb)(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out);

//////////////////////////////////////////////////////////////////////////
// 默认全局参考属性 (klbui_default_t)
//
// 控件创建前通过 klb_gui_default_css_set / klbui.default_css 可改本表.
// 控件 css_init 通常从 klb_gui_get_std_default 复制 margin/padding 与各状态参考属性.
//
// 盒模型 (无伪类):
//   margin  = 0, 0, 0, 0
//   padding = 1, 1, 1, 1
//
// 参考属性分支 (C 字段 -> CSS 伪类, 键名见 klbui_css.md):
//   normal  -> (无后缀)   常规
//   focus   -> :focus     聚焦
//   disable -> :disabled  不使能
//   check   -> :checked   选中参考 (toggle 控件)
//   input   -> :input     输入态
//
// 默认配色: Visual Studio 深色系; font-size=24; text-align=left
//   normal   text #DCDCDC  border #505050  bg #1F1F1F
//   focus    text #DCDC0A  border #DC5050  bg #1F1F1F
//   disable  text #B4B4B4  border #505050  bg #1F1F1F
//   check    text #0AD2D2  border #B450B4  bg #1F1F1F
//   input    text #B4B4B4  border #505050  bg #1F1F1F

static void klbuiex_default_init_normal(klbuicssex_attributes_t* p_normal)
{
    p_normal->text.color = KLB_ARGB8888(255, 220, 220, 220);
    p_normal->text.align = KLBUICSS_text1_left;

    p_normal->font.size = 24;

    p_normal->background.color = KLB_ARGB8888(255, 31, 31, 31);
    p_normal->background.image = NULL;

    p_normal->border.width.top = 1;
    p_normal->border.width.right = 1;
    p_normal->border.width.bottom = 1;
    p_normal->border.width.left = 1;
    p_normal->border.color.top = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.right = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.bottom = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.left = KLB_ARGB8888(255, 80, 80, 80);
}

static void klbuiex_default_init_focus(klbuicssex_attributes_t* p_normal)
{
    p_normal->text.color = KLB_ARGB8888(255, 220, 220, 10);
    p_normal->text.align = KLBUICSS_text1_left;

    p_normal->font.size = 24;

    p_normal->background.color = KLB_ARGB8888(255, 31, 31, 31);
    p_normal->background.image = NULL;

    p_normal->border.width.top = 1;
    p_normal->border.width.right = 1;
    p_normal->border.width.bottom = 1;
    p_normal->border.width.left = 1;
    p_normal->border.color.top = KLB_ARGB8888(255, 220, 80, 80);
    p_normal->border.color.right = KLB_ARGB8888(255, 220, 80, 80);
    p_normal->border.color.bottom = KLB_ARGB8888(255, 220, 80, 80);
    p_normal->border.color.left = KLB_ARGB8888(255, 220, 80, 80);
}

static void klbuiex_default_init_disable(klbuicssex_attributes_t* p_normal)
{
    p_normal->text.color = KLB_ARGB8888(255, 180, 180, 180);
    p_normal->text.align = KLBUICSS_text1_left;

    p_normal->font.size = 24;

    p_normal->background.color = KLB_ARGB8888(255, 31, 31, 31);
    p_normal->background.image = NULL;

    p_normal->border.width.top = 1;
    p_normal->border.width.right = 1;
    p_normal->border.width.bottom = 1;
    p_normal->border.width.left = 1;
    p_normal->border.color.top = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.right = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.bottom = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.left = KLB_ARGB8888(255, 80, 80, 80);
}

static void klbuiex_default_init_check(klbuicssex_attributes_t* p_normal)
{
    p_normal->text.color = KLB_ARGB8888(255, 10, 210, 210);
    p_normal->text.align = KLBUICSS_text1_left;

    p_normal->font.size = 24;

    p_normal->background.color = KLB_ARGB8888(255, 31, 31, 31);
    p_normal->background.image = NULL;

    p_normal->border.width.top = 1;
    p_normal->border.width.right = 1;
    p_normal->border.width.bottom = 1;
    p_normal->border.width.left = 1;
    p_normal->border.color.top = KLB_ARGB8888(255, 180, 80, 180);
    p_normal->border.color.right = KLB_ARGB8888(255, 180, 80, 180);
    p_normal->border.color.bottom = KLB_ARGB8888(255, 180, 80, 180);
    p_normal->border.color.left = KLB_ARGB8888(255, 180, 80, 180);
}

static void klbuiex_default_init_input(klbuicssex_attributes_t* p_normal)
{
    p_normal->text.color = KLB_ARGB8888(255, 180, 180, 180);
    p_normal->text.align = KLBUICSS_text1_left;

    p_normal->font.size = 24;

    p_normal->background.color = KLB_ARGB8888(255, 31, 31, 31);
    p_normal->background.image = NULL;

    p_normal->border.width.top = 1;
    p_normal->border.width.right = 1;
    p_normal->border.width.bottom = 1;
    p_normal->border.width.left = 1;
    p_normal->border.color.top = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.right = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.bottom = KLB_ARGB8888(255, 80, 80, 80);
    p_normal->border.color.left = KLB_ARGB8888(255, 80, 80, 80);
}

static void klbuiex_default_init(klbuiex_default_t* p_default)
{
    // margin
    p_default->def.margin.top = 0;
    p_default->def.margin.right = 0;
    p_default->def.margin.bottom = 0;
    p_default->def.margin.left = 0;

    // padding
    p_default->def.padding.top = 1;
    p_default->def.padding.right = 1;
    p_default->def.padding.bottom = 1;
    p_default->def.padding.left = 1;

    // init
    klbuiex_default_init_normal(&p_default->def.normal);
    klbuiex_default_init_focus(&p_default->def.focus);
    klbuiex_default_init_disable(&p_default->def.disable);
    klbuiex_default_init_check(&p_default->def.check);
    klbuiex_default_init_input(&p_default->def.input);
}

static void klbuiex_default_quit(klbuiex_default_t* p_default)
{
    klbuicssex_attributes_quit(&p_default->def.normal);
    klbuicssex_attributes_quit(&p_default->def.focus);
    klbuicssex_attributes_quit(&p_default->def.disable);
    klbuicssex_attributes_quit(&p_default->def.check);
    klbuicssex_attributes_quit(&p_default->def.input);
}

//////////////////////////////////////////////////////////////////////////
// CSS

static void on_klbuiex_default_margin(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_default->def.margin), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_margin_top(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_default->def.margin), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_margin_right(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_default->def.margin), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_margin_bottom(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_default->def.margin), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_margin_left(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_default->def.margin), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_padding(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_default->def.padding), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_padding_top(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_default->def.padding), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_padding_right(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_default->def.padding), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_padding_bottom(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_default->def.padding), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_padding_left(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_default->def.padding), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_color(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_default->def.normal.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_color_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_default->def.focus.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_color_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_default->def.disable.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_color_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_default->def.check.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_color_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_default->def.input.text), NULL, method, p_in, p_out);
}


static void on_klbuiex_default_text_align(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_default->def.normal.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_align_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_default->def.focus.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_align_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_default->def.disable.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_align_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_default->def.check.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_text_align_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_default->def.input.text), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_font_size(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_default->def.normal.font), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_font_size_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_default->def.focus.font), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_font_size_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_default->def.disable.font), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_font_size_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_default->def.check.font), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_font_size_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_default->def.input.font), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_color(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_default->def.normal.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_color_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_default->def.focus.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_color_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_default->def.disable.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_color_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_default->def.check.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_color_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_default->def.input.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_image(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_default->def.normal.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_image_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_default->def.focus.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_image_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_default->def.disable.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_image_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_default->def.check.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_background_image_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_default->def.input.background), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_width(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_default->def.normal.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_width_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_default->def.focus.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_width_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_default->def.disable.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_width_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_default->def.check.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_width_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_default->def.input.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_color(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_default->def.normal.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_color_focus(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_default->def.focus.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_color_disable(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_default->def.disable.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_color_check(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_default->def.check.border), NULL, method, p_in, p_out);
}

static void on_klbuiex_default_border_color_input(klbuiex_default_t* p_default, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_default->def.input.border), NULL, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 

klbui_default_t* klbuiex_default_get_value(klbuiex_default_t* p_default)
{
    return &p_default->def;
}

int klbuiex_default_css_set(klbuiex_default_t* p_default, const klb_map_t* p_map)
{
    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbuiex_default_cb func = (klbuiex_default_cb)klb_map_to_ptr(&p_default->func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_default, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

klb_map_t* klbuiex_default_css_get(klbuiex_default_t* p_default, const klb_map_t* p_map)
{
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbuiex_default_cb func = (klbuiex_default_cb)klb_map_to_ptr(&p_default->func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_default, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 

#define KLBUIEX_default_bind(KEY_, FUNC_) { klb_map_set_ptr(ptr, (KEY_), (FUNC_), p_default); }

static void klbuiex_default_init_func_map(klbuiex_default_t* p_default)
{
    klb_map_t* ptr = &p_default->func_map;

    //////////////////////////////////////////////
    // 全局参考属性 CSS 键 (property[:checked][:focus][:disabled][:input])
    // 用户文档: klbui_default_css.md

    // 外边距 margin (无伪类)
    KLBUIEX_default_bind("margin", on_klbuiex_default_margin);
    KLBUIEX_default_bind("margin-top", on_klbuiex_default_margin_top);
    KLBUIEX_default_bind("margin-right", on_klbuiex_default_margin_right);
    KLBUIEX_default_bind("margin-bottom", on_klbuiex_default_margin_bottom);
    KLBUIEX_default_bind("margin-left", on_klbuiex_default_margin_left);

    // 内边距 padding (无伪类)
    KLBUIEX_default_bind("padding", on_klbuiex_default_padding);
    KLBUIEX_default_bind("padding-top", on_klbuiex_default_padding_top);
    KLBUIEX_default_bind("padding-right", on_klbuiex_default_padding_right);
    KLBUIEX_default_bind("padding-bottom", on_klbuiex_default_padding_bottom);
    KLBUIEX_default_bind("padding-left", on_klbuiex_default_padding_left);

    // 文本颜色 color
    KLBUIEX_default_bind("color", on_klbuiex_default_text_color);
    KLBUIEX_default_bind("color:focus", on_klbuiex_default_text_color_focus);
    KLBUIEX_default_bind("color:disabled", on_klbuiex_default_text_color_disable);
    KLBUIEX_default_bind("color:checked", on_klbuiex_default_text_color_check);
    KLBUIEX_default_bind("color:input", on_klbuiex_default_text_color_input);

    // 文本对齐 text-align
    KLBUIEX_default_bind("text-align", on_klbuiex_default_text_align);
    KLBUIEX_default_bind("text-align:focus", on_klbuiex_default_text_align_focus);
    KLBUIEX_default_bind("text-align:disabled", on_klbuiex_default_text_align_disable);
    KLBUIEX_default_bind("text-align:checked", on_klbuiex_default_text_align_check);
    KLBUIEX_default_bind("text-align:input", on_klbuiex_default_text_align_input);

    // 字体大小 font-size
    KLBUIEX_default_bind("font-size", on_klbuiex_default_font_size);
    KLBUIEX_default_bind("font-size:focus", on_klbuiex_default_font_size_focus);
    KLBUIEX_default_bind("font-size:disabled", on_klbuiex_default_font_size_disable);
    KLBUIEX_default_bind("font-size:checked", on_klbuiex_default_font_size_check);
    KLBUIEX_default_bind("font-size:input", on_klbuiex_default_font_size_input);

    // 背景色 background-color
    KLBUIEX_default_bind("background-color", on_klbuiex_default_background_color);
    KLBUIEX_default_bind("background-color:focus", on_klbuiex_default_background_color_focus);
    KLBUIEX_default_bind("background-color:disabled", on_klbuiex_default_background_color_disable);
    KLBUIEX_default_bind("background-color:checked", on_klbuiex_default_background_color_check);
    KLBUIEX_default_bind("background-color:input", on_klbuiex_default_background_color_input);

    // 背景图片 background-image
    KLBUIEX_default_bind("background-image", on_klbuiex_default_background_image);
    KLBUIEX_default_bind("background-image:focus", on_klbuiex_default_background_image_focus);
    KLBUIEX_default_bind("background-image:disabled", on_klbuiex_default_background_image_disable);
    KLBUIEX_default_bind("background-image:checked", on_klbuiex_default_background_image_check);
    KLBUIEX_default_bind("background-image:input", on_klbuiex_default_background_image_input);

    // 边框的宽度 border-width
    KLBUIEX_default_bind("border-width", on_klbuiex_default_border_width);
    KLBUIEX_default_bind("border-width:focus", on_klbuiex_default_border_width_focus);
    KLBUIEX_default_bind("border-width:disabled", on_klbuiex_default_border_width_disable);
    KLBUIEX_default_bind("border-width:checked", on_klbuiex_default_border_width_check);
    KLBUIEX_default_bind("border-width:input", on_klbuiex_default_border_width_input);

    // 边框的颜色 border-color
    KLBUIEX_default_bind("border-color", on_klbuiex_default_border_color);
    KLBUIEX_default_bind("border-color:focus", on_klbuiex_default_border_color_focus);
    KLBUIEX_default_bind("border-color:disabled", on_klbuiex_default_border_color_disable);
    KLBUIEX_default_bind("border-color:checked", on_klbuiex_default_border_color_check);
    KLBUIEX_default_bind("border-color:input", on_klbuiex_default_border_color_input);

    //////////////////////////////////////////////
    // 自定义方法
}

static void* klbuiex_default_create(klb_gui_t* p_gui)
{
    klbuiex_default_t* p_default = KLB_MALLOCZ(klbuiex_default_t, 1, 0);

    p_default->p_gui = p_gui;

    klb_map_init(&p_default->func_map);
    klbuiex_default_init(p_default);
    klbuiex_default_init_func_map(p_default);

    return p_default;
}

static void klbuiex_default_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_default_t* p_default = (klbuiex_default_t*)ptr;

    klbuiex_default_quit(p_default);
    klb_map_quit(&p_default->func_map);

    KLB_FREE(p_default)
}

//////////////////////////////////////////////////////////////////////////

klbuiex_default_t* klbuiex_get_default(klb_gui_t* p_gui)
{
    return (klbuiex_default_t*)klb_gui_get_extension(p_gui, KLBUIEX_DEFAULT);
}

int klbuiex_register_default(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_default_create;
    ex.cb_destroy = klbuiex_default_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_DEFAULT, &ex);

    return 0;
}

// end

