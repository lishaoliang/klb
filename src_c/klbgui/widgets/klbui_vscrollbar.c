// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_widgets.h"
#include "klbgui/wnd/klbwnd_vscrollbar.h"
#include "klbmem/klb_mem.h"


/// @struct klbui_vscrollbar_t
/// @brief  垂直滚动条
typedef struct klbui_vscrollbar_t_
{
    klbwnd_vscrollbar_t     vscrollbar;     ///< 必须首位, 保持内存一致
    klbwnd_vscrollbar_css_t css;            ///< 样式

    klb_map_t*              p_func_map;     ///< CSS属性函数表
}klbui_vscrollbar_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
typedef void(*klbui_vscrollbar_cb)(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// 继承/重写方法

static void klbui_vscrollbar_destroy(klb_wnd_t* p_wnd)
{
    klbui_vscrollbar_t* p_vsc = (klbui_vscrollbar_t*)p_wnd->ctrl;

    // 退出基础部分
    klbwnd_vscrollbar_quit(p_wnd);

    // 退出css
    klbwnd_vscrollbar_css_quit(&p_vsc->css);

    KLB_FREE(p_wnd);
}

static int klbui_vscrollbar_on_set(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_vscrollbar_t* p_vsc = (klbui_vscrollbar_t*)p_wnd->ctrl;

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_vscrollbar_cb func = (klbui_vscrollbar_cb)klb_map_to_ptr(p_vsc->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_vsc, KLBUI_CSSEX_set, p_map, NULL);
    }

    return 0;
}

static klb_map_t* klbui_vscrollbar_on_get(klb_wnd_t* p_wnd, const klb_map_t* p_map)
{
    klbui_vscrollbar_t* p_vsc = (klbui_vscrollbar_t*)p_wnd->ctrl;
    klb_map_t* p_out = klb_map_create();

    const char* p_key = klb_map_idx_to_string(p_map, 0);
    klbui_vscrollbar_cb func = (klbui_vscrollbar_cb)klb_map_to_ptr(p_vsc->p_func_map, p_key, NULL);

    if (NULL != func)
    {
        func(p_wnd, p_vsc, KLBUI_CSSEX_get, p_map, p_out);
    }

    return p_out;
}

//////////////////////////////////////////////////////////////////////////
// 仿 CSS 方法

static void on_klbui_vscrollbar_visibility(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_margin(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin(&(p_vsc->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_margin_top(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_top(&(p_vsc->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_margin_right(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_right(&(p_vsc->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_margin_bottom(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_bottom(&(p_vsc->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_margin_left(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_margin_left(&(p_vsc->css.margin), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_padding(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding(&(p_vsc->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_padding_top(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_top(&(p_vsc->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_padding_right(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_right(&(p_vsc->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_padding_bottom(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_bottom(&(p_vsc->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_padding_left(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_padding_left(&(p_vsc->css.padding), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_color(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_vsc->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_color_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_vsc->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_color_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_color(&(p_vsc->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_align(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_vsc->css.normal.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_align_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_vsc->css.focus.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_text_align_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_text_align(&(p_vsc->css.disable.text), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_font_size(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_vsc->css.normal.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_font_size_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_vsc->css.focus.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_font_size_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_font_size(&(p_vsc->css.disable.font), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_color(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_vsc->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_color_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_vsc->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_color_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_color(&(p_vsc->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_image(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_vsc->css.normal.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_image_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_vsc->css.focus.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_background_image_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_background_image(&(p_vsc->css.disable.background), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_width(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_vsc->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_width_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_vsc->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_width_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_width(&(p_vsc->css.disable.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_color(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_vsc->css.normal.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_color_focus(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_vsc->css.focus.border), p_wnd, method, p_in, p_out);
}

static void on_klbui_vscrollbar_border_color_disable(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_border_color(&(p_vsc->css.disable.border), p_wnd, method, p_in, p_out);
}

//////////////////////////////////////
// 自定义属性

static void on_klbui_vscrollbar_value(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
	if (KLBUI_CSSEX_get == method)
	{
		klb_map_set_idx_int64(p_out, 0, klbwnd_vscrollbar_get_value(p_wnd));
	}
	else if (KLBUI_CSSEX_set == method)
	{
		int start = 1;
		int type = klb_map_array_type(p_in, start);
		switch (type)
		{
		case KLB_ADT_int64:
			{
				int v = (int)klb_map_idx_to_int64(p_in, start);
				klbwnd_vscrollbar_set_value(p_wnd, v);
				//klb_wnd_update(p_wnd);
			}
			break;
		case KLB_ADT_uint64:
			{
				int v = (int)klb_map_idx_to_uint64(p_in, start);
				klbwnd_vscrollbar_set_value(p_wnd, v);
				//klb_wnd_update(p_wnd);
			}
			break;
		default:
			break;
		}
	}
}

static void on_klbui_vscrollbar_ranges(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
	if (KLBUI_CSSEX_set == method)
	{
		int start = 1;
		int t = klb_map_array_type(p_in, start);
		if (KLB_ADT_map == t)
		{
			klb_map_t* p_in_hms = klb_map_idx_to_map(p_in, start);

			int min = 0, max = 0, step = 0;
			if (0 < klb_map_array_size(p_in_hms))
			{
				min = (int)klb_map_idx_to_int64(p_in_hms, 0);
				max = (int)klb_map_idx_to_int64(p_in_hms, 0);
				step = (int)klb_map_idx_to_int64(p_in_hms, 0);
			}
			else
			{
				min = (int)klb_map_to_int64(p_in_hms, "min");
				max = (int)klb_map_to_int64(p_in_hms, "max");
				step = (int)klb_map_to_int64(p_in_hms, "step");
			}

			klbwnd_vscrollbar_set_ranges(p_wnd, min, max, step);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// css func

#define KLBUI_vscrollbar_bind(KEY_, FUNC_) { klb_map_set_ptr(p_vsc->p_func_map, (KEY_), (void*)(FUNC_), p_vsc); }

static void klbui_vscrollbar_init_func_map(klb_wnd_t* p_wnd, klbui_vscrollbar_t* p_vsc, klb_gui_t* p_gui)
{
    klb_map_t* ptr = klb_gui_css_map(p_gui, KLBUI_kvscrollbar);
    if (NULL != ptr)
    {
        p_vsc->p_func_map = ptr;
        return;
    }

    // 未找到, 则新添加 解析map, 及处理函数
    ptr = klb_gui_new_css_map(p_gui, KLBUI_kvscrollbar);
    p_vsc->p_func_map = ptr;


    //////////////////////////////////////////////
    // 若第一次, 则添加全局属性解析方法
    // 仿 CSS 方法

    // 显隐
    KLBUI_vscrollbar_bind("visibility", on_klbui_vscrollbar_visibility); // 显示/隐藏

    // 外边距 margin
    KLBUI_vscrollbar_bind("margin", on_klbui_vscrollbar_margin);
    KLBUI_vscrollbar_bind("margin-top", on_klbui_vscrollbar_margin_top);
    KLBUI_vscrollbar_bind("margin-right", on_klbui_vscrollbar_margin_right);
    KLBUI_vscrollbar_bind("margin-bottom", on_klbui_vscrollbar_margin_bottom);
    KLBUI_vscrollbar_bind("margin-left", on_klbui_vscrollbar_margin_left);

    // 内边距 padding
    KLBUI_vscrollbar_bind("padding", on_klbui_vscrollbar_padding);
    KLBUI_vscrollbar_bind("padding-top", on_klbui_vscrollbar_padding_top);
    KLBUI_vscrollbar_bind("padding-right", on_klbui_vscrollbar_padding_right);
    KLBUI_vscrollbar_bind("padding-bottom", on_klbui_vscrollbar_padding_bottom);
    KLBUI_vscrollbar_bind("padding-left", on_klbui_vscrollbar_padding_left);

    // 文本颜色 color
    KLBUI_vscrollbar_bind("color", on_klbui_vscrollbar_text_color);
    KLBUI_vscrollbar_bind("color:focus", on_klbui_vscrollbar_text_color_focus);
    KLBUI_vscrollbar_bind("color:disable", on_klbui_vscrollbar_text_color_disable);

    // 文本对齐 text-align
    KLBUI_vscrollbar_bind("text-align", on_klbui_vscrollbar_text_align);
    KLBUI_vscrollbar_bind("text-align:focus", on_klbui_vscrollbar_text_align_focus);
    KLBUI_vscrollbar_bind("text-align:disable", on_klbui_vscrollbar_text_align_disable);

    // 字体大小 font-size
    KLBUI_vscrollbar_bind("font-size", on_klbui_vscrollbar_font_size);
    KLBUI_vscrollbar_bind("font-size:focus", on_klbui_vscrollbar_font_size_focus);
    KLBUI_vscrollbar_bind("font-size:disable", on_klbui_vscrollbar_font_size_disable);

    // 背景色 background-color
    KLBUI_vscrollbar_bind("background-color", on_klbui_vscrollbar_background_color);
    KLBUI_vscrollbar_bind("background-color:focus", on_klbui_vscrollbar_background_color_focus);
    KLBUI_vscrollbar_bind("background-color:disable", on_klbui_vscrollbar_background_color_disable);

    // 背景图片 background-image
    KLBUI_vscrollbar_bind("background-image", on_klbui_vscrollbar_background_image);
    KLBUI_vscrollbar_bind("background-image:focus", on_klbui_vscrollbar_background_image_focus);
    KLBUI_vscrollbar_bind("background-image:disable", on_klbui_vscrollbar_background_image_disable);

    // 边框的宽度 border-width
    KLBUI_vscrollbar_bind("border-width", on_klbui_vscrollbar_border_width);
    KLBUI_vscrollbar_bind("border-width:focus", on_klbui_vscrollbar_border_width_focus);
    KLBUI_vscrollbar_bind("border-width:disable", on_klbui_vscrollbar_border_width_disable);

    // 边框的颜色 border-color
    KLBUI_vscrollbar_bind("border-color", on_klbui_vscrollbar_border_color);
    KLBUI_vscrollbar_bind("border-color:focus", on_klbui_vscrollbar_border_color_focus);
    KLBUI_vscrollbar_bind("border-color:disable", on_klbui_vscrollbar_border_color_disable);

    //////////////////////////////////////////////
    // 自定义方法

    KLBUI_vscrollbar_bind("value", on_klbui_vscrollbar_value);
	KLBUI_vscrollbar_bind("ranges", on_klbui_vscrollbar_ranges);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbui_vscrollbar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbui_vscrollbar_t));
    klbui_vscrollbar_t* p_vsc = (klbui_vscrollbar_t*)p_wnd->ctrl;

    // 初始化基础部分
    klbwnd_vscrollbar_init(p_wnd, p_gui, x, y, w, h);

    // 重写部分函数
    p_wnd->vtable.destroy = klbui_vscrollbar_destroy;
    p_wnd->vtable.on_set = klbui_vscrollbar_on_set;
    p_wnd->vtable.on_get = klbui_vscrollbar_on_get;

    // css
    klbwnd_vscrollbar_css_init(&p_vsc->css, p_gui);
    klbwnd_vscrollbar_set_css(p_wnd, &p_vsc->css);

    // 初始化CSS 支持的方法
    klbui_vscrollbar_init_func_map(p_wnd, p_vsc, p_gui);

    return p_wnd;
}
