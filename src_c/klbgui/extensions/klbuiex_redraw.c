#include "klbgui/extensions/klbuiex_redraw.h"
#include "klbutil/klb_hlist.h"
#include "klbmem/klb_mem.h"


#define KLB_GUIEX_redraw  "KLB-GUIEX-redraw"


typedef struct klbuiex_redraw_t_
{
    klb_hlist_t*        p_hlist;        ///< 待重绘列表
    bool                redraw_all;     ///< 重绘所有

    klb_gui_t*          p_gui;          ///< GUI对象
}klbuiex_redraw_t;


//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// create / destroy / loop once

static void* klbuiex_redraw_create(klb_gui_t* p_gui)
{
    klbuiex_redraw_t* p_re = KLB_MALLOCZ(klbuiex_redraw_t, 1, 0);

    p_re->p_gui = p_gui;

    p_re->p_hlist = klb_hlist_create(0);
    p_re->redraw_all = false;

    return p_re;
}

static void klbuiex_redraw_destroy(void* ptr, klb_gui_t* p_gui)
{
    klbuiex_redraw_t* p_re = (klbuiex_redraw_t*)ptr;

    // 清空
    klbuiex_redraw_clear(p_re);

    KLB_FREE_BY(p_re->p_hlist, klb_hlist_destroy);
    KLB_FREE(p_re)
}

//////////////////////////////////////////////////////////////////////////
// 私有

// 自身\父窗口是否在列表中
static bool is_in_hlist_klbuiex_redraw(klbuiex_redraw_t* p_re, klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_cur = p_wnd;
    while (NULL != p_cur)
    {
        if (NULL != klb_hlist_find_iter(p_re->p_hlist, &p_cur, sizeof(klb_wnd_t*)))
        {
            return true;
        }

        p_cur = p_cur->p_parent;
    }


    return false;
}

// 窗口是否为隐藏的
static bool is_hide_wnd_klbuiex_redraw(klbuiex_redraw_t* p_re, klb_wnd_t* p_wnd)
{
    // 检查自身及整个窗口链
    // 若父(祖)级处于隐藏状态, 则也为隐藏

    klb_wnd_t* p_cur = p_wnd;
    while (NULL != p_cur)
    {
        if (p_cur->state.status & KLB_WND_STATUS_HIDE)
        {
            return true;
        }

        p_cur = p_cur->p_parent;
    }


    return false;
}

// 唯一性
static void check_only_one_wnd_klbuiex_redraw(klbuiex_redraw_t* p_re)
{
    // 检查所有父窗口是否有刷新
    // 若有, 则不需要绘制本窗口
    // 若无, 则需要绘制本窗口
    // 保证在窗口链表中, 处于最顶层(父层)的窗口记录刷新, 避免多余的绘制

    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_re->p_hlist);

    while (NULL != p_iter)
    {
        klb_hlist_iter_t* p_cur = p_iter;
        p_iter = klb_hlist_next(p_iter);

        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_data(p_cur);
        klb_wnd_t* p_parent = p_wnd->p_parent;

        if (NULL != p_parent && is_in_hlist_klbuiex_redraw(p_re, p_parent))
        {
            // 存在父窗口, 且有父窗口记录
            // 则剔除当前窗口
            klb_hlist_remove(p_re->p_hlist, p_cur);
        }
        else if(is_hide_wnd_klbuiex_redraw(p_re, p_wnd))
        {
            // 处于隐藏状态, 则剔除当前窗口
            klb_hlist_remove(p_re->p_hlist, p_cur);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// 导出, export


void klbuiex_redraw_clear(klbuiex_redraw_t* p_re)
{
    klb_hlist_clear(p_re->p_hlist, NULL, NULL);
    p_re->redraw_all = false;
}

void klbuiex_redraw_push(klbuiex_redraw_t* p_re, klb_wnd_t* p_wnd)
{
    if (is_in_hlist_klbuiex_redraw(p_re, p_wnd))
    {
        return; // 已经有记录, 则无需添加
    }

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_re->p_hlist, &p_wnd, sizeof(klb_wnd_t*), p_wnd);
    assert(NULL != p_iter);
}

void klbuiex_redraw_all(klbuiex_redraw_t* p_re)
{
    klbuiex_redraw_clear(p_re); // 清空记录

    p_re->redraw_all = true;
}

bool klbuiex_redraw_need_repaint(klbuiex_redraw_t* p_re, bool* p_redraw_all)
{
    bool ret = false;

    // redraw_all / 或有记录, 则需要重新绘制
    int count = klb_hlist_size(p_re->p_hlist);
    
    if (p_re->redraw_all || 0 < count)
    {
        ret = true;

        if (0 < count)
        {
            // Fixed Bug. [2025] 这里还有一种情况: 放入刷新列表的窗口, 根本不在 所有需要绘制的窗口链中
            // 在本模块中无法处理这种情况, 需要到真正重绘时, 检查其顶层窗口是否需要绘制.

            // 存在先放子窗口, 再放父窗口的情况
            // 所有需要一次性清理一下
            check_only_one_wnd_klbuiex_redraw(p_re);
        }
    } 

    if (NULL != p_redraw_all) { *p_redraw_all = p_re->redraw_all; }

    return ret;
}

klb_hlist_t* klbuiex_redraw_get_hlist(klbuiex_redraw_t* p_re)
{
    return p_re->p_hlist;
}

//////////////////////////////////////////////////////////////////////////
// register / get

klbuiex_redraw_t* klbuiex_get_redraw(klb_gui_t* p_gui)
{
    return (klbuiex_redraw_t*)klb_gui_get_extension(p_gui, KLB_GUIEX_redraw);
}

int klbuiex_register_redraw(klb_gui_t* p_gui)
{
    klb_gui_extension_t ex = { 0 };

    ex.cb_create = klbuiex_redraw_create;
    ex.cb_destroy = klbuiex_redraw_destroy;
    ex.cb_loop_once = NULL;

    klb_gui_register_extension(p_gui, KLB_GUIEX_redraw, &ex);

    return 0;
}
