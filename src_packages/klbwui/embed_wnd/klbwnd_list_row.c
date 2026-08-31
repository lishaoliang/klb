// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbwui/embed_wnd/klbwnd_list_row.h"
#include "klbwui/core/klbwui_css.h"
#include "klbwui/embed_wnd/klbwnd_list.h"
#include "klbgui/klb_gui.h"
#include "klbmem/klb_mem.h"


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void klbwnd_list_row_deinit_attribute(klbwnd_list_row_t* p_list_row);


//////////////////////////////////////////////////////////////////////////
// 继承重写方法

static void klbwnd_list_row_destroy(klb_wnd_t* p_wnd)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    klbwnd_list_row_deinit(p_wnd);

    KLB_FREE(p_wnd);
}


static void get_column_title_klbwnd_list_row(sds* p_dst, klb_map_t* p_row_map, int column)
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

static void klbwnd_list_row_on_paint_status(klb_wnd_t* p_wnd, klbwnd_list_row_t* p_list_row, klbwnd_list_row_css_t* p_css, klbwnd_list_row_attributes_t* p_attr, klb_rect_t* p_rect, uint32_t background_color2)
{
    if (p_list_row->check)
    {
        // 选中状态
        klb_wnd_draw_fill_rect2(p_wnd, p_rect, p_css->check_background_color);
    }
    else
    {
        // 纯色背景
        if (0 != p_list_row->idx % 2)
        {
            klb_wnd_draw_fill_rect2(p_wnd, p_rect, background_color2);
        }
        else
        {
            klbwuicss_draw_background(p_wnd, p_rect, &p_attr->background);
        }
    }

    // 边框
    //klbuicssex_draw_border(p_wnd, p_rect, &p_attr->border);


    // 绘制行数据
    klbwnd_list_head_t* p_head = p_list_row->p_head;
    klb_map_t* p_row_map = p_list_row->p_show_data;
    if (NULL != p_head && NULL != p_row_map)
    {
        int offx = p_rect->x;

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
            get_column_title_klbwnd_list_row(&p_list_row->title, p_row_map, i);
            klbuicssex_draw_text(p_wnd, p_list_row->title, &rect_column, NULL, NULL, &p_attr->text, &p_attr->font);
        }
    }
}

static int klbwnd_list_row_on_paint(klb_wnd_t* p_wnd)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    klbwnd_list_row_css_t* p_css = p_list_row->p_css;

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
        klbwnd_list_row_on_paint_status(p_wnd, p_list_row, p_css, &p_css->disable, &paint_rect, p_css->disable_background_color2);
    }
    else if (KLB_WND_STATUS_FOCUS & p_wnd->state.status)
    {
        klbwnd_list_row_on_paint_status(p_wnd, p_list_row, p_css, &p_css->focus, &paint_rect, p_css->focus_background_color2);
    }
    else
    {
        klbwnd_list_row_on_paint_status(p_wnd, p_list_row, p_css, &p_css->normal, &paint_rect, p_css->normal_background_color2);
    }

    return 0;
}


static int klbwnd_list_row_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbwnd_list_row_on_paint(p_wnd);

    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export 导出函数

void klbwnd_list_row_set_css(klb_wnd_t* p_wnd, klbwnd_list_row_css_t* p_css)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    p_list_row->p_css = p_css;
}

void klbwnd_list_row_set_index(klb_wnd_t* p_wnd, int idx)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    p_list_row->idx = idx;
}

void klbwnd_list_row_set_check(klb_wnd_t* p_wnd, bool check)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    p_list_row->check = check;
}

bool klbwnd_list_row_get_check(klb_wnd_t* p_wnd)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    return p_list_row->check;
}

void klbwnd_list_row_set_show_data(klb_wnd_t* p_wnd, klbwnd_list_head_t* p_head, klb_map_t* p_show_data)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    p_list_row->p_head = p_head;
    p_list_row->p_show_data = p_show_data;
}

//////////////////////////////////////////////////////////////////////////
// init / deinit attribute

static void klbwnd_list_row_init_attribute(klbwnd_list_row_t* p_list_row)
{
    p_list_row->check = false;
    p_list_row->title = sdsempty(); /* scratch only: column draw buffer, not business default sds */
}

static void klbwnd_list_row_deinit_attribute(klbwnd_list_row_t* p_list_row)
{
    KLB_FREE_BY(p_list_row->title, sdsfree);
}


//////////////////////////////////////////////////////////////////////////
// attributes init / deinit

static void klbwnd_list_row_attributes_init(klbwnd_list_row_attributes_t* p_attr, const klbuicssex_attributes_t* p_src)
{
    p_attr->text = p_src->text;
    p_attr->font = p_src->font;

    p_attr->background.image = NULL;
    p_attr->background.color = p_src->background.color;
    p_attr->background.image = klb_sds_assign(p_attr->background.image, p_src->background.image);
    p_attr->background.image_mode = p_src->background.image_mode;
    p_attr->background.image_flags = p_src->background.image_flags;

    p_attr->border = p_src->border;
}

static void klbwnd_list_row_attributes_deinit(klbwnd_list_row_attributes_t* p_attr)
{
    KLB_FREE_BY(p_attr->background.image, sdsfree);
}

static void klbwnd_list_row_attributes_copy(klbwnd_list_row_attributes_t* p_dst, const klbwnd_list_row_attributes_t* p_src)
{
    p_dst->text = p_src->text;
    p_dst->font = p_src->font;

    p_dst->background.color = p_src->background.color;
    p_dst->background.image = klb_sds_assign(p_dst->background.image, p_src->background.image);
    p_dst->background.image_mode = p_src->background.image_mode;
    p_dst->background.image_flags = p_src->background.image_flags;

    p_dst->border = p_src->border;
}


//////////////////////////////////////////////////////////////////////////
// css init / deinit

void klbwnd_list_row_css_init(klbwnd_list_row_css_t* p_css, klb_gui_t* p_gui)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_gui);

    p_css->margin = p_default->margin;
    p_css->padding = p_default->padding;

    klbwnd_list_row_attributes_init(&p_css->normal, &p_default->normal);
    klbwnd_list_row_attributes_init(&p_css->focus, &p_default->focus);
    klbwnd_list_row_attributes_init(&p_css->disable, &p_default->disable);

    p_css->normal_background_color2 = KLB_ARGB8888(255, 32, 32, 32);
    p_css->focus_background_color2 = KLB_ARGB8888(255, 32, 32, 32);
    p_css->disable_background_color2 = KLB_ARGB8888(255, 32, 32, 32);

    p_css->check_background_color = KLB_ARGB8888(255, 32, 32, 61);
}

void klbwnd_list_row_css_deinit(klbwnd_list_row_css_t* p_css)
{
    klbwnd_list_row_attributes_deinit(&p_css->normal);
    klbwnd_list_row_attributes_deinit(&p_css->focus);
    klbwnd_list_row_attributes_deinit(&p_css->disable);
}

void klbwnd_list_row_css_copy(klbwnd_list_row_css_t* p_dst, klbwnd_list_row_css_t* p_src)
{
    p_dst->margin = p_src->margin;
    p_dst->padding = p_src->padding;

    klbwnd_list_row_attributes_copy(&p_dst->normal, &p_src->normal);
    klbwnd_list_row_attributes_copy(&p_dst->focus, &p_src->focus);
    klbwnd_list_row_attributes_copy(&p_dst->disable, &p_src->disable);

    p_dst->normal_background_color2 = p_src->normal_background_color2;
    p_dst->focus_background_color2 = p_src->focus_background_color2;
    p_dst->disable_background_color2 = p_src->disable_background_color2;

    p_dst->check_background_color = p_src->check_background_color;
}

//////////////////////////////////////////////////////////////////////////
// init / deinit

void klbwnd_list_row_init(klb_wnd_t* p_wnd, klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = NULL;                           // 由继承者重写
    p_wnd->vtable.on_control = klbwnd_list_row_on_control;  // 这里指定
    p_wnd->vtable.on_command = NULL;                        // 由使用者绑定
    p_wnd->vtable.on_set = NULL;                            // 由继承者重写
    p_wnd->vtable.on_get = NULL;                            // 由继承者重写

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = 0x0;

    // 
    klbwnd_list_row_init_attribute(p_list_row);
}

void klbwnd_list_row_deinit(klb_wnd_t* p_wnd)
{
    klbwnd_list_row_t* p_list_row = (klbwnd_list_row_t*)p_wnd->ctrl;

    klbwnd_list_row_deinit_attribute(p_list_row);
}

//////////////////////////////////////////////////////////////////////////
// create

klb_wnd_t* klbwnd_list_row_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbwnd_list_row_t));

    klbwnd_list_row_init(p_wnd, p_gui, x, y, w, h);

    // 补写 destroy 函数
    p_wnd->vtable.destroy = klbwnd_list_row_destroy;

    return p_wnd;
}

// end
