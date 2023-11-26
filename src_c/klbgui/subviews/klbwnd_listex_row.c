// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/subviews/klbwnd_listex_row.h"
#include "klbgui/wnd/klbwnd_listex.h"
#include "klbgui/wnd/klbwnd_check.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_listex_row_quit_attribute(klbwnd_listex_row_t* p_row);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_listex_row_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    klbwnd_listex_row_quit(p_wnd);

    KLB_FREE(p_wnd);
}


static void get_column_title_klbwnd_listex_row(sds* p_dst, klb_map_t* p_row_map, int column)
{
    switch (klb_map_array_type(p_row_map, column))
    {
    case KLB_ADT_bool:
    case KLB_ADT_string:
    case KLB_ADT_double:
    case KLB_ADT_uint64:
    case KLB_ADT_int64:
    {
        // 基本类型, 转字符串
        *p_dst = klb_sdscpy_adt(*p_dst, klb_map_idx_to_adt(p_row_map, column));
    }
    break;
    case KLB_ADT_map:
    {
        // 取首位(KV)
        klb_map_t* p_item_map = klb_map_idx_to_map(p_row_map, column);
        klb_map_iter_t* p_iter = klb_map_begin(p_item_map);
        if (NULL != p_iter)
        {
            *p_dst = klb_sdscpy_adt(*p_dst, klb_map_data(p_iter));
        }
        else
        {
            sdsclear(*p_dst);
        }
    }
    break;
    default:
    {
        sdsclear(*p_dst);
    }
    break;
    }
}

static void klbwnd_listex_row_on_paint_status(klb_wnd_t* p_wnd, klbwnd_listex_row_t* p_row, klbwnd_listex_row_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect, uint32_t background_color2)
{
    if (p_row->b_select)
    {
        // 选中状态
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 32, 32, 61));
    }
    else
    {
        // 纯色背景
        if (0 != p_row->idx % 2)
        {
            klb_wnd_draw_fill_rect2(p_wnd, p_rect, background_color2);
        }
        else
        {
            klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_attr->background.color);
        }
    }

    // 边框
    //klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);


    // 绘制行数据
    klbwnd_listex_head_t* p_head = p_row->p_head;
    klb_map_t* p_row_map = p_row->p_show_data;
    if (NULL != p_head && NULL != p_row_map)
    {
        int offx = p_rect->x + p_row->w0;

        for (int i = 0; i < p_head->column_count; i++)
        {
            klb_rect_t rect_column = *p_rect;
            rect_column.x = offx;
            rect_column.w = p_head->column[i].width;

            // 偏移值
            offx += rect_column.w;

            // 缩小一点文本区域范围, 避开线条
            rect_column.x += 2;
            rect_column.w -= 4;

            // 取文本值
            get_column_title_klbwnd_listex_row(&p_row->title, p_row_map, i);
            klbuicssex_draw_text(p_wnd, p_row->title, &rect_column, NULL, NULL, &p_attr->text, &p_attr->font);
        }
    }
}

static int klbwnd_listex_row_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_listex_row_css_t* p_css = p_row->p_row_css;

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
        klbwnd_listex_row_on_paint_status(p_wnd, p_row, p_css, &p_css->disable, &paint_rect, p_css->disable_background_color2);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_listex_row_on_paint_status(p_wnd, p_row, p_css, &p_css->focus, &paint_rect, p_css->focus_background_color2);
    }
    else
    {
        klbwnd_listex_row_on_paint_status(p_wnd, p_row, p_css, &p_css->normal, &paint_rect, p_css->normal_background_color2);
    }

    return 0;
}


static int klbwnd_listex_row_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_listex_row_on_paint(p_wnd);
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

// 行控件里面的 check 子控件响应
static int on_command_check_klbwnd_listex_row(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_row = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd_row->ctrl;

    if (KLBUI_onchange == e)
    {
        bool check = klb_wnd_is_check(p_wnd);
        p_row->b_check = check;

        if (p_row->p_show_data)
        {
            klb_map_set_bool(p_row->p_show_data, "check", check);
        }

        // check 内容变更
        klb_wnd_on_command(p_wnd_row, KLBUI_onchange, NULL, NULL, lparam, wparam);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_listex_row_set_css(klb_wnd_t* p_wnd, klbwnd_listex_row_css_t* p_row_css, klbwnd_listex_css_t* p_css)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_row->p_row_css = p_row_css;
    p_row->p_css = p_css;

    klbwnd_check_set_css(p_row->p_check, &p_css->css_check);
}

void klbwnd_listex_row_set_index(klb_wnd_t* p_wnd, int idx)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_row->idx = idx;
}

void klbwnd_listex_row_set_select(klb_wnd_t* p_wnd, bool check)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_row->b_select = check;
}

bool klbwnd_listex_row_get_select(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    return p_row->b_select;
}

void klbwnd_listex_row_set_head(klb_wnd_t* p_wnd, klbwnd_listex_head_t* p_head)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_row->p_head = p_head;
}

//static int on_commond_child_klbwnd_listex_row(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
//{
//    klb_wnd_t* p_wnd_row = (klb_wnd_t*)p_wnd->p_udata;
//
//    if (NULL != p_wnd_row->vtable.on_command)
//    {
//        p_wnd_row->vtable.on_command(p_wnd_row, e, p_pt1, p_pt2, lparam, wparam);
//    }
//
//    return 0;
//}

static void init_child_klbwnd_listex_row(klb_wnd_t* p_wnd, klbwnd_listex_row_t* p_row, int m, int n, klb_map_t* p_params, int offx, klb_wnd_on_command_cb on_command, void* ptr)
{
    klb_gui_t* p_gui = p_wnd->p_gui;

    // 若未创建, 则创建
    // 只认第一次类型和位置
    if (NULL == p_row->p_child[m][n])
    {
        // 控件类型
        const char* p_type = klb_map_to_string(p_params, "type");

        // 位置
        klb_rect_t rect = { offx, 0, 0, 0 };
        klb_map_t* p_pos = klb_map_to_map(p_params, "pos");
        if (NULL != p_pos)
        {
            rect.x = (int)klb_map_idx_to_int64(p_pos, 0) + offx;
            rect.y = (int)klb_map_idx_to_int64(p_pos, 1);
            rect.w = (int)klb_map_idx_to_int64(p_pos, 2);
            rect.h = (int)klb_map_idx_to_int64(p_pos, 3);
        }

        klb_wnd_create_cb create = klb_gui_get_creater(p_gui, p_type);

        // 创建子窗口
        klb_wnd_t* p_child = create(p_gui, rect.x, rect.y, rect.w, rect.h);
        klb_wnd_push_child(p_wnd, p_child);

        //klb_wnd_bind_command(p_child, on_commond_child_klbwnd_listex_row, p_wnd);
        klb_wnd_bind_command(p_child, on_command, ptr);

        p_row->p_child[m][n] = p_child;
    }

    // 设置其他参数
    klb_map_t child_param = { 0 };
    klb_map_init(&child_param);

    klb_map_iter_t* p_iter = klb_map_begin(p_params);
    while (NULL != p_iter)
    {
        klb_map_set_idx_string(&child_param, 0, klb_map_key(p_iter));
        klb_map_set_idx_adt_clone(&child_param, 1, klb_map_data(p_iter));

        klb_wnd_set(p_row->p_child[m][n], &child_param);

        p_iter = klb_map_next(p_iter);
    }

    klb_map_quit(&child_param);
}

void klbwnd_listex_row_child(klb_wnd_t* p_wnd, klb_wnd_on_command_cb on_command, void* ptr)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;
    assert(NULL != p_row->p_head);

    int offx = p_row->w0;
    for (int i = 0; i < p_row->p_head->column_count; i++)
    {
        klb_map_t* p_child = p_row->p_head->column[i].p_child;
        if (NULL != p_child)
        {
            int array_size = klb_map_array_size(p_child);
            int count = MIN(KLBWND_LISTEX_ROW_child_max, array_size);

            for (int k = 0; k < count; k++)
            {
                klb_map_t* p_params = klb_map_idx_to_map(p_child, k);
                init_child_klbwnd_listex_row(p_wnd, p_row, i, k, p_params, offx, on_command, ptr);
            }
        }

        offx += p_row->p_head->column[i].width;
    }
}

void klbwnd_listex_row_set_show_data(klb_wnd_t* p_wnd, klb_map_t* p_show_data)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_row->p_show_data = p_show_data;

    bool show = (NULL != p_show_data) ? true : false;
    klb_wnd_show(p_row->p_check, show);

    if (NULL != p_show_data)
    {
        bool check = klb_map_to_bool(p_show_data, "check");

        p_row->b_check = check;
        klb_wnd_check(p_row->p_check, check);
    }

    for (int m = 0; m < KLBWND_LISTEX_column_max; m++)
    {
        for (int n = 0; n < KLBWND_LISTEX_ROW_child_max; n++)
        {
            if (p_row->p_child[m][n])
            {
                klb_wnd_show(p_row->p_child[m][n], show);

                klb_map_t* p_column_map = (NULL != p_show_data) ? klb_map_idx_to_map(p_show_data, m) : NULL;
                klb_map_t* p_idx_map = (NULL != p_column_map) ? klb_map_idx_to_map(p_column_map, n) : NULL;

                if (show && p_idx_map)
                {
                    // 设置其他参数
                    klb_map_t child_param = { 0 };
                    klb_map_init(&child_param);

                    klb_map_iter_t* p_iter = klb_map_begin(p_idx_map);
                    while (NULL != p_iter)
                    {
                        klb_map_set_idx_string(&child_param, 0, klb_map_key(p_iter));
                        klb_map_set_idx_adt_clone(&child_param, 1, klb_map_data(p_iter));

                        klb_wnd_set(p_row->p_child[m][n], &child_param);

                        p_iter = klb_map_next(p_iter);
                    }

                    klb_map_quit(&child_param);
                }
            }
        }
    }
}

bool klbwnd_listex_row_get_check(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    return p_row->b_check;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_listex_row_init_attribute(klb_wnd_t* p_wnd, klbwnd_listex_row_t* p_row)
{
    p_row->b_select = false;
    p_row->b_check = false;
}

static void klbwnd_listex_row_quit_attribute(klbwnd_listex_row_t* p_row)
{
    KLB_FREE_BY(p_row->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_listex_row_css_init(klbwnd_listex_row_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal_background_color2 = KLB_ARGB8888(255, 32, 32, 32);
    p_css->focus_background_color2 = KLB_ARGB8888(255, 32, 32, 32);
    p_css->disable_background_color2 = KLB_ARGB8888(255, 32, 32, 32);
}

void klbwnd_listex_row_css_quit(klbwnd_listex_row_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);
}

void klbwnd_listex_row_css_copy(klbwnd_listex_row_css_t* p_dst, klbwnd_listex_row_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);

    p_dst->normal_background_color2 = p_src->normal_background_color2;
    p_dst->focus_background_color2 = p_src->focus_background_color2;
    p_dst->disable_background_color2 = p_src->disable_background_color2;
}

//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_listex_row_init_subwnds(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;
    klb_gui_t* p_gui = p_wnd->p_gui;

    int w = p_wnd->pos.rect_in_parent.w;
    int h = p_wnd->pos.rect_in_parent.h;

    p_row->w0 = h;

    int w_check = (h * 2) / 3, h_check = w_check;

    p_row->p_check = klbwnd_check_create(p_gui, (p_row->w0 - w_check) / 2, (h - h_check) / 2, w_check, w_check);
    klb_wnd_push_child(p_wnd, p_row->p_check);

    klb_wnd_bind_command(p_row->p_check, on_command_check_klbwnd_listex_row, p_wnd);
}

void klbwnd_listex_row_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                               // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_listex_row_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                            // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                                // 由继承者重写
    p_wnd->vtable.on_get = NULL;                                // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 初始化子属性
    klbwnd_listex_row_init_attribute(p_wnd, p_row);

    // 初始子窗口
    klbwnd_listex_row_init_subwnds(p_wnd);
}

void klbwnd_listex_row_quit(klb_wnd_t* p_wnd)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;

    klbwnd_listex_row_quit_attribute(p_row);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_listex_row_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_listex_row_t));

    klbwnd_listex_row_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_listex_row_destroy;

    return p_wnd;
}
