// Doc Encode : UTF-8 BOM, Unix(LF)
#include "klbgui/extensions/klbuiex_flex.h"
#include "klbgui/klb_wnd.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"


#define KLB_GUIEX_flex   "KLB-GUIEX-flex"


/// @struct klbuiex_flex_t_
/// @brief  flex 自动布局扩展
typedef struct klbuiex_flex_t_
{
    klb_gui_t*      p_gui;              ///< GUI 模块

    klb_hlist_t*    p_hlist;            ///< 需要自动布局的窗口列表
}klbuiex_flex_t;


//////////////////////////////////////////////////////////////////////////
// create / destroy / loop once

static void* klbuiex_flex_create(klb_gui_t* p_gui)
{
    klbuiex_flex_t* p_ex = KLB_MALLOCZ(klbuiex_flex_t, 1, 0);

    p_ex->p_gui = p_gui;

    p_ex->p_hlist = klb_hlist_create(0);

    return p_ex;
}

static void klbuiex_flex_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_flex_t* p_ex = (klbuiex_flex_t*)ptr;

    klbuiex_flex_clear(p_ex);

    KLB_FREE_BY(p_ex->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_ex)
}

//////////////////////////////////////////////////////////////////////////
// 私有

// flow 是否为 AUTO (从 x/y 填满至父 content 右/下边)
static bool is_flex_flow_auto_klbuiex_flex(int flex_val)
{
    klb_wnd_flex_tag_e tag = klbuiex_flex_get_tag(flex_val);

    if (KLB_WND_FLEX_TAG_FLOW != tag)
    {
        return false;
    }

    if (KLB_WND_FLEX_FLOW_AUTO == klbuiex_flex_get_payload_flow(flex_val))
    {
        return true;
    }

    return false;
}

// flex 声明 x/y -> 像素坐标 (列布局 content 原点 + PX 偏移)
static int flex_pos_px_klbuiex_flex(int content_origin, int flex_val)
{
    if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(flex_val))
    {
        return content_origin + klbuiex_flex_get_payload_px(flex_val);
    }

    return content_origin;
}

// AUTO 尺寸: content_size - item_pos
static int flex_dim_auto_remain_klbuiex_flex(int content_size, int item_pos)
{
    int remain = content_size - item_pos;

    if (0 > remain)
    {
        remain = 0;
    }

    return remain;
}

// 主轴(flow)维度是否参与均分剩余空间
static bool is_flex_main_grow_klbuiex_flex(int flex_val)
{
    klb_wnd_flex_tag_e tag = klbuiex_flex_get_tag(flex_val);

    if (KLB_WND_FLEX_TAG_FLOW != tag)
    {
        return false;
    }

    klb_wnd_flex_flow_e flow = klbuiex_flex_get_payload_flow(flex_val);

    if (KLB_WND_FLEX_FLOW_GROW == flow || KLB_WND_FLEX_FLOW_FILL == flow)
    {
        return true;
    }

    return false;
}

// 交叉轴(flow)维度是否拉满 content
static bool is_flex_cross_stretch_klbuiex_flex(int flex_val)
{
    klb_wnd_flex_tag_e tag = klbuiex_flex_get_tag(flex_val);

    if (KLB_WND_FLEX_TAG_FLOW != tag)
    {
        return false;
    }

    klb_wnd_flex_flow_e flow = klbuiex_flex_get_payload_flow(flex_val);

    if (KLB_WND_FLEX_FLOW_STRETCH == flow || KLB_WND_FLEX_FLOW_FILL == flow)
    {
        return true;
    }

    return false;
}

// 列方向 BoxFlow: 父窗口直接子节点 rect_in_flex -> rect_in_parent
static int layout_children_column_klbuiex_flex(klb_wnd_t* p_parent)
{
    int content_x = 0;
    int content_y = 0;
    int content_w = p_parent->pos.rect_in_parent.w;
    int content_h = p_parent->pos.rect_in_parent.h;
    int gap = 0;

    int fixed_main = 0;
    int grow_count = 0;
    klb_wnd_t* p_child = p_parent->p_child;

    while (NULL != p_child)
    {
        if (!klb_wnd_is_hide(p_child) && (KLB_WND_STYLE_FLEX & p_child->state.style))
        {
            klb_rect_t* p_flex = &p_child->pos.rect_in_flex;

            if (is_flex_main_grow_klbuiex_flex(p_flex->h))
            {
                grow_count += 1;
            }
            else if (is_flex_flow_auto_klbuiex_flex(p_flex->h))
            {
                int item_y = flex_pos_px_klbuiex_flex(content_y, p_flex->y);

                fixed_main += flex_dim_auto_remain_klbuiex_flex(content_h, item_y);
            }
            else if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(p_flex->h))
            {
                fixed_main += klbuiex_flex_get_payload_px(p_flex->h);
            }
            else if (0 < p_child->pos.rect_in_parent.h)
            {
                fixed_main += p_child->pos.rect_in_parent.h;
            }
        }

        p_child = p_child->p_next;
    }

    int remain_main = content_h - fixed_main;

    if (0 > remain_main)
    {
        remain_main = 0;
    }

    int grow_unit = 0;

    if (0 < grow_count)
    {
        grow_unit = remain_main / grow_count;
    }

    int main_cursor = content_y;
    p_child = p_parent->p_child;

    while (NULL != p_child)
    {
        if (!klb_wnd_is_hide(p_child) && (KLB_WND_STYLE_FLEX & p_child->state.style))
        {
            klb_rect_t* p_flex = &p_child->pos.rect_in_flex;
            int item_x = content_x;
            int item_y = main_cursor;
            int item_w = content_w;
            int item_h = 0;

            if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(p_flex->x))
            {
                item_x = content_x + klbuiex_flex_get_payload_px(p_flex->x);
            }

            if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(p_flex->y))
            {
                item_y = content_y + klbuiex_flex_get_payload_px(p_flex->y);
            }

            if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(p_flex->w))
            {
                item_w = klbuiex_flex_get_payload_px(p_flex->w);
            }
            else if (is_flex_flow_auto_klbuiex_flex(p_flex->w))
            {
                item_w = flex_dim_auto_remain_klbuiex_flex(content_w, item_x);
            }
            else if (!is_flex_cross_stretch_klbuiex_flex(p_flex->w) && 0 < p_child->pos.rect_in_parent.w)
            {
                item_w = p_child->pos.rect_in_parent.w;
            }

            if (is_flex_main_grow_klbuiex_flex(p_flex->h))
            {
                item_h = grow_unit;
            }
            else if (is_flex_flow_auto_klbuiex_flex(p_flex->h))
            {
                item_h = flex_dim_auto_remain_klbuiex_flex(content_h, item_y);
            }
            else if (KLB_WND_FLEX_TAG_PX == klbuiex_flex_get_tag(p_flex->h))
            {
                item_h = klbuiex_flex_get_payload_px(p_flex->h);
            }
            else if (0 < p_child->pos.rect_in_parent.h)
            {
                item_h = p_child->pos.rect_in_parent.h;
            }

            klb_wnd_move(p_child, item_x, item_y);
            klb_wnd_resize(p_child, item_w, item_h);

            if (KLB_WND_FLEX_TAG_PX != klbuiex_flex_get_tag(p_flex->y))
            {
                main_cursor = item_y + item_h + gap;
            }
        }

        p_child = p_child->p_next;
    }

    return 0;
}

// 处理单个窗口的 rect_in_flex -> rect_in_parent 流程
static int do_parent_rect_klbuiex_flex(klbuiex_flex_t* p_ex, klb_wnd_t* p_wnd)
{
    assert(NULL != p_ex);
    assert(NULL != p_wnd);

    if (0 == (p_wnd->state.status & KLB_WND_STATUS_FLEX_DIRTY))
    {
        return 0;
    }

    // step1. 列方向 BoxFlow: 直接子节点 rect_in_flex -> rect_in_parent
    layout_children_column_klbuiex_flex(p_wnd);

    // step2. 递归: 子窗口若仍为 dirty 容器, 继续布局
    klb_wnd_t* p_child = p_wnd->p_child;

    while (NULL != p_child)
    {
        if (0 != (p_child->state.status & KLB_WND_STATUS_FLEX_DIRTY))
        {
            do_parent_rect_klbuiex_flex(p_ex, p_child);
        }

        p_child = p_child->p_next;
    }

    // step3. 取消 dirty
    p_wnd->state.status &= ~(uint32_t)(KLB_WND_STATUS_FLEX_DIRTY);

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 导出, export

int klbuiex_flex_push(klbuiex_flex_t* p_ex, klb_wnd_t* p_wnd)
{
    assert(NULL != p_ex);
    assert(NULL != p_wnd);

    // KLB_WND_STATUS_FLEX_DIRTY 
    p_wnd->state.status |= KLB_WND_STATUS_FLEX_DIRTY;

    if (NULL != klb_hlist_find_iter(p_ex->p_hlist, &p_wnd, sizeof(klb_wnd_t*)))
    {
        return 1; // 已在列表中
    }

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_ex->p_hlist, &p_wnd, sizeof(klb_wnd_t*), p_wnd);
    assert(NULL != p_iter);

    return 0;
}

int klbuiex_flex_remove(klbuiex_flex_t* p_ex, klb_wnd_t* p_wnd)
{
    assert(NULL != p_ex);
    assert(NULL != p_wnd);

    if (NULL != klb_hlist_remove_bykey(p_ex->p_hlist, &p_wnd, sizeof(klb_wnd_t*)))
    {
        return 0;
    }

    return 1; // 未找到
}

int klbuiex_flex_clear(klbuiex_flex_t* p_ex)
{
    assert(NULL != p_ex);

    klb_hlist_clear(p_ex->p_hlist, NULL, NULL);

    return 0;
}

int klbuiex_flex_do_parent_rect(klbuiex_flex_t* p_ex)
{
    assert(NULL != p_ex);

    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_ex->p_hlist);

    while (NULL != p_iter)
    {
        klb_hlist_iter_t* p_cur = p_iter;
        p_iter = klb_hlist_next(p_iter);

        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_data(p_cur);

        do_parent_rect_klbuiex_flex(p_ex, p_wnd);

        klb_hlist_remove(p_ex->p_hlist, p_cur);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 其他

klb_wnd_flex_tag_e klbuiex_flex_get_tag(int flex_val)
{
    return (klb_wnd_flex_tag_e)((unsigned int)flex_val >> KLB_WND_FLEX_TAG_SHIFT);
}

unsigned int klbuiex_flex_get_payload_u(int flex_val)
{
    return (unsigned int)flex_val & KLB_WND_FLEX_PAYLOAD_MASK;
}

int klbuiex_flex_get_payload_px(int flex_val)
{
    unsigned int payload_u = klbuiex_flex_get_payload_u(flex_val);
    int px = (int)payload_u;

    if (0 != (px & 0x00800000))
    {
        px |= (int)0xFF000000;
    }

    return px;
}

klb_wnd_flex_flow_e klbuiex_flex_get_payload_flow(int flex_val)
{
    return (klb_wnd_flex_flow_e)klbuiex_flex_get_payload_u(flex_val);
}

// flex 声明分量 -> 创建窗口用的像素 (FLOW 等策略由后续布局计算)
static int flex_dim_to_create_px_klbuiex_flex(int flex_val)
{
    klb_wnd_flex_tag_e tag = klbuiex_flex_get_tag(flex_val);

    if (KLB_WND_FLEX_TAG_PX == tag)
    {
        return klbuiex_flex_get_payload_px(flex_val);
    }
    else if (KLB_WND_FLEX_TAG_FLOW == tag)
    {
        return 0;
    }

    // KLB_WND_FLEX_TAG_NONE: 未编码, 视为普通像素
    return flex_val;
}

bool klbuiex_flex_is_flex(int x, int y, int w, int h, uint32_t style, int* p_out_x, int* p_out_y, int* p_out_w, int* p_out_h)
{
    if (0 == (style & KLB_WND_STYLE_FLEX))
    {
        *p_out_x = x;
        *p_out_y = y;
        *p_out_w = w;
        *p_out_h = h;
        return false;
    }

    *p_out_x = flex_dim_to_create_px_klbuiex_flex(x);
    *p_out_y = flex_dim_to_create_px_klbuiex_flex(y);
    *p_out_w = flex_dim_to_create_px_klbuiex_flex(w);
    *p_out_h = flex_dim_to_create_px_klbuiex_flex(h);

    return true;
}


//////////////////////////////////////////////////////////////////////////
// register / get

klbuiex_flex_t* klbuiex_get_flex(klb_gui_t* p_gui)
{
    return (klbuiex_flex_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_flex);
}

int klbuiex_register_flex(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_flex_create;
    ex.cb_destroy = klbuiex_flex_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLB_GUIEX_flex, &ex);

    return 0;
}

// end
