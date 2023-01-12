// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_wnd_in.h"
#include "klbgui/klbui_widgets.h"
#include "klbgui/extensions/klbuiex_extensions.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
// klb_gui.h

klb_gui_t* klb_gui_create(klb_canvas_t* p_canvas)
{
    klb_gui_t* p_gui = KLB_MALLOCZ(klb_gui_t, 1, 0);

    p_gui->p_canvas = p_canvas;

    p_gui->p_extension_hlist = klb_hlist_create(0);
    p_gui->p_extension_activated_hlist = klb_hlist_create(0);

    p_gui->p_msg_list = klb_nlist_create();
    p_gui->p_msg_mutex = klb_mutex_create();

    klb_map_init(&p_gui->css_map);

    // 注册标准扩展
    KLBUIEX_register_extensions_std(p_gui);

    // 取得window hash指针
    p_gui->p_wndhash = klbuiex_get_wndhash(p_gui);

    // 注册标准窗口类型
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

static void klb_gui_quit_extensions(klb_gui_t* p_gui)
{
    // 退出已经激活的扩展
    while (0 < klb_hlist_size(p_gui->p_extension_activated_hlist))
    {
        klb_gui_extension_activated_t* p_activated = (klb_gui_extension_activated_t*)klb_hlist_pop_head(p_gui->p_extension_activated_hlist);
        p_activated->ex.cb_destroy(p_activated->ptr, p_gui); // 销毁

        KLB_FREE_BY(p_activated->name, sdsfree);
        KLB_FREE(p_activated);
    }

    // 退出注册的扩展
    while (0 < klb_hlist_size(p_gui->p_extension_hlist))
    {
        klb_gui_extension_t* p_extension = (klb_gui_extension_t*)klb_hlist_pop_head(p_gui->p_extension_hlist);
        KLB_FREE(p_extension);
    }
}

void klb_gui_destroy(klb_gui_t* p_gui)
{
    assert(NULL != p_gui);

    while (0 < klb_nlist_size(p_gui->p_msg_list))
    {
        klb_msg_t* p_msg = (klb_msg_t*)klb_nlist_pop_head(p_gui->p_msg_list);
        KLB_FREE(p_msg);
    }

    klb_gui_quit_extensions(p_gui);
    klb_map_quit(&p_gui->css_map);

    KLB_FREE_BY(p_gui->p_msg_list, klb_nlist_destroy);
    KLB_FREE_BY(p_gui->p_msg_mutex, klb_mutex_destroy);

    KLB_FREE_BY(p_gui->p_extension_activated_hlist, klb_hlist_destroy);
    KLB_FREE_BY(p_gui->p_extension_hlist, klb_hlist_destroy);

    KLB_FREE(p_gui);
}

/// @brief 注册gui扩展
int klb_gui_register_extension(klb_gui_t* p_gui, const char* p_name, const klb_gui_extension_t* p_extension)
{
    assert(NULL != p_gui);
    assert(NULL != p_name);
    assert(NULL != p_extension);
    assert(NULL != p_extension->cb_create);
    assert(NULL != p_extension->cb_destroy);

    size_t name_len = strlen(p_name);

    klb_gui_extension_t* p_tmp = KLB_MALLOCZ(klb_gui_extension_t, 1, 0);
    memcpy(p_tmp, p_extension, sizeof(klb_gui_extension_t));

    klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_gui->p_extension_hlist, p_name, name_len, p_tmp);
    if (NULL == p_iter)
    {
        KLB_FREE(p_tmp);

        KLB_LOG_E("register gui env extension error!name:[%s]\n", p_name);
        return 1; // 放入失败, 名称重复
    }

    return 0;
}

/// @brief 获取gui扩展
void* klb_gui_get_extension(klb_gui_t* p_gui, const char* p_name)
{
    assert(NULL != p_gui);
    assert(NULL != p_name);

    // 先从激活的里面找
    size_t name_len = strlen(p_name);
    klb_gui_extension_activated_t* p_activated = (klb_gui_extension_activated_t*)klb_hlist_find(p_gui->p_extension_activated_hlist, p_name, name_len);
    if (NULL != p_activated)
    {
        return p_activated->ptr;
    }

    // 未找到, 则激活
    klb_gui_extension_t* p_extension = (klb_gui_extension_t*)klb_hlist_find(p_gui->p_extension_hlist, p_name, name_len);
    if (NULL != p_extension)
    {
        klb_gui_extension_activated_t* p_tmp = KLB_MALLOCZ(klb_gui_extension_activated_t, 1, 0);

        memcpy(&p_tmp->ex, p_extension, sizeof(klb_gui_extension_t));
        p_tmp->name = sdsnewlen(p_name, name_len);

        p_tmp->ptr = p_tmp->ex.cb_create(p_gui);
        assert(NULL != p_tmp->ptr);

        klb_hlist_iter_t* p_iter = klb_hlist_push_tail(p_gui->p_extension_activated_hlist, p_name, name_len, p_tmp);
        assert(NULL != p_iter);

        return p_tmp->ptr;
    }

    return NULL;
}

/// @brief 获取标准控件的默认值指针
const klbui_default_t* klb_gui_get_std_default(klb_gui_t* p_gui)
{
    return klbuiex_default_get_value(klbuiex_get_default(p_gui));
}

/// @brief 附加到 klua_env_t*
int klb_gui_attach_klua_env(klb_gui_t* p_gui, klua_env_t* p_env)
{
    p_gui->p_klua_env = p_env;
    return 0;
}


/// @brief 获取 klua_env_t*
klua_env_t* klb_gui_get_klua_env(klb_gui_t* p_gui)
{
    return p_gui->p_klua_env;
}

int klb_gui_loop_once(klb_gui_t* p_gui, int64_t tc)
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

    // 依次处理, 激活的扩展
    if (0 < klb_hlist_size(p_gui->p_extension_activated_hlist))
    {
        klb_hlist_iter_t* p_iter = klb_hlist_begin(p_gui->p_extension_activated_hlist);
        while (NULL != p_iter)
        {
            klb_gui_extension_activated_t* p_activated = (klb_gui_extension_activated_t*)klb_hlist_data(p_iter);
            if (NULL != p_activated && p_activated->ex.cb_loop_once)
            {
                p_activated->ex.cb_loop_once(p_activated->ptr, p_gui, tc);
            }

            p_iter = klb_hlist_next(p_iter);
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

void klb_gui_push_msg(klb_gui_t* p_gui, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam)
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
    klb_nlist_push_tail(p_gui->p_msg_list, p_msg);
    klb_mutex_unlock(p_gui->p_msg_mutex);
}

int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create)
{
    assert(NULL != p_gui);
    assert(NULL != p_type);
    assert(NULL != cb_create);

    return klbuiex_wndhash_register(p_gui->p_wndhash, p_type, cb_create);
}
int klb_gui_load_image(klb_gui_t* p_gui, const char* p_key, const char* p_img_path)
{
    if (NULL != p_gui->p_canvas && NULL != p_gui->p_canvas->vtable.load_image)
    {
        return p_gui->p_canvas->vtable.load_image(p_gui->p_canvas, p_key, p_img_path, NULL, NULL);
    }

    return 1;
}

int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style, klb_wnd_t** p_out_wnd)
{
    if (NULL != p_out_wnd)
    {
        *p_out_wnd = NULL;
    }

    return klbuiex_wndhash_append(p_gui->p_wndhash, p_type, p_path_name, x, y, w, h, style);
}

int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name)
{
    return klbuiex_wndhash_remove(p_gui->p_wndhash, p_path_name);
}

static void klb_gui_load_wnd(klb_wnd_t* p_wnd)
{
    if (NULL == p_wnd)
    {
        return;
    }

    klb_wnd_t* p_next = p_wnd->p_child;

    while (NULL != p_next)
    {
        klb_gui_load_wnd(p_next);

        p_next = p_next->p_next;
    }

    if (NULL != p_wnd->vtable.on_control)
    {
        klb_point_t pt = { 0, 0 };
        p_wnd->vtable.on_control(p_wnd, KLBUI_LOAD, &pt, &pt, 0, 0);
    }
}

int klb_gui_do_model(klb_gui_t* p_gui, const char* p_path_name)
{
    if (KLBUI_MODAL_WND_MAX <= p_gui->modal_num)
    {
        return 1; // 超过最大弹出数目
    }

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        for (int i = 0; i < p_gui->modal_num; i++)
        {
            if (p_wnd == p_gui->p_modal_wnd[i])
            {
                return 1; // 已经被弹出
            }
        }

        p_gui->p_modal_wnd[p_gui->modal_num] = p_wnd;
        p_gui->modal_num += 1;

        // KLBUI_LOAD
        klb_gui_load_wnd(p_wnd);
        if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
        {
            klb_point_t pt = { 0, 0 };
            p_wnd->vtable.on_command(p_wnd, KLBUI_LOAD, &pt, &pt, 0, 0);
        }

        p_gui->redraw = true;

        return 0;
    }

    return 1;
}

int klb_gui_end_model(klb_gui_t* p_gui, const char* p_path_name)
{
    if (p_gui->modal_num <= 0)
    {
        return 1;
    }

    int index = p_gui->modal_num - 1;

    klb_wnd_t* p_top = p_gui->p_modal_wnd[index];
    klb_wnd_t* p_wnd = p_top;

    int end = 0;
    // KLBUI_UNLOAD
    //if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
    //{
    //    klb_point_t pt = { 0, 0 };
    //    end = p_wnd->vtable.on_command(p_wnd, KLBUI_UNLOAD, &pt, &pt, 0, 0);
    //}

    if (0 != end)
    {
        return end;
    }

    if (NULL != p_gui->p_focus_top && p_top == p_gui->p_focus_top)
    {
        klb_wnd_set_focus(p_gui->p_focus, false);

        p_gui->p_focus_top = NULL;
        p_gui->p_focus = NULL;
    }

    p_gui->p_modal_wnd[index] = NULL;
    p_gui->modal_num = index;

    // KLBUI_UNLOAD
    if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
    {
        klb_point_t pt = { 0, 0 };
        p_wnd->vtable.on_command(p_wnd, KLBUI_UNLOAD, &pt, &pt, 0, 0);
    }

    p_gui->redraw = true;

    return 0;
}

int klb_gui_end_model_all(klb_gui_t* p_gui)
{
    return 0;
}

/// @brief 弹出菜单等页面
int klb_gui_popup(klb_gui_t* p_gui, const char* p_path_name)
{
    return 0;
}

int klb_gui_popup_end(klb_gui_t* p_gui, bool all)
{
    return 0;
}

/// @brief 消息框
int klb_gui_messagebox(klb_gui_t* p_gui, const char* p_path_name)
{
    if (NULL != p_gui->p_msg_box)
    {
        return 1; // 已经弹出
    }

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        if (NULL != p_gui->p_focus)
        {
            klb_wnd_set_focus(p_gui->p_focus, false);

            p_gui->p_focus_top = NULL;
            p_gui->p_focus = NULL;
        }

        p_gui->p_msg_box = p_wnd;

        // KLBUI_LOAD
        klb_gui_load_wnd(p_wnd);
        if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
        {
            klb_point_t pt = { 0, 0 };
            p_wnd->vtable.on_command(p_wnd, KLBUI_LOAD, &pt, &pt, 0, 0);
        }

        p_gui->redraw = true;
        return 0;
    }

    return 1;
}

int klb_gui_messagebox_end(klb_gui_t* p_gui)
{
    if (NULL == p_gui->p_msg_box)
    {
        return 1;
    }
    
    if (NULL != p_gui->p_focus)
    {
        klb_wnd_set_focus(p_gui->p_focus, false);

        p_gui->p_focus_top = NULL;
        p_gui->p_focus = NULL;
    }

    klb_wnd_t* p_wnd = p_gui->p_msg_box;

    // KLBUI_UNLOAD
    if (NULL != p_wnd && NULL != p_wnd->vtable.on_command)
    {
        klb_point_t pt = { 0, 0 };
        p_wnd->vtable.on_command(p_wnd, KLBUI_UNLOAD, &pt, &pt, 0, 0);
    }

    p_gui->p_msg_box = NULL;
    p_gui->redraw = true;

    return 0;
}

int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    p_wnd->p_udata= p_obj;
    p_wnd->vtable.on_command = on_command;

    return 0;
}

int klb_gui_set(klb_gui_t* p_gui, const char* p_path_name, const klb_map_t* p_map)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    int ret = 1;
    if (p_wnd->vtable.on_set)
    {
        ret = p_wnd->vtable.on_set(p_wnd, p_map);
    }

    return ret;
}

klb_map_t* klb_gui_get(klb_gui_t* p_gui, const char* p_path_name, const klb_map_t* p_map)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return NULL;
    }

    klb_map_t* p_ret = NULL;
    if (p_wnd->vtable.on_get)
    {
        p_ret = p_wnd->vtable.on_get(p_wnd, p_map);
    }

    return p_ret;
}

int klb_gui_show(klb_gui_t* p_gui, const char* p_path_name, bool show)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    klb_wnd_show(p_wnd, show);
    return 0;
}

int klb_gui_move(klb_gui_t* p_gui, const char* p_path_name, int x, int y)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    return 0;
}

int klb_gui_resize(klb_gui_t* p_gui, const char* p_path_name, int w, int h)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
// klb_gui_in.h

int klb_gui_pop_message(klb_gui_t* p_gui, klb_msg_t** p_msg)
{
    if (0 == klb_mutex_trylock(p_gui->p_msg_mutex))
    {
        klb_msg_t* p_pop = (klb_msg_t*)klb_nlist_pop_head(p_gui->p_msg_list);
        klb_mutex_unlock(p_gui->p_msg_mutex);

        if (NULL != p_pop)
        {
            *p_msg = p_pop;
            return 0;
        }
    }

    return 1;
}

static klb_wnd_t* klb_gui_find_focus(klb_gui_t* p_gui, int x, int y, klb_wnd_t** p_top)
{
    // messagebox
    if (NULL != p_gui->p_msg_box)
    {
        klb_wnd_t* p_focus = klb_wnd_pt_in(p_gui->p_msg_box, x, y);
        if (NULL != p_focus)
        {
            *p_top = p_gui->p_msg_box;
        }

        return p_focus;
    }

    for (int i = p_gui->modal_num - 1; 0 <= i; i--)
    {
        klb_wnd_t* p_focus = klb_wnd_pt_in(p_gui->p_modal_wnd[i], x, y);

        if (NULL != p_focus)
        {
            if (NULL != p_top)
            {
                *p_top = p_gui->p_modal_wnd[i];
            }

            return p_focus;
        }
    }

    return NULL;
}

int klb_gui_dispatch_message(klb_gui_t* p_gui, klb_msg_t* p_msg)
{
    if (KLB_WM_MOUSEMOVE == p_msg->msg)
    {
        klb_wnd_t* p_focus_top = NULL;
        klb_wnd_t* p_focus = klb_gui_find_focus(p_gui, p_msg->pt1.x, p_msg->pt1.y, &p_focus_top);

        if (NULL != p_gui->p_focus && p_focus != p_gui->p_focus)
        {
            klb_wnd_set_focus(p_gui->p_focus, false);

            klb_wnd_draw(p_gui->p_focus);
            klb_gui_update_rect(p_gui, NULL);
        }

        if (NULL != p_focus)
        {
            klb_wnd_set_focus(p_focus, true);


            klb_wnd_draw(p_focus);
            klb_gui_update_rect(p_gui, NULL);
        }

        p_gui->p_focus_top = p_focus_top;
        p_gui->p_focus = p_focus;
    }

    // 焦点窗口消息
    klb_wnd_t* p_wnd = p_gui->p_focus;
    if (NULL != p_wnd)
    {
        // 这里 on_control / on_command 函数都需要处理, eg. 组件可能需要响应部分消息
        // 先调用组件自身的处理函数
        if (NULL != p_wnd->vtable.on_control)
        {
            p_wnd->vtable.on_control(p_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
        }

        // 再调用绑定的用户函数
        if (NULL != p_wnd->vtable.on_command)
        {
            p_wnd->vtable.on_command(p_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
        }
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
        klb_canvas_set_draw_color(p_gui->p_canvas, KLB_ARGB8888(0, 0, 0, 0));
        klb_canvas_draw_clear(p_gui->p_canvas);

        for (int i = 0; i < p_gui->modal_num; i++)
        {
            klb_wnd_draw(p_gui->p_modal_wnd[i]);
        }

        if (NULL != p_gui->p_msg_box)
        {
            klb_wnd_draw(p_gui->p_msg_box);
        }

        klb_gui_update_rect(p_gui, NULL);
        p_gui->redraw = false;
    }

    return 0;
}

int klb_gui_update_rect(klb_gui_t* p_gui, const klb_rect_t* p_rect)
{
    p_gui->redraw = true;
    p_gui->refresh = true;
    return 0;
}

int klb_gui_refresh(klb_gui_t* p_gui)
{
    if (p_gui->refresh)
    {
        if (p_gui->p_canvas->vtable.refresh_rect)
        {
            p_gui->p_canvas->vtable.refresh_rect(p_gui->p_canvas, &p_gui->p_canvas->rect);
        }

        p_gui->refresh = false;
    }

    return 0;
}
