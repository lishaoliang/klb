// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/extensions/klbuiex_wndticker.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_nlist.h"


#define KLBUIEX_WNDTICKER   "KLBUIEX-wndticker"


/// @struct klbuiex_wndticker_t
/// @brief  控件定时器 扩展; 需要平衡 使用的便利性 和 性能影响
///   \n    思路:
///   \n      本实现思路 实际是在 窗口树 之外 另开一个 需要处理定时器的 窗口列表
///   \n    使用: 
///   \n      1. 窗口 若含有 KLB_WND_STYLE_TICKER 标记, 则在加载之后, 放入 需要处理的 顶层窗口定时器 列表中
///   \n      2. 当整个 窗口树 处于激活(显示) 时, 会周期性 触发 控件 KLBUI_onticker 事件
///   \n    实现:
///   \n      1. 窗口在 激活(onload) 之后, 遍历窗口树 将含有 *_TICKER_*标记 的窗口 放入需要定时处理的列表中
///   \n      2. 在 *_loop_once() 函数中 定期 给控件 推 KLBUI_onticker 事件
///   \n      3. 窗口在 非激活(onunload) 之后, 清理定时器列表
///   \n    优点: 定时器随窗口 onload/onunload 降低维护成本
///   \n    缺点: 只能在窗口 激活 后, 才生效
///   \n    其他影响:
///   \n      !. 当前使用的 是 单图层实现, 若使用 定时器等 刷新, 会导致整个UI重绘
///   \n      !. 可以考虑 加入 popup / messagebox 图层 实现
typedef struct klbuiex_wndticker_t_
{
    klb_gui_t*          p_gui;              ///< GUI

    int64_t             tick_count;         ///< 计时
    int64_t             interval_tc;        ///< 间隔 单位毫秒

    // 窗口计数
    struct
    {
        int             modal_num;          ///< 当前 modal 窗口数
        int             popup_num;          ///< 当前 popup 窗口数
        int             msgbox_num;         ///< 当前 msgbox 窗口数
    };

    // 顶层窗口定时
    struct
    {
        klb_nlist_t*    p_modal_list[KLBUI_MODAL_WND_MAX];  ///< modal窗口需要处理的定时器窗口
        klb_nlist_t*    p_popup_list[KLBUI_POPUP_WND_MAX];  ///< popup窗口需要处理的定时器窗口
        klb_nlist_t*    p_msgbox_list;                      ///< msgbox窗口需要处理的定时器窗口
    };

}klbuiex_wndticker_t;


//////////////////////////////////////////////////////////////////////////
// 前置定义
static int do_ticker_list_klbuiex_wndticker(klbuiex_wndticker_t* p_wndticker, klb_nlist_t* p_list);
static void klbuiex_wndticker_clear(klbuiex_wndticker_t* p_wndticker);


//////////////////////////////////////////////////////////////////////////
// 适配

static void* klbuiex_wndticker_create(klb_gui_t* p_gui)
{
    klbuiex_wndticker_t* p_wndticker = KLB_MALLOCZ(klbuiex_wndticker_t, 1, 0);

    p_wndticker->p_gui = p_gui;

    p_wndticker->tick_count = klb_gui_get_tick_count(p_gui);
    p_wndticker->interval_tc = 500; // 单位毫秒

    p_wndticker->modal_num = 0;
    p_wndticker->popup_num = 0;
    p_wndticker->msgbox_num = 0;

    // modal 顶层窗口定时
    for (int i = 0; i < KLBUI_MODAL_WND_MAX; i++)
    {
        p_wndticker->p_modal_list[i] = klb_nlist_create();
    }

    // popup 顶层窗口定时
    for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
    {
        p_wndticker->p_popup_list[i] = klb_nlist_create();
    }

    // msgbox 顶层窗口定时
    p_wndticker->p_msgbox_list = klb_nlist_create();

    return p_wndticker;
}

static void klbuiex_wndticker_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_wndticker_t* p_wndticker = (klbuiex_wndticker_t*)ptr;

    // 清空
    klbuiex_wndticker_clear(p_wndticker);

    // modal 顶层窗口定时
    for (int i = 0; i < KLBUI_MODAL_WND_MAX; i++)
    {
        KLB_FREE_BY(p_wndticker->p_modal_list[i], klb_nlist_destroy);
    }

    // popup 顶层窗口定时
    for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
    {
        KLB_FREE_BY(p_wndticker->p_popup_list[i], klb_nlist_destroy);
    }

    // msgbox 顶层窗口定时
    KLB_FREE_BY(p_wndticker->p_msgbox_list, klb_nlist_destroy);

    KLB_FREE(p_wndticker)
}

/// @brief 控制操作消息
/// @note 当UI框架需要扩展处理事务时调用
static int klbuiex_wndticker_control(void* ptr, klb_gui_t* p_gui, int msg, uint8_t* p_param_in_out, int param_size)
{
    klbuiex_wndticker_t* p_wndticker = (klbuiex_wndticker_t*)ptr;

    if (KLBUI_EX_MSG_quit == msg)
    {
        klbuiex_wndticker_clear(p_wndticker);
    }
    else if (KLBUI_EX_MSG_clear == msg)
    {
        klbuiex_wndticker_clear(p_wndticker);
    }

    return 0;
}

static int klbuiex_wndticker_loop_once(void* ptr, klb_gui_t* p_gui, int64_t now)
{
    klbuiex_wndticker_t* p_wndticker = (klbuiex_wndticker_t*)ptr;

    // 定期 推 onticker 事件

    if (ABS_SUB(now, p_wndticker->tick_count) <= p_wndticker->interval_tc)
    {
        return 0;
    }

    p_wndticker->tick_count = now;

    // messagebox
    if (0 < p_wndticker->msgbox_num)
    {
        do_ticker_list_klbuiex_wndticker(p_wndticker, p_wndticker->p_msgbox_list);
    }

    // popup
    for (int j = 0; j < p_wndticker->popup_num; j++)
    {
        do_ticker_list_klbuiex_wndticker(p_wndticker, p_wndticker->p_popup_list[j]);
    }

    // modal
    for (int i = 0; i < p_wndticker->modal_num; i++)
    {
        do_ticker_list_klbuiex_wndticker(p_wndticker, p_wndticker->p_modal_list[i]);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 内部私有函数

// 推送 onticker 事件
static int do_ticker_list_klbuiex_wndticker(klbuiex_wndticker_t* p_wndticker, klb_nlist_t* p_list)
{
    assert(NULL != p_list);

    if (klb_nlist_size(p_list) <= 0)
    {
        return 0;
    }

    klb_nlist_iter_t* p_iter = klb_nlist_begin(p_list);

    while (NULL != p_iter)
    {
        klb_wnd_t* p_wnd = klb_nlist_data(p_iter);
        assert(NULL != p_wnd);

        // 窗口 onticker 事件
        // 这里 只触发 控件 事件, 由控件决定后续处理
        klb_wnd_call_control(p_wnd, KLBUI_onticker, NULL, NULL, 0, 0);

        p_iter = klb_nlist_next(p_iter);
    }

    return 0;
}

// 将需要处理定时器的 窗口加入链表
int do_push_list_klbuiex_wndticker(klbuiex_wndticker_t* p_wndticker, klb_nlist_t* p_list, klb_wnd_t* p_wnd)
{
    // 递归 遍历窗口
    if (NULL == p_wnd)
    {
        return 0;
    }

    // 带有 KLB_WND_STYLE_TICKER_TOP 样式, 则加入链表
    if (KLB_WND_STYLE_TICKER & p_wnd->state.style)
    {
        klb_nlist_push_tail(p_list, p_wnd);
    }

    // 遍历子窗口
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        do_push_list_klbuiex_wndticker(p_wndticker, p_list, p_next);

        p_next = p_next->p_next;
    }

    return 0;
}

static void klbuiex_wndticker_clear(klbuiex_wndticker_t* p_wndticker)
{
    // modal 顶层窗口定时
    for (int i = 0; i < KLBUI_MODAL_WND_MAX; i++)
    {
        klb_nlist_clear(p_wndticker->p_modal_list[i], NULL, NULL);
    }

    // popup 顶层窗口定时
    for (int i = 0; i < KLBUI_POPUP_WND_MAX; i++)
    {
        klb_nlist_clear(p_wndticker->p_popup_list[i], NULL, NULL);
    }

    // msgbox 顶层窗口定时
    klb_nlist_clear(p_wndticker->p_msgbox_list, NULL, NULL);

    // 计数
    p_wndticker->modal_num = 0;
    p_wndticker->popup_num = 0;
    p_wndticker->msgbox_num = 0;
}


//////////////////////////////////////////////////////////////////////////
// 对外接口

void klbuiex_wndticker_set_interval(klbuiex_wndticker_t* p_wndticker, int64_t interval)
{
    p_wndticker->interval_tc = interval;
}

int64_t klbuiex_wndticker_get_interval(klbuiex_wndticker_t* p_wndticker)
{
    return p_wndticker->interval_tc;
}

void klbuiex_wndticker_set_modal_num(klbuiex_wndticker_t* p_wndticker, int num)
{
    p_wndticker->modal_num = num;
}

void klbuiex_wndticker_set_popup_num(klbuiex_wndticker_t* p_wndticker, int num)
{
    p_wndticker->popup_num = num;
}

void klbuiex_wndticker_set_msgbox_num(klbuiex_wndticker_t* p_wndticker, int num)
{
    p_wndticker->msgbox_num = num;
}

int klbuiex_wndticker_modal(klbuiex_wndticker_t* p_wndticker, int idx, klb_wnd_t* p_top_wnd)
{
    assert(NULL != p_top_wnd);

    if (0 <= idx && idx < KLBUI_MODAL_WND_MAX)
    {
        klb_nlist_t* p_list = p_wndticker->p_modal_list[idx];
        assert(klb_nlist_size(p_list) <= 0);

        // 递归检查所有窗口, 将含有 定时器 样式 的窗口 加入到列表中
        do_push_list_klbuiex_wndticker(p_wndticker, p_list, p_top_wnd);
    }

    return 0;
}

int klbuiex_wndticker_modal_end(klbuiex_wndticker_t* p_wndticker, int idx)
{
    if (0 <= idx && idx < KLBUI_MODAL_WND_MAX)
    {
        klb_nlist_t* p_list = p_wndticker->p_modal_list[idx];

        // 清空
        klb_nlist_clear(p_list, NULL, NULL);
    }

    return 0;
}


int klbuiex_wndticker_popup(klbuiex_wndticker_t* p_wndticker, int idx, klb_wnd_t* p_top_wnd)
{
    assert(NULL != p_top_wnd);

    if (0 <= idx && idx < KLBUI_POPUP_WND_MAX)
    {
        klb_nlist_t* p_list = p_wndticker->p_popup_list[idx];
        assert(klb_nlist_size(p_list) <= 0);

        // 递归检查所有窗口, 将含有 定时器 样式 的窗口 加入到列表中
        do_push_list_klbuiex_wndticker(p_wndticker, p_list, p_top_wnd);
    }

    return 0;
}

int klbuiex_wndticker_popup_end(klbuiex_wndticker_t* p_wndticker, int idx)
{
    if (0 <= idx && idx < KLBUI_MODAL_WND_MAX)
    {
        klb_nlist_t* p_list = p_wndticker->p_popup_list[idx];

        // 清空
        klb_nlist_clear(p_list, NULL, NULL);
    }

    return 0;
}

int klbuiex_wndticker_msgbox(klbuiex_wndticker_t* p_wndticker, klb_wnd_t* p_top_wnd)
{
    assert(NULL != p_top_wnd);

    klb_nlist_t* p_list = p_wndticker->p_msgbox_list;
    assert(klb_nlist_size(p_list) <= 0);

    // 递归检查所有窗口, 将含有 定时器 样式 的窗口 加入到列表中
    do_push_list_klbuiex_wndticker(p_wndticker, p_list, p_top_wnd);

    return 0;
}

int klbuiex_wndticker_msgbox_end(klbuiex_wndticker_t* p_wndticker)
{
    klb_nlist_t* p_list = p_wndticker->p_msgbox_list;

    // 清空
    klb_nlist_clear(p_list, NULL, NULL);

    return 0;
}


//////////////////////////////////////////////////////////////////////////
// 注册/获取

klbuiex_wndticker_t* klbuiex_get_wndticker(klb_gui_t* p_gui)
{
    return (klbuiex_wndticker_t*)klb_gui_get_extension(p_gui, KLBUIEX_WNDTICKER);
}

int klbuiex_register_wndticker(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_wndticker_create;
    ex.cb_destroy = klbuiex_wndticker_destroy;
    ex.cb_control = klbuiex_wndticker_control;
    ex.cb_loop_once = klbuiex_wndticker_loop_once;

    klb_gui_register_extension(p_gui, KLBUIEX_WNDTICKER, &ex);

    return 0;
}

//end
