// Doc-Encode UTF8-BOM, Space(4), Unix(LF)
#include "klbgui/klb_gui.h"
#include "klbgui/klb_gui_in.h"
#include "klbmem/klb_mem.h"
#include "klbutil/klb_log.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klb_wnd_in.h"
#include "klbgui/klbui_widgets.h"
#include "klbgui/extensions/klbuiex_extensions.h"
#include "klbgui/shwnd/klbshw_calendar.h"
#include "klbgui/shwnd/klbshw_combomenu.h"
#include "klbgui/shwnd/klbshw_decimal.h"
#include "klbgui/shwnd/klbshw_hexadecimal.h"
#include "klbgui/shwnd/klbshw_keyboard_en.h"
#include "klbgui/shwnd/klbshw_messagebox.h"
#include "klbgui/shwnd/klbshw_tip.h"
#include <assert.h>


//////////////////////////////////////////////////////////////////////////
// 前置定义
static void refind_focus_klb_gui(klb_gui_t* p_gui, int x, int y);
static void unfocus_windows_klb_gui(klb_gui_t* p_gui, klb_wnd_t* p_wnd);


//////////////////////////////////////////////////////////////////////////
// klb_gui.h

klb_gui_t* klb_gui_create(klb_canvas_t* p_canvas)
{
    klb_gui_t* p_gui = KLB_MALLOCZ(klb_gui_t, 1, 0);

    ////////////////////////////////////////////
    // 初始化内存
    {
        p_gui->p_canvas = p_canvas;

        p_gui->p_extension_hlist = klb_hlist_create(0);
        p_gui->p_extension_activated_hlist = klb_hlist_create(0);

        p_gui->p_msg_list = klb_nlist_create();
        p_gui->p_msg_mutex = klb_mutex_create();

        p_gui->focus_tc = 0;
        p_gui->focusdelay_tc = 600; // 单位毫秒(ms)
        p_gui->focusdelay = false;

        p_gui->loop_tc = 0;

        p_gui->is_drop_msg_dispatch = false;
        p_gui->is_need_clear = false;
    }


    ////////////////////////////////////////////
    // 注册标准扩展
    KLBUIEX_register_extensions_std(p_gui);

    // 取得常用扩展指针
    {
        // 取得window hash指针
        p_gui->p_wndhash = klbuiex_get_wndhash(p_gui);

        // 取得 图形渲染模块 指针
        p_gui->p_render = klbuiex_get_render(p_gui);

        // 取得重绘记录指针
        p_gui->p_redraw = klbuiex_get_redraw(p_gui);

        // 取得 窗口定时器
        p_gui->p_wndticker = klbuiex_get_wndticker(p_gui);

        // 取得tip
        p_gui->p_tip = klbuiex_get_tip(p_gui);

        // util
        p_gui->p_util = klbuiex_get_util(p_gui);
    }

    // 注册标准窗口类型
    {
        // 注册标准窗口类型
        KLB_GUI_REGISTER_STD(p_gui);

        // 激活共享窗口
        klbui_shwnd_get_calendar(p_gui);
        klbui_shwnd_get_combomenu(p_gui);
        klbui_shwnd_get_decimal(p_gui);
        klbui_shwnd_get_messagebox(p_gui);
        klbui_shwnd_get_tip(p_gui);
    }

    return p_gui;
}

static void klb_gui_quit_extensions(klb_gui_t* p_gui)
{
    // 激活扩展的 退出消息
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_gui->p_extension_activated_hlist);
    while (NULL != p_iter)
    {
        klb_gui_extension_activated_t* p_activated = (klb_gui_extension_activated_t*)klb_hlist_data(p_iter);

        if (NULL != p_activated && NULL != p_activated->ex.cb_control)
        {
            p_activated->ex.cb_control(p_activated->ptr, p_gui, KLBUI_EX_MSG_quit, NULL, 0);
        }

        p_iter = klb_hlist_next(p_iter);
    }

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

    // 清理画布
    klb_gui_attach_canvas(p_gui, NULL);

    // 清理消息
    while (0 < klb_nlist_size(p_gui->p_msg_list))
    {
        klb_msg_t* p_msg = (klb_msg_t*)klb_nlist_pop_head(p_gui->p_msg_list);
        KLB_FREE(p_msg);
    }

    // 退出扩展
    klb_gui_quit_extensions(p_gui);

    // 销毁
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

int klb_gui_default_css_set(klb_gui_t* p_gui, const klb_map_t* p_map)
{
    return klbuiex_default_css_set(klbuiex_get_default(p_gui), p_map);
}

klb_map_t* klb_gui_default_css_get(klb_gui_t* p_gui, const klb_map_t* p_map)
{
    return klbuiex_default_css_get(klbuiex_get_default(p_gui), p_map);
}

void klb_gui_attach_cppgui(klb_gui_t* p_gui, void* p_cgui)
{
    p_gui->p_cppgui = p_cgui;
}

void* klb_gui_get_cppgui(klb_gui_t* p_gui)
{
    return p_gui->p_cppgui;
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

void klb_gui_attach_canvas(klb_gui_t* p_gui, klb_canvas_t* p_canvas)
{
    assert(NULL != p_gui);

    // 主画布
    p_gui->p_canvas = p_canvas;

    // popup / msgbox 画布
    klbuiex_render_try_attach_canvas(p_gui->p_render, p_canvas);

    // tip 画布
    klbuiex_tip_try_attach_canvas(p_gui->p_tip, p_canvas);
}

klb_canvas_t* klb_gui_get_canvas(klb_gui_t* p_gui)
{
    assert(NULL != p_gui);

    return p_gui->p_canvas;
}

bool klb_gui_is_multi_canvas_layer(klb_gui_t* p_gui)
{
    return klbuiex_render_is_multi_layer(p_gui->p_render);
}

int klb_gui_canvas_ioctrl_opt8(klb_gui_t* p_gui, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8)
{
    assert(NULL != p_gui);

    if (p_gui && p_gui->p_canvas && p_gui->p_canvas->vtable.ioctrl_opt8)
    {
        return p_gui->p_canvas->vtable.ioctrl_opt8(p_gui->p_canvas, opt, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, ptr8);
    }

    return -1;
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

void klb_gui_clear_msg(klb_gui_t* p_gui)
{
    klb_mutex_lock(p_gui->p_msg_mutex);

    while (0 < klb_nlist_size(p_gui->p_msg_list))
    {
        klb_msg_t* p_msg = klb_nlist_pop_head(p_gui->p_msg_list);
        KLB_FREE(p_msg);
    }

    klb_mutex_unlock(p_gui->p_msg_mutex);
}

int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create)
{
    assert(NULL != p_gui);
    assert(NULL != p_type);
    assert(NULL != cb_create);

    return klbuiex_wndhash_register(p_gui->p_wndhash, p_type, cb_create);
}

klb_wnd_create_cb klb_gui_get_creater(klb_gui_t* p_gui, const char* p_type)
{
    assert(NULL != p_gui);
    assert(NULL != p_type);

    return klbuiex_wndhash_get_creater(p_gui->p_wndhash, p_type);
}

klb_wnd_t* klb_gui_create_wnd(klb_gui_t* p_gui, const char* p_type, int x, int y, int w, int h)
{
    klb_wnd_t* ptr = NULL;
    klb_wnd_create_cb cb_create = klb_gui_get_creater(p_gui, p_type);

    if (cb_create)
    {
        ptr = cb_create(p_gui, x, y, w, h);
    }

    return ptr;
}

int klb_gui_load_image(klb_gui_t* p_gui, const char* p_key, const char* p_img_path)
{
    if (NULL != p_gui->p_canvas && NULL != p_gui->p_canvas->vtable.load_image)
    {
        return p_gui->p_canvas->vtable.load_image(p_gui->p_canvas, p_key, p_img_path, NULL, NULL);
    }

    return 1;
}

int klb_gui_image_size(klb_gui_t* p_gui, const char* p_key, int* p_out_w, int* p_out_h)
{
    if (NULL != p_gui->p_canvas)
    {
        return klb_canvas_image_size(p_gui->p_canvas, p_key, p_out_w, p_out_h);
    }

    return 1;
}

int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style)
{
    return klbuiex_wndhash_append(p_gui->p_wndhash, p_type, p_path_name, x, y, w, h, style);
}

klb_wnd_t* klb_gui_find_wnd(klb_gui_t* p_gui, const char* p_path_name)
{
    return klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
}

int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name)
{
    return klbuiex_wndhash_remove(p_gui->p_wndhash, p_path_name);
}

int klb_gui_clear(klb_gui_t* p_gui)
{
    // 放弃消息"冒泡"
    klb_gui_drop_msg_dispatch(p_gui, true);

    // 去除所有当前窗口
    klbuiex_redraw_clear(p_gui->p_redraw);
    p_gui->modal_num = 0;
    p_gui->popup_num = 0;
    p_gui->p_msg_box = NULL;

    p_gui->p_focus_top = NULL;
    p_gui->p_focus = NULL;
    p_gui->focus_tc = 0;
    p_gui->focusdelay = false;

    // 所有激活的扩展清理
    klb_hlist_iter_t* p_iter = klb_hlist_begin(p_gui->p_extension_activated_hlist);

    while (NULL != p_iter)
    {
        klb_gui_extension_activated_t* p_activated = (klb_gui_extension_activated_t*)klb_hlist_data(p_iter);
        
        if (p_activated && p_activated->ex.cb_control)
        {
            p_activated->ex.cb_control(p_activated->ptr, p_gui, KLBUI_EX_MSG_clear, NULL, 0);
        }

        p_iter = klb_hlist_next(p_iter);
    }

    // 若申请了其他画布, 需释放其他画布


    // 卸载图片资源
    if (p_gui->p_canvas && p_gui->p_canvas->vtable.clear_image)
    {
        p_gui->p_canvas->vtable.clear_image(p_gui->p_canvas);
    }

    return 0;
}

int klb_gui_clear_async(klb_gui_t* p_gui, klb_gui_clear_result_cb cb_clear, void* ptr)
{
    if (!p_gui->is_need_clear)
    {
        p_gui->cb_clear_result = cb_clear;
        p_gui->p_clear_result = ptr;

        p_gui->is_need_clear = true;

        return 0;
    }

    return 1;
}

// 需要递归执行的窗口事件: eg. KLBUI_onload
static void do_control_event_recursive_klb_wnd(klb_wnd_t* p_wnd, int e, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    if (NULL == p_wnd)
    {
        return;
    }

    klb_wnd_t* p_next = p_wnd->p_child;

    while (NULL != p_next)
    {
        do_control_event_recursive_klb_wnd(p_next, e, p_pt1, p_pt2, lparam, wparam);

        p_next = p_next->p_next;
    }

    klb_wnd_call_control(p_wnd, e, p_pt1, p_pt2, lparam, wparam);
}


// "压栈"待显示窗口流程
static void do_push_stack_top_wnd(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 放弃消息"冒泡"
    klb_gui_drop_msg_dispatch(p_gui, true);

    // 清除焦点
    {
        if (NULL != p_gui->p_focus)
        {
            // 失去焦点流程
            unfocus_windows_klb_gui(p_gui, p_gui->p_focus);

            p_gui->p_focus_top = NULL;
            p_gui->p_focus = NULL;
            p_gui->focus_tc = 0;
            p_gui->focusdelay = false;
        }
    }

    // 设置顶层窗口标记
    klb_wnd_set_style(p_wnd, klb_wnd_get_style(p_wnd) | KLB_WND_STYLE_TOP);

    // "onload" 事件
    {
        // on_control 递归调用所有的控件
        do_control_event_recursive_klb_wnd(p_wnd, KLBUI_onload, NULL, NULL, 0, 0);

        // 只有最顶层窗口拥有 "onload"事件响应
        klb_wnd_call_command(p_wnd, KLBUI_onload, NULL, NULL, 0, 0);
    }

    // "onpredraw" 事件
    {
        // 在 计算 画布 绝对坐标之前, 是因为: 部分控件 需要 在KLBUI_onpredraw事件中 重新调整 子控件的位置
        // on_control 递归调用所有的控件
        do_control_event_recursive_klb_wnd(p_wnd, KLBUI_onpredraw, NULL, NULL, 0, 0);
    }

    // 处理基于画布绝对坐标
    // 注意: 在此之前, 基于画布绝对坐标都是不可靠的
    {
		klb_wnd_update_canvas_rect(p_wnd);
        klb_wnd_calculate_canvas_rect(p_wnd, 0, 0);
    }

    // 重新查找焦点窗口
    {
        refind_focus_klb_gui(p_gui, p_gui->p_util->mouse_pt.x, p_gui->p_util->mouse_pt.y);
    }

    // 标记重绘所有
    klbuiex_redraw_all(p_gui->p_redraw);
}

// "出栈"待显示窗口流程
static void do_pop_statck_top_wnd(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 放弃消息"冒泡"
    klb_gui_drop_msg_dispatch(p_gui, true);

    // 清除焦点
    {
        if (NULL != p_gui->p_focus)
        {
            // 失去焦点流程
            unfocus_windows_klb_gui(p_gui, p_gui->p_focus);

            p_gui->p_focus_top = NULL;
            p_gui->p_focus = NULL;
            p_gui->focus_tc = 0;
            p_gui->focusdelay = false;
        }
    }

    // "onunload" 事件
    {
        // 只有最顶层窗口拥有 "onunload"事件响应
        klb_wnd_call_command(p_wnd, KLBUI_onunload, NULL, NULL, 0, 0);

        // on_control 递归调用所有的控件
        do_control_event_recursive_klb_wnd(p_wnd, KLBUI_onunload, NULL, NULL, 0, 0);
    }

    // 重新查找焦点窗口
    {
        refind_focus_klb_gui(p_gui, p_gui->p_util->mouse_pt.x, p_gui->p_util->mouse_pt.y);
    }

    // 标记重绘所有
    klbuiex_redraw_all(p_gui->p_redraw);
}

// 设置 窗口 图层类型
static void set_wnd_layer_klb_gui(klb_wnd_t* p_wnd, int layer_type)
{
    // 图层类型, 决定后续窗口 画布选择
    uint32_t style = klb_wnd_get_style(p_wnd);

    // 清理图层样式标记
    style &= ~(uint32_t)(KLB_WND_STYLE_LAYER_POPUP);
    style &= ~(uint32_t)(KLB_WND_STYLE_LAYER_MSGBOX);

    // 依据使用图层类型, 重新赋值 图层样式
    if (KLB_CANVAS_LAYER_popup == layer_type)
    {
        style |= KLB_WND_STYLE_LAYER_POPUP;
    }
    else if (KLB_CANVAS_LAYER_msgbox == layer_type)
    {
        style |= KLB_WND_STYLE_LAYER_MSGBOX;
    }
    else
    {
        assert(KLB_CANVAS_LAYER_main == layer_type);
    }

    // 更新样式
    klb_wnd_set_style(p_wnd, style);
}

int klb_gui_modal(klb_gui_t* p_gui, const char* p_path_name)
{
    if (KLBUI_MODAL_WND_MAX <= p_gui->modal_num)
    {
        return 1; // 超过最大弹出数目
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        for (int i = 0; i < p_gui->modal_num; i++)
        {
            if (p_wnd == p_gui->p_modal_wnd[i])
            {
                return 1; // 已经被激活
            }
        }

        int index = p_gui->modal_num;

        // 记录 modal 窗口
        p_gui->p_modal_wnd[index] = p_wnd;
        p_gui->modal_num += 1;

        // 设置 窗口 图层类型
        set_wnd_layer_klb_gui(p_wnd, KLB_CANVAS_LAYER_main);

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_modal(p_gui->p_wndticker, index, p_wnd);
        klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, p_gui->modal_num);

        return 0;
    }

    return 1;
}

/// @brief 模态显示窗口
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_modal_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top)
{
    if (KLBUI_MODAL_WND_MAX <= p_gui->modal_num)
    {
        return 1; // 超过最大弹出数目
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    klb_wnd_t* p_wnd = p_top;

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        for (int i = 0; i < p_gui->modal_num; i++)
        {
            if (p_wnd == p_gui->p_modal_wnd[i])
            {
                return 1; // 已经被弹出
            }
        }

        int index = p_gui->modal_num;

        // 记录 modal 窗口
        p_gui->p_modal_wnd[index] = p_wnd;
        p_gui->modal_num += 1;

        // 设置 窗口 图层类型
        set_wnd_layer_klb_gui(p_wnd, KLB_CANVAS_LAYER_main);

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_modal(p_gui->p_wndticker, index, p_wnd);
        klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, p_gui->modal_num);

        return 0;
    }

    return 1;
}

static void klb_gui_model_end_last(klb_gui_t* p_gui)
{
    if (p_gui->modal_num <= 0)
    {
        return;
    }

    int index = p_gui->modal_num - 1;

    klb_wnd_t* p_top = p_gui->p_modal_wnd[index];
    klb_wnd_t* p_wnd = p_top;

    p_gui->p_modal_wnd[index] = NULL;
    p_gui->modal_num = index;

    // 出栈待显示窗口流程
    do_pop_statck_top_wnd(p_gui, p_wnd);

    // 处理 有定时器 样式的结束流程
    klbuiex_wndticker_modal_end(p_gui->p_wndticker, index);
    klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, p_gui->modal_num);
}


int klb_gui_modal_end(klb_gui_t* p_gui, bool all, const char* p_path_name)
{
    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    if (all)
    {
        // 移除所有
        while (0 < p_gui->modal_num)
        {
            klb_gui_model_end_last(p_gui);
        }
    }
    else
    {
        // 移除最后一个
        klb_gui_model_end_last(p_gui);
    }

    return 0;
}

int klb_gui_modal_num(klb_gui_t* p_gui)
{
    return p_gui->modal_num;
}

/// @brief 弹出菜单等页面
int klb_gui_popup(klb_gui_t* p_gui, const char* p_path_name)
{
    if (KLBUI_POPUP_WND_MAX <= p_gui->popup_num)
    {
        return 1; // 超过最大弹出数目
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        for (int i = 0; i < p_gui->popup_num; i++)
        {
            if (p_wnd == p_gui->p_popup_wnd[i])
            {
                return 1; // 已经被弹出
            }
        }

        int index = p_gui->popup_num;

        // 记录 popup 窗口
        p_gui->p_popup_wnd[index] = p_wnd;
        p_gui->popup_num += 1;

        // 设置 窗口 图层类型
        set_wnd_layer_klb_gui(p_wnd, KLB_CANVAS_LAYER_popup);

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 同步画布位置信息
        klbuiex_render_popup_wnd(p_gui->p_render, index, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_popup(p_gui->p_wndticker, index, p_wnd);
        klbuiex_wndticker_set_popup_num(p_gui->p_wndticker, p_gui->popup_num);

        return 0;
    }

    return 1;
}

int klb_gui_popup_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top)
{
    if (KLBUI_POPUP_WND_MAX <= p_gui->popup_num)
    {
        return 1; // 超过最大弹出数目
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    klb_wnd_t* p_wnd = p_top;

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        for (int i = 0; i < p_gui->popup_num; i++)
        {
            if (p_wnd == p_gui->p_popup_wnd[i])
            {
                return 1; // 已经被弹出
            }
        }

        int index = p_gui->popup_num;

        // 记录 popup 窗口
        p_gui->p_popup_wnd[index] = p_wnd;
        p_gui->popup_num += 1;

        // 设置 窗口 图层类型
        set_wnd_layer_klb_gui(p_wnd, KLB_CANVAS_LAYER_popup);

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 同步画布位置信息
        klbuiex_render_popup_wnd(p_gui->p_render, index, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_popup(p_gui->p_wndticker, index, p_wnd);
        klbuiex_wndticker_set_popup_num(p_gui->p_wndticker, p_gui->popup_num);

        return 0;
    }

    return 1;
}

static void klb_gui_popup_end_last(klb_gui_t* p_gui)
{
    if (p_gui->popup_num <= 0)
    {
        return;
    }

    int index = p_gui->popup_num - 1;

    klb_wnd_t* p_top = p_gui->p_popup_wnd[index];
    klb_wnd_t* p_wnd = p_top;

    p_gui->p_popup_wnd[index] = NULL;
    p_gui->popup_num = index;


    // 出栈待显示窗口流程
    do_pop_statck_top_wnd(p_gui, p_wnd);

    // 处理 有定时器 样式的结束流程
    klbuiex_wndticker_popup_end(p_gui->p_wndticker, index);
    klbuiex_wndticker_set_popup_num(p_gui->p_wndticker, p_gui->popup_num);
}

int klb_gui_popup_end(klb_gui_t* p_gui, bool all)
{
    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    if (all)
    {
        while (0 < p_gui->popup_num)
        {
            klb_gui_popup_end_last(p_gui);
        }
    }
    else
    {
        klb_gui_popup_end_last(p_gui);
    }

    return 0;
}

int klb_gui_popup_num(klb_gui_t* p_gui)
{
    return p_gui->popup_num;
}

/// @brief 消息框
int klb_gui_messagebox(klb_gui_t* p_gui, const char* p_path_name)
{
    if (NULL != p_gui->p_msg_box)
    {
        return 1; // 已经弹出
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    int path_len = strlen(p_path_name);
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);

    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        p_gui->p_msg_box = p_wnd;

        // 设置 窗口 图层类型
        set_wnd_layer_klb_gui(p_wnd, KLB_CANVAS_LAYER_msgbox);

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 同步画布位置信息
        klbuiex_render_msgbox_wnd(p_gui->p_render, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_msgbox(p_gui->p_wndticker, p_wnd);
        klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, 1);

        return 0;
    }

    return 1;
}

/// @brief 消息框: 弹出消息框
int klb_gui_messagebox_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top)
{
    if (NULL != p_gui->p_msg_box)
    {
        return 1; // 已经弹出
    }

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    klb_wnd_t* p_wnd = p_top;
    if (NULL != p_wnd && klb_wnd_is_top(p_wnd))
    {
        p_gui->p_msg_box = p_wnd;

        // 压栈待显示窗口流程
        do_push_stack_top_wnd(p_gui, p_wnd);

        // 同步画布位置信息
        klbuiex_render_msgbox_wnd(p_gui->p_render, p_wnd);

        // 处理 有定时器 样式的流程
        klbuiex_wndticker_msgbox(p_gui->p_wndticker, p_wnd);
        klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, 1);

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

    // Bug. 当前绘制窗口 变更时, 需要清理残存的消息列表, 以免残留的消息继续生效
    // 清空 消息事件 队列
    klb_gui_clear_msg(p_gui);

    klb_wnd_t* p_wnd = p_gui->p_msg_box;

    p_gui->p_msg_box = NULL;

    // 出栈待显示窗口流程
    do_pop_statck_top_wnd(p_gui, p_wnd);

    // 处理 有定时器 样式的结束流程
    klbuiex_wndticker_msgbox_end(p_gui->p_wndticker);
    klbuiex_wndticker_set_modal_num(p_gui->p_wndticker, 0);

    return 0;
}

int klb_gui_messagebox_num(klb_gui_t* p_gui)
{
    return (NULL != p_gui->p_msg_box) ? 1 : 0;
}

int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1; // 未找到
    }

    return klb_wnd_bind_command(p_wnd, on_command, p_obj);
}

int klb_gui_call_control_and_command(klb_gui_t* p_gui, const char* p_path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return -1; // 未找到
    }

    return klb_wnd_call_control_and_command(p_wnd, msg, p_pt1, p_pt2, lparam, wparam);
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

    klb_wnd_move(p_wnd, x, y);
    return 0;
}

int klb_gui_resize(klb_gui_t* p_gui, const char* p_path_name, int w, int h)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    klb_wnd_resize(p_wnd, w, h);
    return 0;
}

/// @brief 获取基于画布的窗口区域
int klb_gui_wndpos_in_canvas(klb_gui_t* p_gui, const char* p_path_name, klb_rect_t* p_out_rect)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    if (NULL != p_out_rect)
    {
        *p_out_rect = p_wnd->pos.rect_in_canvas;
    }

    return 0;
}

/// @brief 获取基于父窗口的区域
int klb_gui_wndpos_in_parent(klb_gui_t* p_gui, const char* p_path_name, klb_rect_t* p_out_rect)
{
    klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
    if (NULL == p_wnd)
    {
        return 1;
    }

    if (NULL != p_out_rect)
    {
        *p_out_rect = p_wnd->pos.rect_in_parent;
    }

    return 0;
}

/// @brief 控件建议宽
int klb_gui_suggestw(klb_gui_t* p_gui, const char* p_path_name, int* p_out_w)
{
    if (NULL != p_path_name && NULL != p_out_w)
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
        if (NULL == p_wnd)
        {
            return 1;
        }

        *p_out_w = klb_wnd_suggestw(p_wnd);
    }

    return 0;
}


/// @brief 控件建议高
int klb_gui_suggesth(klb_gui_t* p_gui, const char* p_path_name, int* p_out_h)
{
    if (NULL != p_path_name && NULL != p_out_h)
    {
        klb_wnd_t* p_wnd = (klb_wnd_t*)klbuiex_wndhash_find(p_gui->p_wndhash, p_path_name);
        if (NULL == p_wnd)
        {
            return 1;
        }

        *p_out_h = klb_wnd_suggesth(p_wnd);
    }

    return 0;
}

int klb_gui_get_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h)
{
    if (NULL != p_out_w)
    {
        *p_out_w = (NULL != p_gui->p_canvas) ? p_gui->p_canvas->rect.w : 0;
    }

    if (NULL != p_out_h)
    {
        *p_out_h = (NULL != p_gui->p_canvas) ? p_gui->p_canvas->rect.h : 0;
    }

    return 0;
}

/// @brief 获取 当前聚焦窗口
klb_wnd_t* klb_gui_get_focus(klb_gui_t* p_gui)
{
    return p_gui->p_focus;
}


/// @brief 获取 当前聚焦窗口 的顶层窗口
klb_wnd_t* klb_gui_get_focus_top(klb_gui_t* p_gui)
{
    return p_gui->p_focus_top;
}


/// @brief 设置聚焦延时时间
void klb_gui_set_focusdelay(klb_gui_t* p_gui, int64_t timeout)
{
    timeout = (0 <= timeout) ? timeout : 0;
    p_gui->focusdelay_tc = timeout;
}


/// @brief 标记所有窗口需要刷新
void klb_gui_update(klb_gui_t* p_gui)
{
    // 标记重回所有窗口
    klbuiex_redraw_all(p_gui->p_redraw);
}

int64_t klb_gui_get_tick_count(klb_gui_t* p_gui)
{
    return p_gui->loop_tc;
}

int64_t klb_gui_get_ticker_interval(klb_gui_t* p_gui)
{
    return klbuiex_wndticker_get_interval(p_gui->p_wndticker);
}

void klb_gui_set_ticker_interval(klb_gui_t* p_gui, int64_t interval)
{
    klbuiex_wndticker_set_interval(p_gui->p_wndticker, interval);
}

//////////////////////////////////////////////////////////////////////////

int klb_gui_update_wnd(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    klbuiex_redraw_push(p_gui->p_redraw, p_wnd);

    return 0;
}

int klb_gui_update_tip(klb_gui_t* p_gui, const char* p_tip)
{
    klbuiex_tip_update(p_gui->p_tip, p_tip);

    return 0;
}

void klb_gui_drop_msg_dispatch(klb_gui_t* p_gui, bool drop)
{
    p_gui->is_drop_msg_dispatch = drop;
}

bool klb_gui_is_drop_msg_dispatch(klb_gui_t* p_gui)
{
    return p_gui->is_drop_msg_dispatch;
}

//////////////////////////////////////////////////////////////////////////
// loop message

// 查找(x,y), 处于哪个窗口中
static klb_wnd_t* find_focus_klb_gui(klb_gui_t* p_gui, int x, int y, klb_wnd_t** p_top)
{
    // messagebox
    if (NULL != p_gui->p_msg_box)
    {
        klb_wnd_t* p_focus = klb_wnd_pt_in(p_gui->p_msg_box, x, y);
        if (NULL != p_focus)
        {
            *p_top = p_gui->p_msg_box;
        }

        // 在有 messagebox 时, 只在 messagebox 里面寻找
        return p_focus;
    }

    // popup
    if (0 < p_gui->popup_num)
    {
        for (int i = p_gui->popup_num - 1; 0 <= i; i--)
        {
            klb_wnd_t* p_wnd = p_gui->p_popup_wnd[i];
            klb_wnd_t* p_focus = klb_wnd_pt_in(p_wnd, x, y);

            if (NULL != p_focus)
            {
                if (NULL != p_top)
                {
                    *p_top = p_gui->p_popup_wnd[i];
                }

                return p_focus;
            }

            if (KLB_WND_STYLE_FOCUS_CONTINUE & p_wnd->state.style)
            {
                // none
            }
            else
            {
                return NULL; // 无此标记, 则退出
            }
        }

        // 在有 popup 时, 只在 popup 里面寻找
        return NULL;
    }

    // modal
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

// 聚焦窗口流程
static void focus_windows_klb_gui(klb_gui_t* p_gui, klb_wnd_t* p_wnd, int x, int y)
{
    // 即将得到焦点事件
    klb_wnd_call_control_and_command(p_wnd, KLBUI_focusin, NULL, NULL, 0, 0);

    // 得到焦点
    klb_wnd_set_focus(p_wnd, true);

    // 得到焦点事件
    klb_wnd_call_control_and_command(p_wnd, KLBUI_focus, NULL, NULL, 0, 0);

#if 0
    // Tip 的显示 转移到 聚焦延时 处理流程中
    // 这里暂保留备份, 待删除
    // 显示tip
    const sds tip = klb_wnd_get_tip(p_wnd);
    if (NULL != tip && 0 < sdslen(tip))
    {
        klb_rect_t rect = p_wnd->pos.rect_in_canvas;

        int w = 0, h = 0;
        klbuiex_tip_set_tilte(p_gui->p_tip, tip, &w, &h);

        int screen_w = 0, screen_h = 0;
        klb_gui_get_wh(p_gui, &screen_w, &screen_h);

        //int sx = rect.x + rect.w / 2;
        int sy = rect.y + rect.h + 1;

        //if (screen_w < sx + w) { sx = screen_w - w; };
        if (screen_h < sy + h) { sy = screen_h - h - 1; };

        klbuiex_tip_show(p_gui->p_tip, true, x, sy);
    }
#endif
}

// 取消聚焦流程
static void unfocus_windows_klb_gui(klb_gui_t* p_gui, klb_wnd_t* p_wnd)
{
    // 即将失去焦点事件
    klb_wnd_call_control_and_command(p_wnd, KLBUI_focusout, NULL, NULL, 0, 0);

    // 失去焦点
    klb_wnd_set_focus(p_wnd, false);

    // 失去焦点事件
    klb_wnd_call_control_and_command(p_wnd, KLBUI_blur, NULL, NULL, 0, 0);

    // 隐藏tip
    klbuiex_tip_show(p_gui->p_tip, false, 0, 0);
}

// 重新寻找鼠标焦点
static void refind_focus_klb_gui(klb_gui_t* p_gui, int x, int y)
{
    // 寻找焦点窗口
    klb_wnd_t* p_focus_top = NULL;
    klb_wnd_t* p_focus = find_focus_klb_gui(p_gui, x, y, &p_focus_top);

    if (NULL != p_gui->p_focus && p_focus != p_gui->p_focus)
    {
        // 取消焦点流程
        unfocus_windows_klb_gui(p_gui, p_gui->p_focus);

        // 更新窗口
        // KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW 样式, 许可将窗口设计为可响应事件, 但不自动重绘
        // 使用者需要自行维护相关绘图部分设计
        if (!(KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW & p_gui->p_focus->state.style))
        {
            klb_wnd_update(p_gui->p_focus);
        }
    }

    if (NULL != p_focus && p_focus != p_gui->p_focus)
    {
        // 聚焦流程
        focus_windows_klb_gui(p_gui, p_focus, x, y);

        // 更新窗口
        if (!(KLB_WND_STYLE_FOCUS_WITHOUT_REDRAW & p_focus->state.style))
        {
            klb_wnd_update(p_focus);
        }
    }

    p_gui->p_focus_top = p_focus_top;
    p_gui->p_focus = p_focus;

    if (NULL != p_focus)
    {
        p_gui->focus_tc = p_gui->loop_tc;
        p_gui->focusdelay = true;
    }
}

static int klb_gui_dispatch_message(klb_gui_t* p_gui, klb_msg_t* p_msg)
{
    // Note. 消息事件处理流程
    // 并不仅仅是焦点窗口需要处理
    // 此外, 调用者可能需要直接处理相关消息
    // 则 所有消息 总归有一个最顶层窗口处理

    // Bug. klb_wnd_on_control_and_command 调用后
    // 在其响应函数中可能使用 model/popup/messagebox, 甚至 remove/clear 等函数
    // 此时 已经破坏正常消息处理流程 的条件
    // 所以 这些情况下, 需要终止消息事件 继续传递处理
    // 解决方案1. on_control/on_command函数返回值, 标记是否终止消息 "冒泡". (ps.以前这么处理的, 较为繁琐, 调用者需要谨慎区分)
    // 解决方案2. 将有影响的函数调用中, 做个标记, 这里检测到标记, 终止消息 "冒泡". (ps.当前处理方案)

    // step1. 移动消息; 更新坐标/焦点窗口
    if (KLBUI_mousemove == p_msg->msg)
    {
        // 更新记录鼠标位置
        p_gui->p_util->mouse_pt = p_msg->pt1;

        // 重新寻找焦点窗口
        refind_focus_klb_gui(p_gui, p_msg->pt1.x, p_msg->pt1.y);
    }


    // step2. 开始处理消息, 先清理标记
    klb_gui_drop_msg_dispatch(p_gui, false);


    // step3. 若有焦点窗口, 处理焦点窗口消息事件
    klb_wnd_t* p_wnd = p_gui->p_focus;

    if (NULL != p_wnd)
    {
        int ret_command = 0;

        // 焦点窗口事件
        if (0 <= ret_command)
        {
            ret_command = klb_wnd_call_control_and_command(p_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
        }
        if (klb_gui_is_drop_msg_dispatch(p_gui)) { return 0; };

        // 检查窗口链中 需要抓取 KLB_WND_STYLE_PEEK_EVENT 标记的窗口
        // 若有标记, 则响应消息
        klb_wnd_t* p_tmp = p_wnd->p_parent;
        while (NULL != p_tmp && NULL != p_tmp->p_parent && 0 <= ret_command)
        {
            if (KLB_WND_STYLE_PEEK_EVENT & p_tmp->state.style)
            {
                if (0 <= ret_command)
                {
                    ret_command = klb_wnd_call_control_and_command(p_tmp, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
                }
                if (klb_gui_is_drop_msg_dispatch(p_gui)) { return 0; };
            }

            p_tmp = p_tmp->p_parent;
        }

        // 当前焦点窗口的最顶层窗口, 响应消息
        klb_wnd_t* p_top_wnd = (NULL != p_tmp) ? p_tmp : p_wnd;
        if (p_wnd != p_top_wnd)
        {
            if (0 <= ret_command)
            {
                klb_wnd_call_control_and_command(p_top_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
            }
            if (klb_gui_is_drop_msg_dispatch(p_gui)) { return 0; };
        }
    }

    // step4. 若无焦点窗口, 则将消息 交给 messagebox/popup/modal 绘图次序中最后的 那个顶层窗口
    p_wnd = NULL;
    if (NULL == p_gui->p_focus)
    {
        // 处在最后绘制的 顶层窗口
        if (NULL != p_gui->p_msg_box)
        {
            p_wnd = p_gui->p_msg_box;
        }
        else if(0 < p_gui->popup_num)
        {
            p_wnd = p_gui->p_popup_wnd[p_gui->popup_num - 1];
        }
        else if(0 < p_gui->modal_num)
        {
            p_wnd = p_gui->p_modal_wnd[p_gui->modal_num - 1];
        }

        // 最顶层窗口处理消息
        if (NULL != p_wnd)
        {
            klb_wnd_call_control_and_command(p_wnd, p_msg->msg, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
            if (klb_gui_is_drop_msg_dispatch(p_gui)) { return 0; };
        }
    }

    // step5. 若无焦点窗口, 处理 messagebox/popup 窗口的 KLBUI_outwindow 消息
    // 方便关闭窗口
    p_wnd = NULL;
    bool is_outwindow = false;
    if (NULL == p_gui->p_focus)
    {
        // 若无焦点窗口, 则检查是否有 messagebox / popup 窗口
        // 若有messagebox / popup , 则将消息交个顶层窗口处理
        if (NULL != p_gui->p_msg_box)
        {
            p_wnd = p_gui->p_msg_box;

            // Bug. "click"/"dblclick"/"mousedown", 会重复发送事件, 这里选用"click"来判定
            // 判定是否在messagebox窗口之外点击
            if (KLBUI_click == p_msg->msg || KLBUI_dblclick == p_msg->msg)
            {
                if (!klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, p_msg->pt1.x, p_msg->pt1.y))
                {
                    is_outwindow = true;
                }
            }
        }
        else if (0 < p_gui->popup_num)
        {
            p_wnd = p_gui->p_popup_wnd[p_gui->popup_num - 1];

            // Bug. "click"/"dblclick"/"mousedown", 会重复发送事件, 这里选用"click"来判定
            // 判定是否在popup窗口之外点击
            if (KLBUI_click == p_msg->msg || KLBUI_dblclick == p_msg->msg)
            {
                if (!klb_pt_in_rect(&p_wnd->pos.rect_in_canvas, p_msg->pt1.x, p_msg->pt1.y))
                {
                    is_outwindow = true;
                }
            }
        }
    }

    if (is_outwindow && NULL != p_wnd)
    {
        klb_wnd_call_control_and_command(p_wnd, KLBUI_outwindow, &p_msg->pt1, &p_msg->pt2, p_msg->lparam, p_msg->wparam);
        if (klb_gui_is_drop_msg_dispatch(p_gui)) { return 0; };
    }

    return 0;
}

static int klb_gui_pop_message(klb_gui_t* p_gui, klb_msg_t** p_msg)
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

static int klb_gui_process_message_once(klb_gui_t* p_gui)
{
    klb_msg_t* p_msg = NULL;
    if (!p_gui->is_need_clear && 0 == klb_gui_pop_message(p_gui, &p_msg))
    {
        if (p_gui->p_canvas)
        {
            // 设置了画布, 才处理消息
            klb_gui_dispatch_message(p_gui, p_msg);
        }

        KLB_FREE(p_msg);
        return 0; // 有消息处理
    }

    return 1;
}

int klb_gui_loop_once(klb_gui_t* p_gui, int64_t tc)
{
    int ret = 0;

    // step 1. 更新 gui tc
    p_gui->loop_tc = tc;


    // step 2. 消息处理
    while (true)
    {
        if (0 != klb_gui_process_message_once(p_gui))
        {
            break; // 一次将队列中的所有消息处理完毕
        }
    }

    // step 3. 检查处理 是否需要处理 KLBUI_focusdelay 消息等
    if (p_gui->focusdelay && p_gui->focusdelay_tc <= (ABS_SUB(tc, p_gui->focus_tc)))
    {
        klb_wnd_t* p_wnd = p_gui->p_focus;
        if (NULL != p_wnd)
        {
            // KLBUI_focusdelay 消息
            if (KLB_WND_STYLE_FOCUS_DELAY & p_wnd->state.style)
            {
                klb_wnd_call_control_and_command(p_wnd, KLBUI_focusdelay, NULL, NULL, 0, 0);
            }

            // 显示tip; 优先 动态tip, 再次 静态tip
            sds tip = (sds)klb_wnd_get_tip_dynamic(p_wnd);
            if (NULL == tip)
            {
                tip = (sds)klb_wnd_get_tip(p_wnd);
            }

            if (NULL != tip && 0 < sdslen(tip))
            {
                klb_rect_t rect = p_wnd->pos.rect_in_canvas;

                int w = 0, h = 0;
                klbuiex_tip_set_tilte(p_gui->p_tip, tip, &w, &h);

                int screen_w = 0, screen_h = 0;
                klb_gui_get_wh(p_gui, &screen_w, &screen_h);

                int sx = p_gui->p_util->mouse_pt.x;
                //int sx = rect.x + rect.w / 2;
                int sy = rect.y + rect.h + 1;

                if (screen_w < sx + w) { sx = screen_w - w; };
                if (screen_h < sy + h) { sy = screen_h - h - 1; };

                klbuiex_tip_show(p_gui->p_tip, true, sx, sy);
            }
        }

        p_gui->focusdelay = false;
    }

    // step 4. 依次处理, 激活的扩展
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

    // step 5. 是否需要清理
    // 这里控件等事件处理完毕, 在这里执行清理动作
    if (p_gui->is_need_clear)
    {
        klb_gui_clear(p_gui); // 清理

        // 清理完毕之后, 回调
        if (p_gui->cb_clear_result)
        {
            p_gui->cb_clear_result(p_gui->p_clear_result, p_gui);
        }

        p_gui->is_need_clear = false;
    }

    // step 6. 重绘 / 刷新
    klbuiex_render_redraw_and_refresh(p_gui->p_render);

    return ret;
}
