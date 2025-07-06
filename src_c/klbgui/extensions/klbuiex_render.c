// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_render.h"
#include "klbmem/klb_mem.h"
#include "klbgui/klb_gui_in.h"
#include "klbgui/klb_wnd_in.h"
#include "klbutil/klb_canvas.h"


/// @def   KLBUIEX_RENDER
/// @brief 注册的内部扩展名称
#define KLBUIEX_RENDER          "KLBUIEX-render"


/// @struct klbuiex_render_t
/// @brief  GUI图形渲染模块
///  1. UI窗口图形绘制
typedef struct klbuiex_render_t_
{
    klb_gui_t*      p_gui;

    bool            is_multi_layer;                         ///< 是否 使用多图层

    klb_canvas_t*   p_popup_canvas[KLBUI_POPUP_WND_MAX];    ///< popup 图层画布[is_multi_layer=true]
    klb_canvas_t*   p_msgbox_canvas;                        ///< msgbox 图层画布[is_multi_layer=true]
}klbuiex_render_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义


//////////////////////////////////////////////////////////////////////////
// 适配 

static void* klbuiex_render_create(klb_gui_t* p_gui)
{
    klbuiex_render_t* p_render = KLB_MALLOCZ(klbuiex_render_t, 1, 0);

    p_render->p_gui = p_gui;

    return p_render;
}

static void klbuiex_render_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_render_t* p_render = (klbuiex_render_t*)ptr;

    KLB_FREE(p_render)
}

//////////////////////////////////////////////////////////////////////////
// 主画布 + TIP画布

// 重绘所有
static void redraw_all_klbuiex_render(klb_gui_t* p_gui, klb_rect_t* p_rect)
{
    if (NULL == p_gui->p_canvas)
    {
        return;
    }

    klb_canvas_set_draw_color(p_gui->p_canvas, KLB_ARGB8888(0, 0, 0, 0));
    klb_canvas_draw_clear(p_gui->p_canvas);

    for (int i = 0; i < p_gui->modal_num; i++)
    {
        klb_wnd_draw(p_gui->p_modal_wnd[i]);
    }

    for (int i = 0; i < p_gui->popup_num; i++)
    {
        klb_wnd_draw(p_gui->p_popup_wnd[i]);
    }

    if (NULL != p_gui->p_msg_box)
    {
        klb_wnd_draw(p_gui->p_msg_box);
    }

    //全部区域
    *p_rect = p_gui->p_canvas->rect;
}

// 检查窗口 是否在 model/popup/messagebox 中
static bool is_in_model_popup_msgbox_klbuiex_render(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 判定其顶层窗口即可
    // 若不在其中, 则不需要绘制
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    // messagebox
    if (p_top == p_gui->p_msg_box)
    {
        return true;
    }

    // popup
    for (int m = p_gui->popup_num - 1; 0 <= m; m--)
    {
        if (p_top == p_gui->p_popup_wnd[m])
        {
            return true;
        }
    }

    // modal
    for (int n = p_gui->modal_num - 1; 0 <= n; n--)
    {
        if (p_top == p_gui->p_modal_wnd[n])
        {
            return true;
        }
    }

    return false;
}


// 按列表重绘
static void redraw_wnd_hlist_klbuiex_render(klb_gui_t* p_gui, klb_hlist_t* p_hlist, klb_rect_t* p_rect)
{
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_hlist);

    bool first = true;

    // 依次绘制所有窗口
    while (NULL != p_iter)
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_data(p_iter);

        // Bug. 这里还有一种情况: 放入刷新列表的窗口, 根本不在 所有需要绘制的窗口链中
        // 在这里排除: 检查其顶层窗口, 是否在 model/popup/messagebox 中
        if (is_in_model_popup_msgbox_klbuiex_render(p_gui, p_wnd))
        {
            klb_wnd_draw(p_wnd);

            if (first)
            {
                *p_rect = p_wnd->pos.rect_in_canvas;
                first = false;
            }
            else
            {
                klb_rect_t rect1 = *p_rect;
                klb_rect_union(p_rect, &rect1, &p_wnd->pos.rect_in_canvas); // 需要刷新区域, 取并集
            }
        }

        p_iter = klb_hlist_next(p_iter);
    }
}

// 重绘,刷新
static int redraw_and_refresh_klbuiex_render(klb_gui_t* p_gui)
{
    // step1. 处理主画布绘制
    bool is_refresh_main = false;
    klb_rect_t rect_main = { 0 };

    // 检查主画布的刷新
    bool is_redraw_all = false;
    if (klbuiex_redraw_need_repaint(p_gui->p_redraw, &is_redraw_all))
    {
        klb_rect_t rect = { 0 };

        if (is_redraw_all)
        {
            redraw_all_klbuiex_render(p_gui, &rect);
        }
        else
        {
            redraw_wnd_hlist_klbuiex_render(p_gui, klbuiex_redraw_get_hlist(p_gui->p_redraw), &rect);
        }

        // 绘制完成, 清空 redraw 标记
        klbuiex_redraw_clear(p_gui->p_redraw);

        // 与主画布做交集, 并检查是否需要刷新主画布
        if (NULL != p_gui->p_canvas && klb_rect_intersect(&rect_main, &rect, &p_gui->p_canvas->rect) && 0 < rect_main.w && 0 < rect_main.h)
        {
            is_refresh_main = true;
        }
    }

    // step2. 处理tip图层绘制
    bool is_redraw_tip = false;
    if (klbuiex_tip_need_repaint(p_gui->p_tip))
    {
        // 重新绘制 tip
        klbuiex_tip_redraw(p_gui->p_tip);

        is_redraw_tip = true;
    }

    bool is_refresh_tip = false;
    klb_rect_t rect_tip_dst = { 0 };
    klb_rect_t rect_tip_src = { 0 };
    klb_canvas_t* p_tip_canvas = NULL;

    bool is_show_tip = false;
    p_tip_canvas = klbuiex_tip_get_refresh(p_gui->p_tip, &is_show_tip, &rect_tip_dst, &rect_tip_src);

    if (is_show_tip &&
        (NULL != p_tip_canvas) &&
        (is_redraw_tip || is_refresh_main))
    {
        is_refresh_tip = true;
    }

    // step3. 恢复上次的tip区域
    bool is_refresh_old = false;
    klb_rect_t rect_old = { 0 };

    if (klbuiex_tip_get_old(p_gui->p_tip, &rect_old))
    {
        if (is_refresh_main || is_redraw_tip || is_refresh_tip)
        {
            is_refresh_old = true;
            klbuiex_tip_set_old_refresh(p_gui->p_tip, false);
        }
    }

    // step4. 最终汇总刷新参数
    int layer_count = 0, idx = 0;
    klb_rect_t dst[KLB_CANVAS_LAYER_max];
    klb_rect_t src[KLB_CANVAS_LAYER_max];
    klb_canvas_t* p_tmp_canvas[KLB_CANVAS_LAYER_max];

    {
        if (is_refresh_main)
        {
            dst[idx] = rect_main;
            src[idx] = rect_main;
            p_tmp_canvas[idx] = p_gui->p_canvas;

            idx += 1;
        }

        if (is_refresh_old)
        {
            dst[idx] = rect_old;
            src[idx] = rect_old;
            p_tmp_canvas[idx] = p_gui->p_canvas;

            idx += 1;
        }

        if (is_refresh_tip)
        {
            dst[idx] = rect_tip_dst;
            src[idx] = rect_tip_src;
            p_tmp_canvas[idx] = p_tip_canvas;

            klbuiex_tip_set_old(p_gui->p_tip, &rect_tip_dst);
            klbuiex_tip_set_old_refresh(p_gui->p_tip, true);

            idx += 1;
        }

        layer_count = idx;
    }

    // step4. 汇总刷新
    if (0 < layer_count && NULL != p_gui->p_canvas)
    {
        // 刷新画布到"显存"; 注意: 刷到"显存"必须使用主画布
        klb_canvas_refresh(p_gui->p_canvas, dst, p_tmp_canvas, src, layer_count);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 多画布
// 

// 添加图层画布
static void add_canvas_layer_klbuiex_render(klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int* p_layer_count, int layer_type, bool use, bool redraw, klb_rect_t* p_rect, klb_canvas_t* p_canvas)
{
    int idx = *p_layer_count;
    if (idx + 1 <= KLB_CANVAS_LAYER_max)
    {
        klb_rect_t rect_zero = { 0 };

        layers[idx].layer_type = layer_type;
        layers[idx].is_used = use;
        layers[idx].is_redraw = redraw;
        layers[idx].redraw_rect = (NULL != p_rect) ? *p_rect : rect_zero;
        layers[idx].p_canvas = p_canvas;

        *p_layer_count = idx + 1;
    }
}

// 初始化图层 次序
static void init_canvas_layer_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int* p_layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    // 初始值
    *p_layer_count = 0;

    // KLB_CANVAS_LAYER_main 图层
    add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_main, (0 < p_gui->modal_num) ? true : false, false, NULL, p_gui->p_canvas);

    // KLB_CANVAS_LAYER_popup 图层
    for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
    {
        add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_popup, (i < p_gui->popup_num) ? true : false, false, NULL, p_render->p_popup_canvas[i]);
    }

    // KLB_CANVAS_LAYER_msgbox 图层
    add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_msgbox, (NULL != p_gui->p_msg_box) ? true : false , false, NULL, p_render->p_msgbox_canvas);

    // KLB_CANVAS_LAYER_tip 图层
    add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_tip, (p_gui->p_tip->is_show) ? true : false, false, NULL, p_gui->p_tip->p_canvas);
}

// 放图层
static void push_canvas_layer_klbuiex_render(klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count, int layer_type, bool redraw, const klb_rect_t* p_rect, const klb_canvas_t* p_canvas)
{
    // 查找图层, 并更新
    for (int i = 0; i < layer_count; i++)
    {
        if (layers[i].layer_type == layer_type && layers[i].p_canvas == p_canvas)
        {
            // 若已经放入, 则更新
            if (redraw)
            {
                // 更新刷新区域
                if (!layers[i].is_redraw)
                {
                    // 首次使用
                    layers[i].redraw_rect = *p_rect;
                }
                else
                {
                    klb_rect_t rect = { 0 };
                    klb_rect_union(&rect, &layers[i].redraw_rect, p_rect);

                    layers[i].redraw_rect = rect;
                }
            }

            // 更新使用情况
            layers[i].is_redraw = redraw;

            return;
        }
    }
}

// 是否在 model 窗口中
static bool is_in_model_klbuiex_render(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 判定其顶层窗口即可
    // 若不在其中, 则不需要绘制
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    // modal
    for (int n = p_gui->modal_num - 1; 0 <= n; n--)
    {
        if (p_top == p_gui->p_modal_wnd[n])
        {
            return true;
        }
    }

    return false;
}

// 是否在 popup 窗口中
static bool is_in_popup_klbuiex_render(klb_gui_t* p_gui, klb_wnd_t* p_wnd, int* p_idx)
{
    // 判定其顶层窗口即可
    // 若不在其中, 则不需要绘制
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    // popup
    for (int m = p_gui->popup_num - 1; 0 <= m; m--)
    {
        if (p_top == p_gui->p_popup_wnd[m])
        {
            *p_idx = m;
            return true;
        }
    }

    return false;
}

// 是否在 msgbox 窗口中
static bool is_in_msgbox_klbuiex_render(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 判定其顶层窗口即可
    // 若不在其中, 则不需要绘制
    klb_wnd_t* p_top = klb_wnd_get_top(p_wnd);

    // messagebox
    if (p_top == p_gui->p_msg_box)
    {
        return true;
    }

    return false;
}

static bool muti_redraw_tip_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    bool is_need_refresh = false;

    // step2. 处理tip图层绘制
    bool is_redraw_tip = false;
    if (klbuiex_tip_need_repaint(p_gui->p_tip))
    {
        // 重新绘制 tip
        klbuiex_tip_redraw(p_gui->p_tip);

        is_redraw_tip = true;
    }

    bool is_refresh_tip = false;
    klb_rect_t rect_tip_dst = { 0 };
    klb_rect_t rect_tip_src = { 0 };
    klb_canvas_t* p_tip_canvas = NULL;

    bool is_show_tip = false;
    p_tip_canvas = klbuiex_tip_get_refresh(p_gui->p_tip, &is_show_tip, &rect_tip_dst, &rect_tip_src);

    if (is_show_tip &&
        (NULL != p_tip_canvas) &&
        (is_redraw_tip))
    {
        is_refresh_tip = true;
    }

    // step3. 恢复上次的tip区域
    bool is_refresh_old = false;
    klb_rect_t rect_old = { 0 };

    if (klbuiex_tip_get_old(p_gui->p_tip, &rect_old))
    {
        if (is_redraw_tip || is_refresh_tip)
        {
            is_refresh_old = true;
            klbuiex_tip_set_old_refresh(p_gui->p_tip, false);
        }
    }

    if (is_refresh_old)
    {
        push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &rect_old, p_gui->p_canvas);
        is_need_refresh = true;
    }

    if (is_refresh_tip)
    {
        push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_tip, true, &rect_tip_dst, p_tip_canvas);
        is_need_refresh = true;
    }

    return is_need_refresh;
}

// 按列表重绘
static void multi_redraw_wnd_hlist_klbuiex_render(klbuiex_render_t* p_render, klb_hlist_t* p_hlist, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    // 依次绘制所有窗口
    // 注意: 放入列表的窗口可能是无序的, 但图层绘制必须是有序的
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_hlist);
    while (NULL != p_iter)
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_data(p_iter);
        int index = 0;

        if (is_in_model_klbuiex_render(p_gui, p_wnd))
        {
            // 绘图
            klb_wnd_draw(p_wnd);

            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &p_wnd->pos.rect_in_canvas, p_gui->p_canvas);
        }
        else if(is_in_popup_klbuiex_render(p_gui, p_wnd, &index))
        {
            // 绘图
            klb_wnd_draw(p_wnd);

            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_popup, true, &p_wnd->pos.rect_in_canvas, p_render->p_popup_canvas[index]);
        }
        else if(is_in_msgbox_klbuiex_render(p_gui, p_wnd))
        {
            klb_wnd_draw(p_wnd);

            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_msgbox, true, &p_wnd->pos.rect_in_canvas, p_render->p_msgbox_canvas);
        }

        p_iter = klb_hlist_next(p_iter);
    }
}

// 重绘所有
static void multi_redraw_all_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    if (NULL == p_gui->p_canvas)
    {
        return;
    }

    klb_canvas_set_draw_color(p_gui->p_canvas, KLB_ARGB8888(0, 0, 0, 0));
    klb_canvas_draw_clear(p_gui->p_canvas);

    for (int i = 0; i < p_gui->modal_num; i++)
    {
        klb_wnd_draw(p_gui->p_modal_wnd[i]);
    }

    push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &p_gui->p_canvas->rect, p_gui->p_canvas);

    for (int i = 0; i < p_gui->popup_num; i++)
    {
        if (NULL != p_gui->p_popup_wnd[i])
        {
            klb_canvas_set_draw_color(p_render->p_popup_canvas[i], KLB_ARGB8888(0, 0, 0, 0));
            klb_canvas_draw_clear(p_render->p_popup_canvas[i]);

            klb_wnd_draw(p_gui->p_popup_wnd[i]);

            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_popup, true, &p_render->p_popup_canvas[i]->rect, p_render->p_popup_canvas[i]);
        }
    }

    if (NULL != p_gui->p_msg_box)
    {
        klb_canvas_set_draw_color(p_render->p_msgbox_canvas, KLB_ARGB8888(0, 0, 0, 0));
        klb_canvas_draw_clear(p_render->p_msgbox_canvas);

        klb_wnd_draw(p_gui->p_msg_box);

        push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_msgbox, true, &p_render->p_msgbox_canvas->rect, p_render->p_msgbox_canvas);
    }
}

// 重绘,刷新
static int multi_redraw_and_refresh_klbuiex_render(klbuiex_render_t* p_render)
{
    klb_gui_t* p_gui = p_render->p_gui;

    klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max] = { 0 };
    int layer_count = 0;
    init_canvas_layer_klbuiex_render(p_render, layers, &layer_count);

    // 检查主画布的刷新
    bool is_redraw_all = false;
    bool is_refresh = klbuiex_redraw_need_repaint(p_gui->p_redraw, &is_redraw_all);
    if (is_refresh)
    {
        if (is_redraw_all)
        {
            multi_redraw_all_klbuiex_render(p_render, layers, layer_count);
        }
        else
        {
            multi_redraw_wnd_hlist_klbuiex_render(p_render, klbuiex_redraw_get_hlist(p_gui->p_redraw), layers, layer_count);
        }   

        // 绘制完成, 清空 redraw 标记
        klbuiex_redraw_clear(p_gui->p_redraw);
    }

    bool is_refresh_tip = false;
    if (is_refresh && is_redraw_all)
    {
        is_refresh_tip = true;

        klb_rect_t rect_tip_dst = { 0 };
        klb_rect_t rect_tip_src = { 0 };
        klb_canvas_t* p_tip_canvas = NULL;

        bool is_show_tip = false;
        p_tip_canvas = klbuiex_tip_get_refresh(p_gui->p_tip, &is_show_tip, &rect_tip_dst, &rect_tip_src);

        if (is_show_tip)
        {
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_tip, true, &rect_tip_dst, p_tip_canvas);
        }
    }
    else
    {
        // TIP
        is_refresh_tip = muti_redraw_tip_klbuiex_render(p_render, layers, layer_count);
    }

    // 刷新
    if (NULL != p_gui->p_canvas && (is_refresh || is_refresh_tip))
    {
        int refresh_opt = is_redraw_all ? KLB_CANVAS_REFRESH_copy : KLB_CANVAS_REFRESH_copy_bubble;
        klb_canvas_refresh_layer(p_gui->p_canvas, refresh_opt, layers, layer_count);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 对外接口

void klbuiex_render_try_attach_canvas(klbuiex_render_t* p_render, const klb_canvas_t* p_main_canvas)
{
    if (NULL != p_main_canvas)
    {
        // 主画布存在, 则从主画布接口函数中申请画布

        // 需要所有画布均 申请成功, 才认为是多画布模式
        bool is_multi = true;

        for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
        {
            assert(NULL == p_render->p_popup_canvas[i]);

            p_render->p_popup_canvas[i] = klb_canvas_malloc((klb_canvas_t*)p_main_canvas, i, 0, KLB_CANVAS_LAYER_popup);
            if (NULL == p_render->p_popup_canvas[i])
            {
                is_multi = false;
            }
        }

        assert(NULL == p_render->p_msgbox_canvas);
        p_render->p_msgbox_canvas = klb_canvas_malloc((klb_canvas_t*)p_main_canvas, 0, 0, KLB_CANVAS_LAYER_msgbox);
        if (NULL == p_render->p_msgbox_canvas)
        {
            is_multi = false;
        }

        // 是否为多画布模式
        p_render->is_multi_layer = is_multi;
    }
    else
    {
        // 释放画布
        for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
        {
            KLB_FREE_BY(p_render->p_popup_canvas[i], klb_canvas_destroy);
        }

        KLB_FREE_BY(p_render->p_msgbox_canvas, klb_canvas_destroy);

        p_render->is_multi_layer = false;
    }
}

int klbuiex_render_redraw_and_refresh(klbuiex_render_t* p_render)
{
    if (p_render->is_multi_layer)
    {
        // 多图层 绘图: 指 modal/popup/msgbox 主体窗口 分别采用独立的图层来绘制
        return multi_redraw_and_refresh_klbuiex_render(p_render);
    }
    else
    {
        // 单图层 绘图: 指 modal/popup/msgbox 主体窗口 采用单一图层来绘制
        return redraw_and_refresh_klbuiex_render(p_render->p_gui);
    }
}

/// @brief 是否为多图层模式
bool klbuiex_render_is_multi_layer(klbuiex_render_t* p_render)
{
    return p_render->is_multi_layer;
}

/// @brief 获取窗口对应的 画布
klb_canvas_t* klbuiex_render_get_popup_canvas(klbuiex_render_t* p_render, klb_wnd_t* p_top)
{
    klb_gui_t* p_gui = p_render->p_gui;

    if (p_render->is_multi_layer)
    {
        for (int i = 0; i < p_gui->popup_num; i++)
        {
            if (p_top == p_gui->p_popup_wnd[i])
            {
                return p_render->p_popup_canvas[i];
            }
        }
    }

    return p_gui->p_canvas;
}

klb_canvas_t* klbuiex_render_get_msgbox_canvas(klbuiex_render_t* p_render)
{
    klb_gui_t* p_gui = p_render->p_gui;

    if (p_render->is_multi_layer)
    {
        return p_render->p_msgbox_canvas;
    }

    return p_gui->p_canvas;
}

int klbuiex_render_popup_wnd(klbuiex_render_t* p_render, int idx, klb_wnd_t* p_top)
{
    klb_gui_t* p_gui = p_render->p_gui;

    if (p_render->is_multi_layer)
    {
        int x = p_top->pos.rect_in_canvas.x;
        int y = p_top->pos.rect_in_canvas.y;
        int w = p_top->pos.rect_in_canvas.w;
        int h = p_top->pos.rect_in_canvas.h;

        klb_canvas_t* p_canvas = p_render->p_popup_canvas[idx];
        klb_canvas_move(p_canvas, x, y);
        klb_canvas_resize(p_canvas, w, h);
    }

    return 0;
}

int klbuiex_render_msgbox_wnd(klbuiex_render_t* p_render, klb_wnd_t* p_top)
{
    if (p_render->is_multi_layer)
    {
        int x = p_top->pos.rect_in_canvas.x;
        int y = p_top->pos.rect_in_canvas.y;
        int w = p_top->pos.rect_in_canvas.w;
        int h = p_top->pos.rect_in_canvas.h;

        klb_canvas_t* p_canvas = p_render->p_msgbox_canvas;
        klb_canvas_move(p_canvas, x, y);
        klb_canvas_resize(p_canvas, w, h);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 注册/获取

klbuiex_render_t* klbuiex_get_render(klb_gui_t* p_gui)
{
    return (klbuiex_render_t*)klb_gui_get_extension(p_gui, KLBUIEX_RENDER);
}

int klbuiex_register_render(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_render_create;
    ex.cb_destroy = klbuiex_render_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLBUIEX_RENDER, &ex);

    return 0;
}

// end
