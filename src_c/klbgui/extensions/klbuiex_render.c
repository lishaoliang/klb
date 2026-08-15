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
    klb_gui_t*      p_gui;                                  ///< GUI模块

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

        // Fixed Bug. [2025] 这里还有一种情况: 放入刷新列表的窗口, 根本不在 所有需要绘制的窗口链中
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

    // step4. 汇总刷新

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 多画布
// 

// 添加图层画布
static void add_canvas_layer_klbuiex_render(klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int* p_layer_count, int layer_type, bool use, bool redraw, klb_rect_t* p_rect, klb_canvas_t* p_canvas)
{
    int idx = *p_layer_count;
    if (NULL != p_canvas && idx + 1 <= KLB_CANVAS_LAYER_max)
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

    // 放入 所有 使用到的图层
    // 并初始化 是否使用 状态

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

    // KLB_CANVAS_LAYER_udata 图层
    {
        klb_canvas_t* p_udata_canvas = klbuiex_udatalayer_get_canvas(p_gui->p_udatalayer);
        bool is_show_udata = klbuiex_udatalayer_is_show(p_gui->p_udatalayer);

        add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_udata, is_show_udata, false, NULL, p_udata_canvas);
    }

    // KLB_CANVAS_LAYER_wait 图层
    {
        klb_canvas_t* p_wait_canvas = klbuiex_waitlayer_get_canvas(p_gui->p_waitlayer);
        bool is_show_wait = klbuiex_waitlayer_is_show(p_gui->p_waitlayer);

        add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_wait, is_show_wait, false, NULL, p_wait_canvas);
    }

    // KLB_CANVAS_LAYER_tip 图层
    {
        klb_canvas_t* p_tip_canvas = klbuiex_tip_get_canvas(p_gui->p_tip);
        bool is_show_tip = klbuiex_tip_is_show(p_gui->p_tip);

        add_canvas_layer_klbuiex_render(layers, p_layer_count, KLB_CANVAS_LAYER_tip, is_show_tip, false, NULL, p_tip_canvas);
    }
}

// 放入图层
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

// 按列表重绘
static bool multi_redraw_wnd_hlist_klbuiex_render(klbuiex_render_t* p_render, klb_hlist_t* p_hlist, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    bool is_real_refresh = false;

    // 依次绘制所有窗口
    // !!!注意: 放入列表的窗口可能是无序的, 但图层绘制必须是有序的
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_hlist);
    while (NULL != p_iter)
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_data(p_iter);
        int index = 0;

        if (is_in_model_klbuiex_render(p_gui, p_wnd))
        {
            // 重绘
            klb_wnd_draw(p_wnd);

            // 标记需要刷新
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &p_wnd->pos.rect_in_canvas, p_gui->p_canvas);

            is_real_refresh = true;
        }
        else if(is_in_popup_klbuiex_render(p_gui, p_wnd, &index))
        {
            // 重绘
            klb_wnd_draw(p_wnd);

            // 标记需要刷新
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_popup, true, &p_wnd->pos.rect_in_canvas, p_render->p_popup_canvas[index]);

            is_real_refresh = true;
        }
        else if(is_in_msgbox_klbuiex_render(p_gui, p_wnd))
        {
            // 重绘
            klb_wnd_draw(p_wnd);

            // 标记需要刷新
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_msgbox, true, &p_wnd->pos.rect_in_canvas, p_render->p_msgbox_canvas);

            is_real_refresh = true;
        }
        else if(klbuiex_udatalayer_wnd_in_bind(p_gui->p_udatalayer, p_wnd))
        {
            // 在用户自定义图层 中; 标记 需要重绘
            klbuiex_udatalayer_set_redraw(p_gui->p_udatalayer);
        }
        else if(klbuiex_waitlayer_wnd_in_bind(p_gui->p_waitlayer, p_wnd))
        {
            // 在 等待图层 中; 标记 需要重绘
            klbuiex_waitlayer_set_redraw(p_gui->p_waitlayer);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    return is_real_refresh;
}

// 重绘所有窗口
static bool multi_redraw_wnd_all_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count)
{
    klb_gui_t* p_gui = p_render->p_gui;

    if (NULL == p_gui->p_canvas)
    {
        return false;
    }

    // step1. 清理主画布
    // 1. 因modal窗口 并不一定占满 整个屏幕, 这里需要清理
    klb_canvas_set_draw_color(p_gui->p_canvas, KLB_ARGB8888(0, 0, 0, 0));
    klb_canvas_draw_clear(p_gui->p_canvas);

    // step2. 绘制 modal 窗口 / 放入需要更新图层
    {
        // 重绘
        for (int i = 0; i < p_gui->modal_num; i++)
        {
            klb_wnd_draw(p_gui->p_modal_wnd[i]);
        }

        // 标记 需要刷新画布
        push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &p_gui->p_canvas->rect, p_gui->p_canvas);
    }

    // step3. 绘制 popup 窗口 / 放入需要更新图层
    // 1. 因 popup 窗口, 一定会占满 整个 popup 画布区域, 所以肯定会被完整更新
    for (int i = 0; i < p_gui->popup_num; i++)
    {
        if (NULL != p_gui->p_popup_wnd[i])
        {
            // popup窗口可能 在边界为透明 这里需要清理画布
            klb_canvas_t* p_popup_canvas = p_render->p_popup_canvas[i];

            // 清理画布
            klb_canvas_set_draw_color(p_popup_canvas, KLB_ARGB8888(0, 0, 0, 0));
            klb_canvas_draw_clear(p_popup_canvas);

            // 重绘
            klb_wnd_draw(p_gui->p_popup_wnd[i]);

            // 标记 需要刷新画布
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_popup, true, &p_popup_canvas->rect, p_popup_canvas);
        }
    }

    // step3.绘制 msgbox 窗口 / 放入需要更新图层
    if (NULL != p_gui->p_msg_box)
    {
        // msgbox窗口可能 在边界为透明 这里需要清理画布
        klb_canvas_t* p_msgbox_canvas = p_render->p_msgbox_canvas;

        // 清理画布
        klb_canvas_set_draw_color(p_msgbox_canvas, KLB_ARGB8888(0, 0, 0, 0));
        klb_canvas_draw_clear(p_msgbox_canvas);

        // 重绘
        klb_wnd_draw(p_gui->p_msg_box);

        // 标记 需要刷新画布
        push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_msgbox, true, &p_msgbox_canvas->rect, p_msgbox_canvas);
    }

    return true;
}

// 处理 udata 图层
static bool muti_redraw_udata_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count, bool is_redraw_all)
{
    klb_gui_t* p_gui = p_render->p_gui;
    klb_canvas_t* p_udata_canvas = klbuiex_udatalayer_get_canvas(p_gui->p_udatalayer);

    bool is_need_refresh = false;

    if (is_redraw_all)
    {
        // 完整更新模式
        if (klbuiex_udatalayer_is_show(p_gui->p_udatalayer))
        {
            // 设置重绘
            klbuiex_udatalayer_set_redraw(p_gui->p_udatalayer);

            // 重绘
            klbuiex_udatalayer_redraw(p_gui->p_udatalayer);

            // 标记 需要刷新画布            
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_udata, true, &p_udata_canvas->rect, p_udata_canvas);

            // 设置 udata的 "脏矩形" 区域
            klbuiex_udatalayer_set_dirty(p_gui->p_udatalayer, true, &p_udata_canvas->rect);

            is_need_refresh = true;
        }
    }
    else
    {
        // 区域更新模式

        // step1. 检查是否, 需要处理上次 TIP的 "脏矩形" 区域
        {
            klb_rect_t dirty_rect = { 0 };
            if (klbuiex_udatalayer_get_dirty(p_gui->p_udatalayer, &dirty_rect))
            {
                bool refresh_dirty = true;

                // 若 处于 显示, 且 显示区域和 "脏矩形" 一致, 则暂不需要刷新 "脏矩形"
                klb_rect_t rect = p_udata_canvas->rect;
                if (klbuiex_udatalayer_is_show(p_gui->p_udatalayer) &&
                    rect.x == dirty_rect.x && rect.y == dirty_rect.y &&
                    rect.w == dirty_rect.w && rect.h == dirty_rect.h)
                {
                    refresh_dirty = false;
                }

                if (refresh_dirty)
                {
                    // 标记 需要刷新画布
                    push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &dirty_rect, p_gui->p_canvas);

                    // 已经处理了 "脏矩形"
                    klbuiex_udatalayer_set_dirty(p_gui->p_udatalayer, false, NULL);

                    is_need_refresh = true;
                }
            }
        }

        // step2. 检查是否需要处理 当前显示的TIP
        {
            if (klbuiex_udatalayer_is_show(p_gui->p_udatalayer) && klbuiex_udatalayer_redraw(p_gui->p_udatalayer))
            {
                // 标记 需要刷新画布
                push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_udata, true, &p_udata_canvas->rect, p_udata_canvas);

                // 设置 udata 的 "脏矩形" 区域
                klbuiex_udatalayer_set_dirty(p_gui->p_udatalayer, true, &p_udata_canvas->rect);

                is_need_refresh = true;
            }
        }
    }

    return is_need_refresh;
}

// 处理 wait 图层
static bool muti_redraw_wait_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count, bool is_redraw_all)
{
    klb_gui_t* p_gui = p_render->p_gui;
    klb_canvas_t* p_wait_canvas = klbuiex_waitlayer_get_canvas(p_gui->p_waitlayer);

    bool is_need_refresh = false;

    if (is_redraw_all)
    {
        // 完整更新模式
        if (klbuiex_waitlayer_is_show(p_gui->p_waitlayer))
        {
            // 重绘
            klbuiex_waitlayer_redraw(p_gui->p_waitlayer);

            // 标记 需要刷新画布            
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_wait, true, &p_wait_canvas->rect, p_wait_canvas);

            // 设置 wait 的 "脏矩形" 区域
            klbuiex_waitlayer_set_dirty(p_gui->p_waitlayer, true, &p_wait_canvas->rect);

            is_need_refresh = true;
        }
    }
    else
    {
        // 区域更新模式

        // step1. 检查是否, 需要处理上次 TIP的 "脏矩形" 区域
        {
            klb_rect_t dirty_rect = { 0 };
            if (klbuiex_waitlayer_get_dirty(p_gui->p_waitlayer, &dirty_rect))
            {
                bool refresh_dirty = true;

                // 若 wait 图层 处于 显示, 且 显示区域和 "脏矩形" 一致, 则暂不需要刷新 "脏矩形"
                klb_rect_t rect = p_wait_canvas->rect;
                if (klbuiex_waitlayer_is_show(p_gui->p_waitlayer) &&
                    rect.x == dirty_rect.x && rect.y == dirty_rect.y &&
                    rect.w == dirty_rect.w && rect.h == dirty_rect.h)
                {
                    refresh_dirty = false;
                }

                if (refresh_dirty)
                {
                    // 标记 需要刷新画布
                    push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &dirty_rect, p_gui->p_canvas);

                    // 已经处理了 "脏矩形"
                    klbuiex_waitlayer_set_dirty(p_gui->p_waitlayer, false, NULL);

                    is_need_refresh = true;
                }
            }
        }

        // step2. 检查是否需要处理 当前显示的TIP
        {
            if (klbuiex_waitlayer_is_show(p_gui->p_waitlayer) && klbuiex_waitlayer_redraw(p_gui->p_waitlayer))
            {
                // 标记 需要刷新画布
                push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_wait, true, &p_wait_canvas->rect, p_wait_canvas);

                // 设置 wait 图层 的 "脏矩形" 区域
                klbuiex_waitlayer_set_dirty(p_gui->p_waitlayer, true, &p_wait_canvas->rect);

                is_need_refresh = true;
            }
        }
    }

    return is_need_refresh;
}

// 处理TIP图层
static bool muti_redraw_tip_klbuiex_render(klbuiex_render_t* p_render, klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max], int layer_count, bool is_redraw_all)
{
    klb_gui_t* p_gui = p_render->p_gui;
    klb_canvas_t* p_tip_canvas = klbuiex_tip_get_canvas(p_gui->p_tip);

    bool is_need_refresh = false;

    if (is_redraw_all)
    {
        // 完整更新模式
        if (klbuiex_tip_is_show(p_gui->p_tip))
        {
            // 设置重绘
            klbuiex_tip_set_redraw(p_gui->p_tip);

            // 重绘
            klbuiex_tip_redraw(p_gui->p_tip);

            // 标记 需要刷新画布            
            push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_tip, true, &p_tip_canvas->rect, p_tip_canvas);

            // 设置 TIP的 "脏矩形" 区域
            klbuiex_tip_set_dirty(p_gui->p_tip, true, &p_tip_canvas->rect);

            is_need_refresh = true;
        }
    }
    else
    {
        // 区域更新模式

        // step1. 检查是否, 需要处理上次 TIP的 "脏矩形" 区域
        {
            klb_rect_t dirty_rect = { 0 };
            if (klbuiex_tip_get_dirty(p_gui->p_tip, &dirty_rect))
            {
                bool refresh_dirty = true;

                // 若 TIP处于 显示, 且 显示区域和 "脏矩形" 一致, 则暂不需要刷新 "脏矩形"
                klb_rect_t rect = p_tip_canvas->rect;
                if (klbuiex_tip_is_show(p_gui->p_tip) && 
                    rect.x == dirty_rect.x && rect.y == dirty_rect.y && 
                    rect.w == dirty_rect.w && rect.h == dirty_rect.h)
                {
                    refresh_dirty = false;
                }

                if (refresh_dirty)
                {
                    // 标记 需要刷新画布
                    push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_main, true, &dirty_rect, p_gui->p_canvas);

                    // 已经处理了 "脏矩形"
                    klbuiex_tip_set_dirty(p_gui->p_tip, false, NULL);

                    is_need_refresh = true;
                }
            }
        }

        // step2. 检查是否需要处理 当前显示的TIP
        {
            if (klbuiex_tip_is_show(p_gui->p_tip) && klbuiex_tip_redraw(p_gui->p_tip))
            {
                // 标记 需要刷新画布
                push_canvas_layer_klbuiex_render(layers, layer_count, KLB_CANVAS_LAYER_tip, true, &p_tip_canvas->rect, p_tip_canvas);

                // 设置 TIP的 "脏矩形" 区域
                klbuiex_tip_set_dirty(p_gui->p_tip, true, &p_tip_canvas->rect);

                is_need_refresh = true;
            }
        }
    }

    return is_need_refresh;
}

// 重绘,刷新
static int multi_redraw_and_refresh_klbuiex_render(klbuiex_render_t* p_render)
{
    klb_gui_t* p_gui = p_render->p_gui;

    // step1. 初始化 所有 有效 图层
    klb_canvas_layer_t layers[KLB_CANVAS_LAYER_max] = { 0 };
    int layer_count = 0;
    init_canvas_layer_klbuiex_render(p_render, layers, &layer_count);

    // step2. 检查主窗口的刷新
    bool is_real_refresh_wnd = false;
    bool is_redraw_all = false; // 是否完整更新
    bool is_refresh_wnd = klbuiex_redraw_need_repaint(p_gui->p_redraw, &is_redraw_all);
    if (is_refresh_wnd)
    {
        if (is_redraw_all)
        {
            // 完整更新模式; 需要完全重绘
            is_real_refresh_wnd = multi_redraw_wnd_all_klbuiex_render(p_render, layers, layer_count);
        }
        else
        {
            // 区域更新模式; 局部重绘, 按需要刷新的窗口列表重绘
            is_real_refresh_wnd = multi_redraw_wnd_hlist_klbuiex_render(p_render, klbuiex_redraw_get_hlist(p_gui->p_redraw), layers, layer_count);
        }

        // 绘制完成, 清空 redraw 标记
        klbuiex_redraw_clear(p_gui->p_redraw);
    }

    // step4. 处理 udata 图层
    bool is_real_refresh_udata = muti_redraw_udata_klbuiex_render(p_render, layers, layer_count, is_redraw_all);

    // step5. 处理 wait 图层
    bool is_real_refresh_wait = muti_redraw_wait_klbuiex_render(p_render, layers, layer_count, is_redraw_all);

    // step6. 处理TIP图层
    bool is_real_refresh_tip = muti_redraw_tip_klbuiex_render(p_render, layers, layer_count, is_redraw_all);

    // stepN. 依据刷新标记, 刷新图层
    if (NULL != p_gui->p_canvas && (
        is_real_refresh_wnd ||      // 窗口层是否需要刷新
        is_real_refresh_udata ||    // udata图层 是否需要刷新
        is_real_refresh_wait ||     // wait 图层 是否需要刷新
        is_real_refresh_tip))       // tip 图层 是否需要刷新
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
