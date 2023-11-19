// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/wnd/klbwnd_listex.h"
#include "klbgui/subviews/klbwnd_listex_row.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_listex_quit_attribute(klbwnd_listex_t* p_list);
static int on_command_vscrollbar_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);
static int on_command_check_all_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);
static int on_command_child_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);
static int on_command_listex_row_klbwnd_listex(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);

//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_listex_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    klbwnd_listex_quit(p_wnd);

    KLB_FREE(p_wnd);
}

static void klbwnd_listex_on_paint_status_head(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list, klbwnd_listex_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 绘制题头
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 41, 41, 41));

    // 处理每一列
    int offx = p_rect->x + p_list->w0;
    for (int i = 0; i < p_list->head.column_count; i++)
    {
        klb_rect_t rect_column = *p_rect;
        rect_column.x = offx;
        rect_column.w = p_list->head.column[i].width;

        // 偏移值
        offx += rect_column.w;

        // 中间竖线
        klb_wnd_draw_line2(p_wnd, rect_column.x + rect_column.w, rect_column.y,
                        rect_column.x + rect_column.w, rect_column.y + rect_column.h - 1, KLB_ARGB8888(255, 61, 61, 61));

        // 缩小一点文本区域范围, 避开线条
        rect_column.x += 2;
        rect_column.w -= 4;

        // 标题文本
        klbuicssex_draw_text(p_wnd, p_list->head.column[i].title, &rect_column, NULL, NULL, &p_attr->text, &p_attr->font);
    }
}

static void klbwnd_listex_on_paint_status_body(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list, klbwnd_listex_css_t* p_css, klbuicssex_attributes_t* p_attr)
{
    /* eg.
    jq('listex1').append({
        {{['11']='行11'}, {['12']='行12'}, {['13']='行13'}},
        {{['21']='行21'}, {['22']='行22'}, {['23']='行23'}},
        {{['31']='行31'}, {['32']='行32'}, {['33']='行33'}},
    })

    或

    jq('listex1').append({
        {'行11', '行12', '行13'},
        {'行21', '行22', '行23'},
        {'行31', '行32', '行33'},
    })
    */

    // 绘制内容体
    // 行内容体数据 交给 子控件 去绘制
    klb_map_t* p_data_map = &p_list->data_map;

    // 数据的数组大小
    int array_size = klb_map_array_size(p_data_map);
    int start = p_list->vsc_value; // 显示数据的起始位置

    for (int i = 0; i < p_list->rows_count; i++)
    {
        klb_wnd_t* p_list_row = p_list->p_rows[i];

        // 数据
        klb_map_t* p_row_map = klb_map_idx_to_map(p_data_map, i + start);
        if (NULL != p_row_map)
        {
            klbwnd_listex_row_set_show_data(p_list_row, p_row_map);
        }
        else
        {
            klbwnd_listex_row_set_show_data(p_list_row, NULL);
        }

        // 选中状态
        if (0 <= p_list->sel &&  p_list->sel < array_size && p_list->sel == i + start)
        {
            klbwnd_listex_row_set_select(p_list_row, true);
        }
        else
        {
            klbwnd_listex_row_set_select(p_list_row, false);
        }
    }
}

static void klbwnd_listex_on_paint_status(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list, klbwnd_listex_css_t* p_css, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
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

    klb_rect_t rect_content = *p_rect;
    rect_content.x += p_attr->border.width.left;
    rect_content.y += p_attr->border.width.top;
    rect_content.w -= (p_attr->border.width.left + p_attr->border.width.right);
    rect_content.h -= (p_attr->border.width.top + p_attr->border.width.bottom);

    int head_h = 42;

    // 绘制题头
    klb_rect_t rect_head = rect_content;
    rect_head.h = head_h;
    klbwnd_listex_on_paint_status_head(p_wnd, p_list, p_css, p_attr, &rect_head);

    // 内容体
    klbwnd_listex_on_paint_status_body(p_wnd, p_list, p_css, p_attr);
}

static int klbwnd_listex_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_listex_css_t* p_css = p_list->p_css;

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
        klbwnd_listex_on_paint_status(p_wnd, p_list, p_css, &p_css->disable, &paint_rect);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_listex_on_paint_status(p_wnd, p_list, p_css, &p_css->focus, &paint_rect);
    }
    else
    {
        klbwnd_listex_on_paint_status(p_wnd, p_list, p_css, &p_css->normal, &paint_rect);
    }

    return 0;
}

// 重新布局
static int relayout_body_klbwnd_listex(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list, klb_rect_t* p_rect, int row_h)
{
    // 滚动条
    klb_map_t* p_data_map = &p_list->data_map;
    int array_size = klb_map_array_size(p_data_map);

    if (p_list->rows_count < array_size)
    {
        // 需要滚动条
        p_list->vsc_max = array_size - p_list->rows_count / 2;

        if (p_list->vsc_value < p_list->vsc_min) { p_list->vsc_value = p_list->vsc_min; };
        if (p_list->vsc_max < p_list->vsc_value) { p_list->vsc_value = p_list->vsc_max; };

        klbwnd_vscrollbar_set_ranges(p_list->p_vscrollbar, p_list->vsc_min, p_list->vsc_max, 1);
        klbwnd_vscrollbar_set_value(p_list->p_vscrollbar, p_list->vsc_value);

        klb_wnd_move(p_list->p_vscrollbar, p_rect->x + p_rect->w - 20, p_rect->y);
        klb_wnd_resize(p_list->p_vscrollbar, 20, p_rect->h);

        p_rect->w -= 20;

        klb_wnd_show(p_list->p_vscrollbar, true);
    }
    else
    {
        // 不需要滚动条
        klb_wnd_show(p_list->p_vscrollbar, false);
        p_list->vsc_value = 0;
    }

    int offy = p_rect->y;

    for (int i = 0; i < p_list->rows_count; i++)
    {
        klb_rect_t rect_row = *p_rect;
        rect_row.y = offy;
        rect_row.h = row_h;

        klb_wnd_t* p_list_row = p_list->p_rows[i];

        klb_wnd_move(p_list_row, rect_row.x, rect_row.y);
        klb_wnd_resize(p_list_row, rect_row.w, rect_row.h);

        offy += row_h;
    }

    return 0;
}

// 预绘制事件
static int klbwnd_listex_on_predraw(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_parent;

    // 在预绘制事件中, 处理列表框的 行控件
    klbwnd_listex_css_t* p_css = p_list->p_css;

    int head_h = 42;
    int row_h = 32;

    klb_rect_t rect_body = { 0 };
    rect_body.x = p_css->normal.border.width.left;
    rect_body.y = p_css->normal.border.width.top + head_h;
    rect_body.w = p_rect->w - (p_css->normal.border.width.left + p_css->normal.border.width.right);
    rect_body.h = p_rect->h - (p_css->normal.border.width.top + p_css->normal.border.width.bottom) - head_h - 1;

    int row_count = (0 < rect_body.h) ? rect_body.h / row_h : 0;  // 总共可容纳控件个数

    // 检查 行控件 是否存在
    // 确保需要使用的处于显示状态
    for (int i = 0; i < KLBWND_LISTEX_row_max; i++)
    {
        if (i < row_count)
        {
            klb_wnd_t* p_list_row = p_list->p_rows[i];
            if (NULL == p_list_row)
            {
                p_list_row = klbwnd_listex_row_create(p_wnd->p_gui, 0, 0, rect_body.w, row_h);
                klb_wnd_push_child(p_wnd, p_list_row);

                // 自身响应
                klb_wnd_bind_command(p_list_row, on_command_listex_row_klbwnd_listex, p_wnd);

                klbwnd_listex_row_set_head(p_list_row, &p_list->head);

                // 子控件响应
                klbwnd_listex_row_child(p_list_row, on_command_child_klbwnd_listex, p_wnd);

                p_list->p_rows[i] = p_list_row;
            }

            klbwnd_listex_row_set_css(p_list_row, &p_list->p_css->css_listex_row, p_list->p_css);
            klbwnd_listex_row_set_index(p_list_row, i);
            klb_wnd_show(p_list_row, true);
        }
        else
        {
            // 显示不了, 隐藏起来
            if (p_list->p_rows[i])
            {
                klb_wnd_show(p_list->p_rows[i], false);
            }
        }
    }

    p_list->rows_count = row_count;

    // 重新布局
    relayout_body_klbwnd_listex(p_wnd, p_list, &rect_body, row_h);

    return 0;
}

static int klbwnd_listex_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpredraw:
        return klbwnd_listex_on_predraw(p_wnd);
        break;
    case KLBUI_onpaint:
        return klbwnd_listex_on_paint(p_wnd);
        break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 私有函数

// 滚动条事件
static int on_command_vscrollbar_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_listex = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd_listex->ctrl;

    if (KLBUI_onchange == e)
    {
        p_list->vsc_value = klbwnd_vscrollbar_get_value(p_list->p_vscrollbar);

        klb_wnd_update(p_wnd_listex);
    }

    return 0;
}

// 左上角 check all 事件
static int on_command_check_all_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_listex = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd_listex->ctrl;

    if (KLBUI_onchange == e)
    {
        bool check_all = klb_wnd_is_check(p_wnd);

        klb_map_t* p_data_map = klbwnd_listex_get_data_map(p_wnd_listex);
        int array_size = klb_map_array_size(p_data_map);
        for (int i = 0; i < array_size; i++)
        {
            klb_map_t* p_row_map = klb_map_idx_to_map(p_data_map, i);
            if (p_row_map)
            {
                klb_map_set_bool(p_row_map, "check", check_all);
            }
        }

        klb_wnd_update(p_wnd_listex);
    }

    return 0;
}

// 行控件里面的 子控件响应
static int on_command_child_klbwnd_listex(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd_listex = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd_listex->ctrl;

    // 子控件响应之后, 发现难以使用标准的消息格式来表达所有情况
    // 这里临时记录子控件, 若调用者需要控件信息, 可读取其指针, 并通过其指针访问具体信息
    // 注意: 记录的信息很快会被替换掉
    p_list->p_event_wnd = p_wnd;

    // 添加bit2标记, 转换为特殊事件
    e |= KLBUI_event_bit2;

    // 转换为 listex 的事件
    klb_wnd_on_command(p_wnd_listex, e, p_pt1, p_pt2, lparam, wparam);

    return 0;
}

// 子行 控件响应
static int on_command_listex_row_klbwnd_listex(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_listex_row_t* p_row = (klbwnd_listex_row_t*)p_wnd->ctrl;
    klb_wnd_t* p_wnd_listex = (klb_wnd_t*)p_wnd->p_udata;
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd_listex->ctrl;

    if (KLBUI_click == msg || KLBUI_dblclick == msg)
    {
        // 单行选中
        if (p_list->enable_sel)
        {
            // 有效的选中点击
            if (NULL != p_row->p_head && NULL != p_row->p_show_data)
            {
                if (p_list->sel != p_row->idx)
                {
                    for (int i = 0; i < p_list->rows_count; i++)
                    {
                        if (klbwnd_listex_row_get_select(p_list->p_rows[i]))
                        {
                            klbwnd_listex_row_set_select(p_list->p_rows[i], false);
                            klb_wnd_update(p_list->p_rows[i]);
                        }
                    }

                    p_list->sel = p_row->idx;

                    klbwnd_listex_row_set_select(p_wnd, true);
                    klb_wnd_update(p_wnd);

                    klb_wnd_on_command(p_wnd_listex, KLBUI_onchange, NULL, NULL, 0, 0);
                }
            }
        }
    }
    else if (KLBUI_onchange == msg)
    {
        // 有 check 被选中了, 检查数据
        bool check_all = true;

        klb_map_t* p_data_map = klbwnd_listex_get_data_map(p_wnd_listex);
        int array_size = klb_map_array_size(p_data_map);
        for (int i = 0; i < array_size; i++)
        {
            klb_map_t* p_row_map = klb_map_idx_to_map(p_data_map, i);
            bool check = (NULL != p_row_map) ? klb_map_to_bool(p_row_map, "check") : false;

            if (!check)
            {
                check_all = false;
                break;
            }
        }

        klb_wnd_check(p_list->p_check, check_all);
        klb_wnd_update(p_list->p_check);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_listex_set_css(klb_wnd_t* p_wnd, klbwnd_listex_css_t* p_css)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    p_list->p_css = p_css;

    if (p_list->p_check)
    {
        klbwnd_check_set_css(p_list->p_check, &p_css->css_check);
    }

    if (p_list->p_vscrollbar)
    {
        klbwnd_vscrollbar_set_css(p_list->p_vscrollbar, &p_css->css_vscrollbar);
    }
}

int klbwnd_listex_append_column(klb_wnd_t* p_wnd, int w_column, const char* p_title, const klb_map_t* p_child)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    if (KLBWND_LISTEX_column_max <= p_list->head.column_count)
    {
        return 1; // 满了
    }

    // 新增加
    int idx = p_list->head.column_count;

    p_list->head.column[idx].enable = true;
    p_list->head.column[idx].width = w_column;
    p_list->head.column[idx].title = klb_sdscpy(p_list->head.column[idx].title, p_title);

    if (NULL != p_child)
    {
        if (NULL == p_list->head.column[idx].p_child)
        {
            p_list->head.column[idx].p_child = klb_map_create();
        }

        klb_map_copy(p_list->head.column[idx].p_child, p_child);
    }
    else
    {
        KLB_FREE_BY(p_list->head.column[idx].p_child, klb_map_destroy);
    }

    p_list->head.column_count++;

    return 0;
}

klb_map_t* klbwnd_listex_get_data_map(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    return &p_list->data_map;
}

/// @brief 重新布局
void klbwnd_listex_relayout(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_parent;

    // 在预绘制事件中, 处理列表框的 行控件
    klbwnd_listex_css_t* p_css = p_list->p_css;

    int head_h = 42;
    int row_h = 32;

    klb_rect_t rect_body = { 0 };
    rect_body.x = p_css->normal.border.width.left;
    rect_body.y = p_css->normal.border.width.top + head_h;
    rect_body.w = p_rect->w - (p_css->normal.border.width.left + p_css->normal.border.width.right);
    rect_body.h = p_rect->h - (p_css->normal.border.width.top + p_css->normal.border.width.bottom) - head_h - 1;

    // 重新布局
    relayout_body_klbwnd_listex(p_wnd, p_list, &rect_body, row_h);

    // 需要重新更新画布坐标
    klb_wnd_update_canvas_rect(p_wnd);
}

void klbwnd_listex_clear(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    // 清空选中
    p_list->sel = -1;

    // 清空列
    p_list->head.column_count = 0;

    // 清空数据
    klb_map_clear(&p_list->data_map);

    // 重新布局
    klbwnd_listex_relayout(p_wnd);
}

void klbwnd_listex_clear_data(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    // 清空选中
    p_list->sel = -1;

    // 清空数据
    klb_map_clear(&p_list->data_map);

    // 重新布局
    klbwnd_listex_relayout(p_wnd);
}

void klbwnd_listex_set_sel(klb_wnd_t* p_wnd, int sel)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    p_list->sel = sel;
}

klb_map_t* klbwnd_listex_get_sel(klb_wnd_t* p_wnd, int* p_sel)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    // 数据
    klb_map_t* p_data_map = &p_list->data_map;

    // 数据的数组大小
    int array_size = klb_map_array_size(p_data_map);

    if (0 <= p_list->sel &&  p_list->sel < array_size)
    {
        // 有效的选中
        if (p_sel) *p_sel = p_list->sel;
        return klb_map_idx_to_map(p_data_map, p_list->sel);
    }

    if (p_sel) *p_sel = -1;
    return NULL;
}

/// @brief 获取当前事件窗口
klb_wnd_t* klbwnd_listex_get_event_wnd(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    return p_list->p_event_wnd;
}

//////////////////////////////////////////////////////////////////////////
// init / quit attribute

static void klbwnd_listex_init_attribute(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list)
{
    p_list->head.column_count = 0;

    klb_map_init(&p_list->data_map);

    p_list->sel = -1;
    p_list->enable_sel = false;
}

static void klbwnd_listex_quit_attribute(klbwnd_listex_t* p_list)
{
    klb_map_quit(&p_list->data_map);

    for (int i = 0; i < KLBWND_LISTEX_column_max; i++)
    {
        KLB_FREE_BY(p_list->head.column[i].title, sdsfree);
        KLB_FREE_BY(p_list->head.column[i].p_child, klb_map_destroy);
    }

    KLB_FREE_BY(p_list->tmp, sdsfree);
}

//////////////////////////////////////////////////////////////////////////
// css init / quit

void klbwnd_listex_css_init(klbwnd_listex_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbuicssex_attributes_init(&p_css->normal, &p_default->normal);
    klbuicssex_attributes_init(&p_css->focus, &p_default->focus);
    klbuicssex_attributes_init(&p_css->disable, &p_default->disable);

    // 行控件 CSS
    klbwnd_listex_row_css_init(&p_css->css_listex_row, p_gui);

    // check
    klbwnd_check_css_init(&p_css->css_check, p_gui);

    // 垂直滚动条
    klbwnd_vscrollbar_css_init(&p_css->css_vscrollbar, p_gui);

    p_css->css_vscrollbar.normal.border.width.top = 0;
    p_css->css_vscrollbar.normal.border.width.right = 0;
    p_css->css_vscrollbar.normal.border.width.bottom = 0;

    p_css->css_vscrollbar.focus.border.width.top = 0;
    p_css->css_vscrollbar.focus.border.width.right = 0;
    p_css->css_vscrollbar.focus.border.width.bottom = 0;

    p_css->css_vscrollbar.disable.border.width.top = 0;
    p_css->css_vscrollbar.disable.border.width.right = 0;
    p_css->css_vscrollbar.disable.border.width.bottom = 0;
}

void klbwnd_listex_css_quit(klbwnd_listex_css_t* p_css)
{
    klbuicssex_attributes_quit(&p_css->normal);
    klbuicssex_attributes_quit(&p_css->focus);
    klbuicssex_attributes_quit(&p_css->disable);

    // 行控件 CSS
    klbwnd_listex_row_css_quit(&p_css->css_listex_row);

    // check
    klbwnd_check_css_quit(&p_css->css_check);

    // 垂直滚动条
    klbwnd_vscrollbar_css_quit(&p_css->css_vscrollbar);
}

void klbwnd_listex_css_copy(klbwnd_listex_css_t* p_dst, klbwnd_listex_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbuicssex_attributes_copy(&p_dst->normal, &p_src->normal);
    klbuicssex_attributes_copy(&p_dst->focus, &p_src->focus);
    klbuicssex_attributes_copy(&p_dst->disable, &p_src->disable);

    // 行控件 CSS
    klbwnd_listex_row_css_copy(&p_dst->css_listex_row, &p_src->css_listex_row);

    // check
    klbwnd_check_css_copy(&p_dst->css_check, &p_src->css_check);

    // 垂直滚动条
    klbwnd_vscrollbar_css_copy(&p_dst->css_vscrollbar, &p_src->css_vscrollbar);

}

//////////////////////////////////////////////////////////////////////////
// init / quit

static void klbwnd_listex_init_subwnds(klb_wnd_t* p_wnd, klbwnd_listex_t* p_list)
{
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_parent;

    int head_h = 42;
    int row_h = 32;

    // 全选 选中框check 
    p_list->w0 = row_h;
    int w_check = (row_h * 2) / 3, h_check = w_check;

    p_list->p_check = klbwnd_check_create(p_wnd->p_gui, (p_list->w0 - w_check) / 2, (head_h - h_check) / 2, w_check, h_check);
    klb_wnd_push_child(p_wnd, p_list->p_check);
    klb_wnd_bind_command(p_list->p_check, on_command_check_all_klbwnd_listex, p_wnd);

    if (p_list->p_css)
    {
        klbwnd_check_set_css(p_list->p_check, &p_list->p_css->css_check);
    }

    // 右侧滚动条
    p_list->p_vscrollbar = klbwnd_vscrollbar_create(p_wnd->p_gui, 0, head_h, 20, p_rect->h - head_h);
    klb_wnd_push_child(p_wnd, p_list->p_vscrollbar);
    klb_wnd_show(p_list->p_vscrollbar, false);

    klb_wnd_bind_command(p_list->p_vscrollbar, on_command_vscrollbar_klbwnd_listex, p_wnd);

    if (p_list->p_css)
    {
        klbwnd_vscrollbar_set_css(p_list->p_vscrollbar, &p_list->p_css->css_vscrollbar);
    }
}

void klbwnd_listex_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_listex_on_control;    // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_NOFOCUS;

    // 初始化属性
    klbwnd_listex_init_attribute(p_wnd, p_list);

    // 初始化子窗口
    klbwnd_listex_init_subwnds(p_wnd, p_list);
}

void klbwnd_listex_quit(klb_wnd_t* p_wnd)
{
    klbwnd_listex_t* p_list = (klbwnd_listex_t*)p_wnd->ctrl;

    klb_map_clear(&p_list->data_map);
    klbwnd_listex_quit_attribute(p_list);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_listex_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_listex_t));

    klbwnd_listex_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_listex_destroy;

    return p_wnd;
}
