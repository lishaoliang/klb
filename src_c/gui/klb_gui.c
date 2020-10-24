#include "gui/klb_gui.h"
#include "gui/klb_gui_in.h"
#include "mem/klb_mem.h"
#include "log/klb_log.h"
#include "gui/klb_wnd.h"
#include "gui/klb_wnd_in.h"
#include "gui/klb_widget.h"
#include <assert.h>


static int klb_gui_thread(void* p_obj, int* p_run);

//////////////////////////////////////////////////////////////////////////
// klb_gui.h

klb_gui_t* klb_gui_create(klb_canvas_t* p_canvas)
{
    klb_gui_t* p_gui = KLB_MALLOC(klb_gui_t, 1, 0);
    KLB_MEMSET(p_gui, 0, sizeof(klb_gui_t));

    p_gui->p_canvas = p_canvas;

    p_gui->p_top_list = klb_list_create();
    p_gui->p_wnd_hlist = klb_hlist_create(100000);
    p_gui->p_wnd_type_hlist = klb_hlist_create(3000);

    p_gui->p_msg_list = klb_list_create();
    p_gui->p_msg_mutex = klb_mutex_create();

    // 注册标准窗口类型
    // 创建完成之后, 可注册自定义控件
    KLB_GUI_REGISTER_STD(p_gui);

    return p_gui;
}

static void klb_gui_destroy_wnd(klb_wnd_t* p_wnd)
{
    klb_wnd_t* p_next = p_wnd->p_child;
    while (NULL != p_next)
    {
        klb_wnd_t* p_cur = p_next;
        p_next = p_next->p_next;

        klb_gui_destroy_wnd(p_cur);
    }

    KLB_FREE_WND(p_wnd);
}

void klb_gui_destroy(klb_gui_t* p_gui)
{
    assert(NULL != p_gui);

    while (0 < klb_list_size(p_gui->p_msg_list))
    {
        klb_msg_t* p_msg = (klb_msg_t*)klb_list_pop_head(p_gui->p_msg_list);
        KLB_FREE(p_msg);
    }

    while (0 < klb_list_size(p_gui->p_top_list))
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klb_list_pop_tail(p_gui->p_top_list);
        klb_gui_destroy_wnd(p_wnd);
    }

    while (0 < klb_hlist_size(p_gui->p_wnd_type_hlist))
    {
        klb_hlist_pop_head(p_gui->p_wnd_type_hlist);
    }

    while (0 < klb_hlist_size(p_gui->p_wnd_hlist))
    {
        klb_hlist_pop_head(p_gui->p_wnd_hlist);
    }

    KLB_FREE_BY(p_gui->p_top_list, klb_list_destroy);
    KLB_FREE_BY(p_gui->p_wnd_type_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_gui->p_wnd_hlist, klb_hlist_destroy);

    KLB_FREE_BY(p_gui->p_msg_list, klb_list_destroy);
    KLB_FREE_BY(p_gui->p_msg_mutex, klb_mutex_destroy);
    KLB_FREE(p_gui);
}

int klb_gui_start(klb_gui_t* p_gui)
{
    assert(NULL == p_gui->p_thread1);

    p_gui->p_thread1 = klb_thread_create(klb_gui_thread, p_gui, -1, "klb gui");

    return (NULL != p_gui->p_thread1) ? 0 : 1;
}

void klb_gui_stop(klb_gui_t* p_gui)
{
    assert(NULL != p_gui);

    KLB_FREE_BY(p_gui->p_thread1, klb_thread_destroy);
}

int klb_gui_process_message(klb_gui_t* p_gui, uint64_t tick_count)
{
    int ret = 0;

    // 消息处理
    while (true)
    {
        if (0 != klb_gui_process_message_once(p_gui))
        {
            break; // 一次将队列中的所有消息处理完毕
        }
    }

    // 是否重绘
    klb_gui_redraw(p_gui);

    // 刷新
    klb_gui_refresh(p_gui);

    return ret;
}

void klb_gui_attach_canvas(klb_gui_t* p_gui, klb_canvas_t* p_canvas)
{
    assert(NULL != p_gui);

    p_gui->p_canvas = p_canvas;
}

void klb_gui_push(klb_gui_t* p_gui, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
{
    klb_msg_t* p_msg = KLB_MALLOC(klb_msg_t, 1, 0);
    KLB_MEMSET(p_msg, 0, sizeof(klb_msg_t));

    p_msg->msg = msg;
    p_msg->pt1.x = x1;
    p_msg->pt1.y = y1;
    p_msg->pt2.x = x2;
    p_msg->pt2.y = y2;
    p_msg->lparam = lparam;
    p_msg->wparam = wparam;

    klb_mutex_lock(p_gui->p_msg_mutex);
    klb_list_push_tail(p_gui->p_msg_list, p_msg);
    klb_mutex_unlock(p_gui->p_msg_mutex);
}

int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create)
{
    assert(NULL != p_gui);
    assert(NULL != p_type);
    assert(NULL != cb_create);

    int len = strlen(p_type);
    if (NULL == klb_hlist_push_tail(p_gui->p_wnd_type_hlist, p_type, len, (void*)cb_create))
    {
        return 1; // 放入失败
    }

    return 0;
}

static void klb_gui_split_path_name(const char* p_path_name, char** p_dir, int* p_dir_len)
{
    char *p = p_path_name ? strrchr(p_path_name, '/') : NULL;

    if (NULL != p)
    {
        *p_dir = (char*)p_path_name;
        *p_dir_len = p - p_path_name;
    }
    else
    {
        *p_dir = NULL;
        *p_dir_len = 0;
    }
}

int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, klb_wnd_t** p_out_wnd)
{
    klb_wnd_create_cb create = (klb_wnd_create_cb)klb_hlist_find(p_gui->p_wnd_type_hlist, p_type, strlen(p_type));
    if (NULL == create)
    {
        return 1;
    }

    char* p_dir = NULL;
    int dir_len = 0;

    klb_gui_split_path_name(p_path_name, &p_dir, &dir_len);
    if (NULL == p_dir)
    {
        return 2;
    }

    klb_wnd_t* p_wnd = NULL;
    int path_len = strlen(p_path_name);

    if (0 < dir_len)
    {
        // 路径中有父窗口
        klb_wnd_t* p_parent = (klb_wnd_t*)klb_hlist_find(p_gui->p_wnd_hlist, p_dir, dir_len);
        if (NULL == p_parent)
        {
            return 3;
        }

        if (NULL != klb_hlist_find(p_gui->p_wnd_hlist, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        p_wnd = create(x, y, w, h);
        klb_wnd_push_child(p_parent, p_wnd);
        klb_hlist_push_tail(p_gui->p_wnd_hlist, p_path_name, path_len, p_wnd);
    }
    else
    {
        // 路径中无父窗口
        // 根目录, 顶层窗口
        if (NULL != klb_hlist_find(p_gui->p_wnd_hlist, p_path_name, path_len))
        {
            return 4; // 已经存在
        }

        p_wnd = create(x, y, w, h);
        klb_wnd_set_top(p_wnd, p_gui);
        klb_hlist_push_tail(p_gui->p_wnd_hlist, p_path_name, path_len, p_wnd);

        klb_list_push_tail(p_gui->p_top_list, p_wnd);
    }

    p_gui->redraw = true;

    if (NULL != p_out_wnd)
    {
        *p_out_wnd = p_wnd;
    }
    return 0;
}

int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name)
{
    return 0;
}

int klb_gui_do_model(klb_gui_t* p_gui, const char* p_path_name)
{
    return 0;
}

int klb_gui_end_model(klb_gui_t* p_gui, const char* p_path_name)
{
    return 0;
}

int klb_gui_end_model_all(klb_gui_t* p_gui)
{
    return 0;
}

int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_find(p_gui->p_wnd_hlist, p_path_name, strlen(p_path_name));
    if (NULL == p_wnd)
    {
        return 1;
    }

    p_wnd->p_obj = p_obj;
    p_wnd->vtable.on_command = on_command;

    return 0;
}

int klb_gui_set(klb_gui_t* p_gui, const char* p_path_name, const char* p_json)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_find(p_gui->p_wnd_hlist, p_path_name, strlen(p_path_name));
    if (NULL == p_wnd)
    {
        return 1;
    }

    int ret = 1;
    if (p_wnd->vtable.on_set)
    {
        ret = p_wnd->vtable.on_set(p_wnd, p_json);
    }

    return ret;
}

char* klb_gui_get(klb_gui_t* p_gui, const char* p_path_name, const char* p_json)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klb_hlist_find(p_gui->p_wnd_hlist, p_path_name, strlen(p_path_name));
    if (NULL == p_wnd)
    {
        return NULL;
    }

    char* p_ret = NULL;
    if (p_wnd->vtable.on_get)
    {
        p_ret = p_wnd->vtable.on_get(p_wnd, p_json);
    }

    return p_ret;
}

//////////////////////////////////////////////////////////////////////////
// klb_gui_in.h

int klb_gui_pop_message(klb_gui_t* p_gui, klb_msg_t** p_msg)
{
    if (0 == klb_mutex_trylock(p_gui->p_msg_mutex))
    {
        klb_msg_t* p_pop = (klb_msg_t*)klb_list_pop_head(p_gui->p_msg_list);
        klb_mutex_unlock(p_gui->p_msg_mutex);

        if (NULL != p_pop)
        {
            *p_msg = p_pop;
            return 0;
        }
    }

    return 1;
}

static klb_wnd_t* klb_gui_find_focus(klb_gui_t* p_gui, int x, int y)
{
    klb_list_iter_t* p_iter = klb_list_end(p_gui->p_top_list);
    while (NULL != p_iter)
    {
        klb_wnd_t* p_top = (klb_wnd_t*)klb_list_data(p_iter);
        klb_wnd_t* p_focus = klb_wnd_pt_in(p_top, x, y);

        if (NULL != p_focus)
        {
            return p_focus;
        }

        p_iter = klb_list_prev(p_iter);
    }

    return NULL;
}

int klb_gui_dispatch_message(klb_gui_t* p_gui, klb_msg_t* p_msg)
{
    if (KLB_WM_MOUSEMOVE == p_msg->msg)
    {
        klb_wnd_t* p_focus = klb_gui_find_focus(p_gui, p_msg->pt1.x, p_msg->pt1.y);

        if (NULL != p_gui->p_focus && p_focus != p_gui->p_focus)
        {
            klb_wnd_set_focus(p_gui->p_focus, false);

            if (KLB_CANVAS_LOCK_OK(p_gui->p_canvas))
            {
                klb_wnd_draw(p_gui->p_focus);
                KLB_CANVAS_UNLOCK(p_gui->p_canvas);
            }

            klb_gui_update_rect(p_gui, NULL);
        }

        if (NULL != p_focus)
        {
            klb_wnd_set_focus(p_focus, true);

            if (KLB_CANVAS_LOCK_OK(p_gui->p_canvas))
            {
                klb_wnd_draw(p_focus);
                KLB_CANVAS_UNLOCK(p_gui->p_canvas);
            }

            klb_gui_update_rect(p_gui, NULL);
        }

        p_gui->p_focus = p_focus;
    }

    // 焦点窗口消息
    klb_wnd_t* p_wnd = p_gui->p_focus;
    if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
    {
        p_wnd->vtable.on_command(p_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
    }

    return 0;
}


int klb_gui_process_message_once(klb_gui_t* p_gui)
{
    klb_msg_t* p_msg = NULL;
    if (0 == klb_gui_pop_message(p_gui, &p_msg))
    {
        int ret = klb_gui_dispatch_message(p_gui, p_msg);

        KLB_FREE(p_msg);
        return 0; // 有消息处理
    }

    return 1;
}

int klb_gui_redraw(klb_gui_t* p_gui)
{
    if (p_gui->redraw)
    {
        if (KLB_CANVAS_LOCK_OK(p_gui->p_canvas))
        {
            klb_canvas_draw_clear(p_gui->p_canvas, 0, 0, p_gui->p_canvas->rect.w, p_gui->p_canvas->rect.h);

            klb_list_iter_t* p_iter = klb_list_end(p_gui->p_top_list);
            while (NULL != p_iter)
            {
                klb_wnd_t* p_wnd = (klb_wnd_t*)klb_list_data(p_iter);
                klb_wnd_draw(p_wnd);

                p_iter = klb_list_prev(p_iter);
            }

            KLB_CANVAS_UNLOCK(p_gui->p_canvas);
        }

        klb_gui_update_rect(p_gui, NULL);

        p_gui->redraw = false;
    }

    return 0;
}

int klb_gui_update_rect(klb_gui_t* p_gui, const klb_rect_t* p_rect)
{
    p_gui->refresh = true;
    return 0;
}

int klb_gui_refresh(klb_gui_t* p_gui)
{
    if (p_gui->refresh)
    {
        if (p_gui->p_canvas->vtable.refresh)
        {
            p_gui->p_canvas->vtable.refresh(p_gui->p_canvas, 0, 0, p_gui->p_canvas->rect.w, p_gui->p_canvas->rect.h);
        }

        p_gui->refresh = false;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static int test_dlg_btn_on_command(klb_wnd_t* p_wnd, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    switch (msg)
    {
    case KLB_WM_LBUTTONDOWN:
        KLB_LOG("klb_button_on_command\n");
        break;
    default:
        break;
    }

    return 0;
}

static int test_dlg(klb_gui_t* p_gui)
{
#if 0
    klb_wnd_t* p_dlg = klb_dialog_create(10, 10, 640, 480);

    klb_wnd_t* p_btn1 = klb_button_create(10, 10, 100, 24);
    klb_wnd_push_child(p_dlg, p_btn1);

    klb_wnd_t* p_btn2 = klb_button_create(10, 64, 100, 24);
    klb_wnd_push_child(p_dlg, p_btn2);


    klb_wnd_set_top(p_dlg, p_gui);
    klb_list_push_tail(p_gui->p_top_list, p_dlg);
#else
    klb_gui_append(p_gui, "kdialog", "/home", 10, 10, 640, 480, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn1", 10, 10, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn2", 10, 40, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn3", 10, 70, 100, 24, NULL);
    klb_gui_append(p_gui, "kbutton", "/home/btn4", 10, 100, 100, 24, NULL);

    klb_gui_bind_command(p_gui, "/home/btn4", test_dlg_btn_on_command, NULL);
#endif

    return 0;
}

static int klb_gui_thread(void* p_obj, int* p_run)
{
    KLB_LOG("start klb gui thread.\n");
    klb_gui_t* p_gui = (klb_gui_t*)(p_obj);

    while (*p_run)
    {
        klb_gui_process_message(p_gui, 0);

        klb_sleep(10);
    }

    KLB_LOG("stop klb gui thread.\n");
    return 0;
}
