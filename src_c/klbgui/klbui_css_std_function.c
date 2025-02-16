// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include <string.h>
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// 控件通用 css 属性方法

// 基于窗口画布坐标
static void on_klbguicssmapstd_wndpos_canvas(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t* p_pos = klb_map_create();
        klb_map_set_int64(p_pos, "x", p_wnd->pos.rect_in_canvas.x);
        klb_map_set_int64(p_pos, "y", p_wnd->pos.rect_in_canvas.y);
        klb_map_set_int64(p_pos, "w", p_wnd->pos.rect_in_canvas.w);
        klb_map_set_int64(p_pos, "h", p_wnd->pos.rect_in_canvas.h);

        klb_map_set_idx_map(p_out, 0, p_pos);
    }
}


// 基于父窗口坐标
static void on_klbguicssmapstd_wndpos_parent(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_t* p_pos = klb_map_create();
        klb_map_set_int64(p_pos, "x", p_wnd->pos.rect_in_parent.x);
        klb_map_set_int64(p_pos, "y", p_wnd->pos.rect_in_parent.y);
        klb_map_set_int64(p_pos, "w", p_wnd->pos.rect_in_parent.w);
        klb_map_set_int64(p_pos, "h", p_wnd->pos.rect_in_parent.h);

        klb_map_set_idx_map(p_out, 0, p_pos);
    }
}

// 窗口移动
static void on_klbguicssmapstd_move(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb_map_t* p_pos = klb_map_idx_to_map((klb_map_t*)p_in, start);

        if (NULL != p_pos)
        {
            int x = (int)klb_map_to_int64(p_pos, "x");
            int y = (int)klb_map_to_int64(p_pos, "y");

            klb_wnd_move(p_wnd, x, y);
            klb_wnd_update(p_wnd);
        }
    }
}

// 重设窗口大小
static void on_klbguicssmapstd_resize(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        klb_map_t* p_pos = klb_map_idx_to_map((klb_map_t*)p_in, start);

        if (NULL != p_pos)
        {
            int w = (int)klb_map_to_int64(p_pos, "w");
            int h = (int)klb_map_to_int64(p_pos, "h");

            klb_wnd_resize(p_wnd, w, h);
            klb_wnd_update(p_wnd);
        }
    }
}

// 控件建议宽度
static void on_klbguicssmapstd_suggestw(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klb_wnd_suggestw(p_wnd));
    }
}

// 控件建议高度
static void on_klbguicssmapstd_suggesth(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_int64(p_out, 0, klb_wnd_suggesth(p_wnd));
    }
}

// 样式: 单个样式, eg. KLB_WND_STYLE_PEEK_EVENT
static void on_klbguicssmapstd_style_std(uint32_t dst_style, klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        uint32_t style = klb_wnd_get_style(p_wnd);

        klb_map_set_idx_bool(p_out, 0, (dst_style & style) ? true : false);
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool has = klb_map_idx_to_bool(p_in, start);

        uint32_t style = klb_wnd_get_style(p_wnd);
        if (has)
        {
            style |= dst_style;
        }
        else
        {
            style &= ~(uint32_t)(dst_style);
        }

        klb_wnd_set_style(p_wnd, style);
    }
}

// 样式: (在消息冒泡中)读取消息事件: KLB_WND_STYLE_PEEK_EVENT
static void on_klbguicssmapstd_style_peek_event(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_PEEK_EVENT, p_wnd, ptr, method, p_in, p_out);
}

// 样式: 设置 无聚焦状态 样式: KLB_WND_STYLE_NOFOCUS
static void on_klbguicssmapstd_style_nofocus(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_NOFOCUS, p_wnd, ptr, method, p_in, p_out);
}

// 样式: 设置 无on_command命令响应样式: KLB_WND_STYLE_NOCOMMAND
static void on_klbguicssmapstd_style_nocommand(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_NOCOMMAND, p_wnd, ptr, method, p_in, p_out);
}

// 样式: 设置 聚焦时不重绘: KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW
static void on_klbguicssmapstd_style_focus_without_redraw(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW, p_wnd, ptr, method, p_in, p_out);
}

// 样式: 设置 继续寻找焦点窗口 样式 : KLB_WND_STYLE_FOCUS_CONTINUE
static void on_klbguicssmapstd_style_focus_continue(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_FOCUS_CONTINUE, p_wnd, ptr, method, p_in, p_out);
}

// 样式: 设置 支持聚焦之后, 延时消息 样式: KLB_WND_STYLE_FOCUS_DELAY
static void on_klbguicssmapstd_style_focus_delay(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    on_klbguicssmapstd_style_std(KLB_WND_STYLE_FOCUS_DELAY, p_wnd, ptr, method, p_in, p_out);
}

// 显示
static void on_klbguicssmapstd_show(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_show(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool show = klb_map_idx_to_bool(p_in, start);

        klb_wnd_show(p_wnd, show);
    }
}

// 隐藏
static void on_klbguicssmapstd_hide(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_hide(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool hide = klb_map_idx_to_bool(p_in, start);

        klb_wnd_hide(p_wnd, hide);
        klb_wnd_update(p_wnd);
    }
}

// 输入状态
static void on_klbguicssmapstd_input(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_input(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool input = klb_map_idx_to_bool(p_in, start);

        klb_wnd_input(p_wnd, input);
        klb_wnd_update(p_wnd);
    }
}

// 选中
static void on_klbguicssmapstd_check(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_check(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool check = klb_map_idx_to_bool(p_in, start);

        klb_wnd_check(p_wnd, check);
        klb_wnd_update(p_wnd);
    }
}

// 不使能
static void on_klbguicssmapstd_disable(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_disable(p_wnd));
    }
    else if(KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool disable = klb_map_idx_to_bool(p_in, start);

        klb_wnd_disable(p_wnd, disable);
        klb_wnd_update(p_wnd);
    }
}

// 使能
static void on_klbguicssmapstd_enable(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_enable(p_wnd));
    }
    else if (KLBUI_CSSEX_set == method)
    {
        int start = 1;
        bool enable = klb_map_idx_to_bool(p_in, start);

        klb_wnd_enable(p_wnd, enable);
        klb_wnd_update(p_wnd);
    }
}

// 所有窗口中的最顶层
static void on_klbguicssmapstd_topmost(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    if (KLBUI_CSSEX_get == method)
    {
        klb_map_set_idx_bool(p_out, 0, klb_wnd_is_topmost(p_wnd));
    }
}

// tip
static void on_klbguicssmapstd_tip(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_tip(p_wnd, method, p_in, p_out);
}

// 显示/隐藏
static void on_klbguicssmapstd_visibility(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

static void do_first_control_event_klbguicssmapstd(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (NULL == p_wnd)
    {
        return;
    }

    // on control
    klb_wnd_call_control(p_wnd, e, p_pt1, p_pt2, lparam, wparam);

    // 
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        do_first_control_event_klbguicssmapstd(p_next, e, p_pt1, p_pt2, lparam, wparam);

        p_next = p_next->p_next;
    }
}

// 重新布局
static void on_klbguicssmapstd_layout(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    // 先调用自身的 KLBUI_layout
    // 再依次调用 子窗口的 KLBUI_layout

    do_first_control_event_klbguicssmapstd(p_wnd, KLBUI_layout, NULL, NULL, 0, 0);

    klb_wnd_update(p_wnd);
}

// 刷新
static void on_klbguicssmapstd_refresh(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klb_wnd_update(p_wnd);
}

//////////////////////////////////////////////////////////////////////////
// 控件通用 css 属性方法

#define KLBGUI_cssmapstd_bind(KEY_, FUNC_) { klb_map_set_ptr(p_css_map, (KEY_), (void*)(FUNC_), NULL); }

void klb_gui_css_map_append_std_function(klb_map_t* p_css_map, void* ptr)
{
    // 注意: 所有子控件的响应函数定义类似如下:
    // static void on_xxx(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
    // 其中 void* ptr 为具体控件指针, 公共方法中无法处理此指针
    // 公共方法, 可以处理 所有与 klb_wnd_t* 相关的部分

    // 窗口坐标
    KLBGUI_cssmapstd_bind("wndpos-canvas", on_klbguicssmapstd_wndpos_canvas); // 基于窗口画布坐标
    KLBGUI_cssmapstd_bind("wndpos-parent", on_klbguicssmapstd_wndpos_parent); // 基于父窗口坐标

    KLBGUI_cssmapstd_bind("wndpos_canvas", on_klbguicssmapstd_wndpos_canvas); // 基于窗口画布坐标
    KLBGUI_cssmapstd_bind("wndpos_parent", on_klbguicssmapstd_wndpos_parent); // 基于父窗口坐标

    // 窗口移动
    KLBGUI_cssmapstd_bind("move", on_klbguicssmapstd_move);             // 相对父窗口,移动(x,y); eg. jq('aaa').move({x=10,y=10})


    // 重设窗口大小
    KLBGUI_cssmapstd_bind("resize", on_klbguicssmapstd_resize);         // 重设窗口大小(w,h); eg. jq('aaa').resize({w=120,h=32})


    // 控件建议宽度/高度
    KLBGUI_cssmapstd_bind("suggestw", on_klbguicssmapstd_suggestw);     // 控件建议宽度
    KLBGUI_cssmapstd_bind("suggesth", on_klbguicssmapstd_suggesth);     // 控件建议高度


    // 样式 style
    KLBGUI_cssmapstd_bind("style-peek-event", on_klbguicssmapstd_style_peek_event);                         // (在消息冒泡中)读取消息事件: KLB_WND_STYLE_PEEK_EVENT
    KLBGUI_cssmapstd_bind("style-nofocus", on_klbguicssmapstd_style_nofocus);                               // 设置 无聚焦状态 样式: KLB_WND_STYLE_NOFOCUS
    KLBGUI_cssmapstd_bind("style-nocommand", on_klbguicssmapstd_style_nocommand);                           // 设置 无on_command命令响应样式: KLB_WND_STYLE_NOCOMMAND
    KLBGUI_cssmapstd_bind("style-focus-without-redraw", on_klbguicssmapstd_style_focus_without_redraw);     // 设置 聚焦时不重绘: KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW
    KLBGUI_cssmapstd_bind("style-focus-continue", on_klbguicssmapstd_style_focus_continue);                 // 设置 继续寻找焦点窗口 样式: KLB_WND_STYLE_FOCUS_CONTINUE
    KLBGUI_cssmapstd_bind("style-focus-delay", on_klbguicssmapstd_style_focus_delay);                       // 设置 支持聚焦之后, 延时消息 样式: KLB_WND_STYLE_FOCUS_DELAY

    KLBGUI_cssmapstd_bind("style_peek_event", on_klbguicssmapstd_style_peek_event);                         // (在消息冒泡中)读取消息事件: KLB_WND_STYLE_PEEK_EVENT
    KLBGUI_cssmapstd_bind("style_nofocus", on_klbguicssmapstd_style_nofocus);                               // 设置 无聚焦状态 样式: KLB_WND_STYLE_NOFOCUS
    KLBGUI_cssmapstd_bind("style_nocommand", on_klbguicssmapstd_style_nocommand);                           // 设置 无on_command命令响应样式: KLB_WND_STYLE_NOCOMMAND
    KLBGUI_cssmapstd_bind("style_focus_without_redraw", on_klbguicssmapstd_style_focus_without_redraw);     // 设置 聚焦时不重绘: KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW
    KLBGUI_cssmapstd_bind("style_focus_continue", on_klbguicssmapstd_style_focus_continue);                 // 设置 继续寻找焦点窗口 样式: KLB_WND_STYLE_FOCUS_CONTINUE
    KLBGUI_cssmapstd_bind("style_focus_delay", on_klbguicssmapstd_style_focus_delay);                       // 设置 支持聚焦之后, 延时消息 样式: KLB_WND_STYLE_FOCUS_DELAY

    // 状态 status
    KLBGUI_cssmapstd_bind("show", on_klbguicssmapstd_show);             // 显示(true/false)
    KLBGUI_cssmapstd_bind("hide", on_klbguicssmapstd_hide);             // 隐藏(true/false)
    KLBGUI_cssmapstd_bind("input", on_klbguicssmapstd_input);           // 输入(true/false)
    KLBGUI_cssmapstd_bind("check", on_klbguicssmapstd_check);           // 选中(true/false)
    KLBGUI_cssmapstd_bind("disable", on_klbguicssmapstd_disable);       // 不使能(true/false)
    KLBGUI_cssmapstd_bind("enable", on_klbguicssmapstd_enable);         // 使能(true/false)
    KLBGUI_cssmapstd_bind("topmost", on_klbguicssmapstd_topmost);       // 所有最顶层窗口中的视觉最上层的那个


    // tip
    KLBGUI_cssmapstd_bind("tip", on_klbguicssmapstd_tip);               // 设置tip


    // 显示/隐藏
    KLBGUI_cssmapstd_bind("visibility", on_klbguicssmapstd_visibility); // 显示/隐藏


    // 重新布局
    KLBGUI_cssmapstd_bind("layout", on_klbguicssmapstd_layout);         // 重新布局


    // 刷新/重绘
    KLBGUI_cssmapstd_bind("refresh", on_klbguicssmapstd_refresh);       // 刷新/重绘
}
