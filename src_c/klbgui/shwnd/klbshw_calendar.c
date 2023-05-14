// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/shwnd/klbshw_calendar.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_rect.h"
#include "klbgui/wnd/klbwnd_button.h"
#include "klbgui/klb_gui.h"


typedef struct klbshw_calendar_t_
{
    sds                 title;  ///< 标题, 显示值
}klbshw_calendar_t;


//////////////////////////////////////////////////////////////////////////

static klb_wnd_t* klbshw_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h);
static void klbshw_calendar_quit_attribute(klbshw_calendar_t* p_calendar);
static void klbshw_calendar_popup_end(klb_wnd_t* p_wnd_menu, klbshw_calendar_t* p_calendar, bool ok);


static void klbshw_calendar_destroy(klb_wnd_t* p_wnd)
{
    klbshw_calendar_t* p_calendar = (klbshw_calendar_t*)p_wnd->ctrl;

    klbshw_calendar_quit_attribute(p_calendar);

    KLB_FREE(p_wnd);
}

static void klbshw_calendar_on_paint_status(klb_wnd_t* p_wnd, klbshw_calendar_t* p_calendar, klbuicssex_attributes_t* p_attr, klb_rect_t* p_rect)
{
    // 背景
    klb_wnd_draw_fill_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 31, 41, 21));

    // 字符串背景
    int text_h = 42;
    klb_rect_t text_rect = { p_rect->x, p_rect->y, p_rect->w, text_h };
    klb_wnd_draw_fill_rect2(p_wnd, &text_rect, KLB_ARGB8888(255, 45, 45, 48));

    klb_wnd_draw_rect2(p_wnd, p_rect, KLB_ARGB8888(255, 81, 81, 81));

    // 字符串
    int offset = 8;
    text_rect.x += offset;
    text_rect.w -= offset * 2;

    klbuicssex_draw_text(p_wnd, p_calendar->title, &text_rect, &p_attr->border, NULL, &p_attr->text, &p_attr->font);
}

static int klbshw_calendar_on_paint(klb_wnd_t* p_wnd)
{
    klbshw_calendar_t* p_calendar = (klbshw_calendar_t*)p_wnd->ctrl;
    klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;

    if (KLB_WND_STATUS_HIDE & p_wnd->state.status)
    {
        return 0;
    }

    // 绘图区域
    klb_rect_t paint_rect = *p_rect;

    //klbshw_calendar_on_paint_status(p_wnd, p_calendar, &p_calendar->normal, &paint_rect);

    return 0;
}

static int klbshw_calendar_on_control(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klbshw_calendar_t* p_calendar = (klbshw_calendar_t*)p_wnd->ctrl;

    switch (msg)
    {
    case KLBUI_onpaint:
        return klbshw_calendar_on_paint(p_wnd);
    case KLBUI_click:
    case KLBUI_dblclick:
    case KLBUI_mousedown:
    {
        klb_rect_t* p_rect = &p_wnd->pos.rect_in_canvas;
        if (!klb_pt_in_rect(p_rect, p_pt1->x, p_pt1->y))
        {
            klbshw_calendar_popup_end(p_wnd, p_calendar, false);
        }
    }
    break;
    default:
        break;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// export

/// @brief 获取日历顶层对话框(菜单)
klb_wnd_t* klbui_shwnd_get_calendar(klb_gui_t* p_gui)
{
    klb_wnd_t* p_wnd = klb_gui_get_shwnd(p_gui, KLBSHW_calendar);

    if (NULL == p_wnd)
    {
        int w = 0, h = 0;
        klbshw_calendar_wh(p_gui, &w, &h);

        p_wnd = klbshw_calendar_create(p_gui, 0, 0, w, h);

        int ret = klb_gui_push_shwnd(p_gui, KLBSHW_calendar, p_wnd);
        assert(0 == ret);
    }

    return p_wnd;
}

/// @brief 获取所需要的宽高
int klbshw_calendar_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{
    if (p_out_w) { *p_out_w = 320; }
    if (p_out_h) { *p_out_h = 240; }

    return 0;
}

int klbshw_calendar_bind(klb_wnd_t* p_wnd, klbshw_calendar_cb cb, void* ptr)
{
    klbshw_calendar_t* p_calendar = (klbshw_calendar_t*)p_wnd->ctrl;

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 内部私有函数

// 结束
static void klbshw_calendar_popup_end(klb_wnd_t* p_wnd, klbshw_calendar_t* p_calendar, bool ok)
{

    // 结束弹出
    klb_gui_popup_end(p_wnd->p_gui, true);
}

static int klbshw_calendar_init_subwnd(klb_wnd_t* p_wnd, klbshw_calendar_t* p_calendar, klb_gui_t* p_gui)
{

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 初始化属性默认值

static void klbshw_calendar_init_attribute(klb_wnd_t* p_wnd, klbshw_calendar_t* p_calendar)
{
    const klbui_default_t* p_default = klb_gui_get_std_default(p_wnd->p_gui);

}

static void klbshw_calendar_quit_attribute(klbshw_calendar_t* p_calendar)
{
    KLB_FREE_BY(p_calendar->title, sdsfree);
}

//////////////////////////////////////////////////////////////////////////


static klb_wnd_t* klbshw_calendar_create(klb_gui_t* p_gui, int x, int y, int w, int h)
{
    klb_wnd_t* p_wnd = KLB_MALLOCZ(klb_wnd_t, 1, sizeof(klbshw_calendar_t));
    klbshw_calendar_t* p_calendar = (klbshw_calendar_t*)p_wnd->ctrl;

    p_wnd->pos.rect_in_parent.x = x;
    p_wnd->pos.rect_in_parent.y = y;
    p_wnd->pos.rect_in_parent.w = w;
    p_wnd->pos.rect_in_parent.h = h;

    p_wnd->vtable.destroy = klbshw_calendar_destroy;
    p_wnd->vtable.on_control = klbshw_calendar_on_control;
    p_wnd->vtable.on_command = NULL;
    p_wnd->vtable.on_set = NULL;
    p_wnd->vtable.on_get = NULL;

    p_wnd->p_gui = p_gui;

    // 样式 style
    p_wnd->state.style = KLB_WND_STYLE_TOP | KLB_WND_STYLE_NOFOCUS;

    // css
    klbshw_calendar_init_attribute(p_wnd, p_calendar);

    // 子窗口
    klbshw_calendar_init_subwnd(p_wnd, p_calendar, p_gui);

    return p_wnd;
}
