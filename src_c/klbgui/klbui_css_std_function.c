// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/klb_gui.h"
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
        int type = klb_map_array_type(p_in, start);
        switch (type)
        {
        case KLB_ADT_bool:
            {
                klb_wnd_disable(p_wnd, klb_map_idx_to_bool(p_in, start));
                klb_wnd_update(p_wnd);
            }
            break;
        default:
            break;
        }
    }
}

// tip
static void on_klbguicssmapstd_tip(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_tip(p_wnd, method, p_in, p_out);
}

static void on_klbguicssmapstd_visibility(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
{
    klbuicssex_visibility(p_wnd, method, p_in, p_out);
}

//////////////////////////////////////////////////////////////////////////
// 控件通用 css 属性方法

#define KLBGUI_cssmapstd_bind(KEY_, FUNC_) { klb_map_set_ptr(p_css_map, (KEY_), (void*)(FUNC_), ptr); }

void klb_gui_css_map_append_std_function(klb_map_t* p_css_map, void* ptr)
{
    // 注意: 所有子控件的响应函数定义类似如下:
    // void on_xxx(klb_wnd_t* p_wnd, void* ptr, int method, const klb_map_t* p_in, klb_map_t* p_out)
    // 其中 void* ptr 为具体控件指针, 公共方法中无法处理此指针
    // 公共方法, 可以处理 所有与 klb_wnd_t* 相关的部分
    // 公共方法不强制使用: 若需要, 在 klb_gui_new_css_map 之后调用本函数即可

    // 窗口坐标
    KLBGUI_cssmapstd_bind("wndpos-canvas", on_klbguicssmapstd_wndpos_canvas); // 基于窗口画布坐标
    KLBGUI_cssmapstd_bind("wndpos-parent", on_klbguicssmapstd_wndpos_parent); // 基于父窗口坐标


    // 窗口移动
    KLBGUI_cssmapstd_bind("move", on_klbguicssmapstd_move);


    // 重设窗口大小
    KLBGUI_cssmapstd_bind("resize", on_klbguicssmapstd_resize);


    // 控件建议宽度/高度
    KLBGUI_cssmapstd_bind("suggestw", on_klbguicssmapstd_suggestw);
    KLBGUI_cssmapstd_bind("suggesth", on_klbguicssmapstd_suggesth);


    // 状态
    KLBGUI_cssmapstd_bind("disable", on_klbguicssmapstd_disable);       // 不使能


    // tip
    KLBGUI_cssmapstd_bind("tip", on_klbguicssmapstd_tip);

    // 显示/隐藏
    KLBGUI_cssmapstd_bind("visibility", on_klbguicssmapstd_visibility);

}
