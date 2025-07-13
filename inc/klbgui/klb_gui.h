///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_gui.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   GUI接口定义
///   默认配色采用 visual studio 深色系风格
///   图标来源于1: http://iconpark.oceanengine.com/official
///   图标来源于2: https://remixicon.com/
/// @version 0.6
/// @history 修改历史
///   \n [2023-1] 调整绘制窗体类型: "modal" - "popup" - "messagebox" - "tip"
///   \n [2023-1] 添加扩展机制: 将由扩展来处理ui的部分功能
///   \n [2024-4] 添加 klb_gui_canvas_ioctrl_opt8 接口, 许可 开发者 在上层透过GUI框架, 直接对画布进行交互操作
///   \n [2025-1] 添加定时器相关功能
///   \n [2025-6] 调整UI图形渲染功能到专门的内部渲染扩展模块中
///   \n [2025-6] 添加支持多画布图层绘图模式(指 modal/popup/msgbox 分别使用不同的 画布)
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GUI_H__
#define __KLB_GUI_H__

#include "klb_type.h"
#include "klua/klua_env.h"
#include "klbutil/klb_rect.h"
#include "klbutil/klb_canvas.h"
#include "klbutil/klb_map.h"
#include "klbgui/klb_msg.h"
#include "klbgui/klbui_event.h"
#include "klbgui/klbui_default.h"
#include "klbgui/klbui_extension.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klbui_css_ex.h"
#include "klbgui/klb_wnd.h"
#include "klbgui/klbui_shwnd.h"
#include "klbgui/klbui_util.h"
#include "klbgui/klbui_datetime.h"
#include "klbgui/klbui_layer.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;
typedef struct klb_wnd_t_ klb_wnd_t;


/// @brief 创建一个GUI对象
/// @param [in] *p_canvas       主显存画布
/// @return klb_gui_t* GUI对象
KLB_API klb_gui_t* klb_gui_create(klb_canvas_t* p_canvas);


/// @brief 销毁GUI对象
/// @param [in] *p_gui          GUI对象
/// @return 无
KLB_API void klb_gui_destroy(klb_gui_t* p_gui);


/// @brief 附加到 klbui::CGui*, 提供cpp支持
KLB_API void klb_gui_attach_cppgui(klb_gui_t* p_gui, void* p_cgui);


/// @brief 获取附加的 klbui::CGui*
KLB_API void* klb_gui_get_cppgui(klb_gui_t* p_gui);


/// @brief 附加到 klua_env_t*
KLB_API int klb_gui_attach_klua_env(klb_gui_t* p_gui, klua_env_t* p_env);


/// @brief 获取附加的 klua_env_t*
KLB_API klua_env_t* klb_gui_get_klua_env(klb_gui_t* p_gui);


/// @brief 处理消息(事件)流程
/// @param [in] *p_gui          GUI对象
/// @param [in] tc              系统滴答数(毫秒)
/// @return int 0.有消息(事件处理); 非0.无消息处理
/// @note 由外部申请线程, 处理UI流程
///  \n 当有消息(事件)时调用; 定时器定时调用
KLB_API int klb_gui_loop_once(klb_gui_t* p_gui, int64_t tc);


/// @brief 附加到某个显存画布
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_canvas       画布
/// @return 无
KLB_API void klb_gui_attach_canvas(klb_gui_t* p_gui, klb_canvas_t* p_canvas);


/// @brief 获取附加到的显存画布指针
/// @param [in] *p_gui          GUI对象
/// @return klb_canvas_t* 
KLB_API klb_canvas_t* klb_gui_get_canvas(klb_gui_t* p_gui);


/// @brief 获取是否为 多图层画布模式
/// @param [in] *p_gui          GUI对象
/// @return bool
/// @note
///    true. 指 modal/popup/msgbox 分别使用不同的 画布
///    false. 指 modal/popup/msgbox 共享使用 主画布
KLB_API bool klb_gui_is_multi_canvas_layer(klb_gui_t* p_gui);


/// @brief 扩展交互函数; GUI框架仅做透明调用
/// @param [in] *p_gui          GUI对象
/// @param [in] opt             操作代码
/// @param [in, out] *ptrN      参数1 ~ 参数8
/// @return int 错误码; (-1.未设置主画布)
/// @note 本函数 许可 开发者 在上层透过GUI框架, 直接对主画布进行交互操作
///       具体消息, 参数格式等 由 开发者 自行定义, 最多8个参数
KLB_API int klb_gui_canvas_ioctrl_opt8(klb_gui_t* p_gui, int opt, void* ptr1, void* ptr2, void* ptr3, void* ptr4, void* ptr5, void* ptr6, void* ptr7, void* ptr8);


/// @brief 向GUI中放入消息
/// @param [in] *p_gui          GUI对象
/// @param [in] msg             消息/事件: eg. KLBUI_click
/// @return 无
KLB_API void klb_gui_push_msg(klb_gui_t* p_gui, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam);


/// @brief 清空消息事件
/// @param [in] *p_gui          GUI对象
/// @param [in] msg             消息/事件: eg. KLBUI_click
/// @return 无
KLB_API void klb_gui_clear_msg(klb_gui_t* p_gui);


/// @brief 创建窗口
/// @param [in] x               相对父窗口X坐标
/// @param [in] y               相对父窗口Y坐标
/// @param [in] w               宽
/// @param [in] h               高
/// @return klb_wnd_t* 窗口对象
typedef klb_wnd_t* (*klb_wnd_create_cb)(klb_gui_t* p_gui, int x, int y, int w, int h);


/// @brief 注册窗口类型
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @param [in] cb_create       创建窗口
/// @return int 0.成功; 非0.失败(错误码)
///  \n 标准控件类型名命名规则为 "k*", eg."kbutton", "kdialog"
///  \n 若不存在 类型, 则新注册 创建函数
///  \n 若已存在 类型, 则替换 创建函数
KLB_API int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create);


/// @brief 获取注册的窗口类型的 创建函数
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @return klb_wnd_create_cb 创建函数; 或 NULL
KLB_API klb_wnd_create_cb klb_gui_get_creater(klb_gui_t* p_gui, const char* p_type);


/// @brief 按类型 创建一个窗口; 注意.仅创建, 并未加入到窗口树中
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @return klb_wnd_t* 窗口指针; 或 NULL
KLB_API klb_wnd_t* klb_gui_create_wnd(klb_gui_t* p_gui, const char* p_type, int x, int y, int w, int h);


/// @brief 加载图片资源
KLB_API int klb_gui_load_image(klb_gui_t* p_gui, const char* p_key, const char* p_img_path);


/// @brief 获取图片资源大小
KLB_API int klb_gui_image_size(klb_gui_t* p_gui, const char* p_key, int* p_out_w, int* p_out_h);


/// @brief 添加窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] x               相对父窗口X坐标
/// @param [in] y               相对父窗口Y坐标
/// @param [in] w               宽
/// @param [in] h               高
/// @return int 0.成功; 非0.失败(错误码)
/// @note 前父窗口必须存在; eg. "/home/btn1" 则需要 "/home" 必须存在, 才能添加
KLB_API int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style);


/// @brief 依路径查找窗口
/// @return klb_wnd_t* 窗口指针
KLB_API klb_wnd_t* klb_gui_find_wnd(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 移除窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 清理所有窗口
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
/// @note 当修改分辨率等需要将所有UI清理掉时
///     清理内容:
///       1. klb_gui_append 添加的所有窗口
///       2. klb_gui_load_image 加载的所有图片资源
///       3. klb_gui_push_shwnd 添加的所有共享窗口
///     不清理内容
///       a. klb_gui_register 注册的控件类型
///       b. klb_gui_register_extension 注册的扩展
///     !!!注意: 不能在GUI的流程中, 调用此函数, 否则内部数据异常
KLB_API int klb_gui_clear(klb_gui_t* p_gui);


/// @brief 异步清理窗口之后的回调函数
/// @param [in] *ptr            调用者附加参数指针
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
typedef int (*klb_gui_clear_result_cb)(void* ptr, klb_gui_t* p_gui);


/// @brief 异步等待合适时机, 清理所有窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] cb_clear        清理完成的回调函数
/// @param [in] *ptr            清理完成的回调函数
/// @return int 0.成功; 非0.失败(错误码)
/// @note 当修改分辨率等需要将所有UI清理掉时
///     清理内容:
///       1. klb_gui_append 添加的所有窗口
///       2. klb_gui_load_image 加载的所有图片资源
///       3. klb_gui_push_shwnd 添加的所有共享窗口
///     不清理内容
///       a. klb_gui_register 注册的控件类型
///       b. klb_gui_register_extension 注册的扩展
///     注意: 清理窗口会在GUI特定的合适时机清理， 清理完成之后再使用回调函数通知调用者
KLB_API int klb_gui_clear_async(klb_gui_t* p_gui, klb_gui_clear_result_cb cb_clear, void* ptr);


/// @brief 模态显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_modal(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 模态显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_modal_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top);


/// @brief 关闭显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_modal_end(klb_gui_t* p_gui, bool all, const char* p_path_name);


/// @brief 获取 当前modal窗口数目
/// @param [in] *p_gui          GUI对象
/// @return int  modal 窗口数
KLB_API int klb_gui_modal_num(klb_gui_t* p_gui);


/// @brief 弹出菜单/对话框等页面
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/menu1"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_popup(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 弹出菜单/对话框等页面
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_popup_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top);


/// @brief 关闭弹出的页面
KLB_API int klb_gui_popup_end(klb_gui_t* p_gui, bool all);


/// @brief 获取 当前popup窗口数目
/// @param [in] *p_gui          GUI对象
/// @return int  popup 窗口数
KLB_API int klb_gui_popup_num(klb_gui_t* p_gui);


/// @brief 消息框: 弹出消息框
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/messagebox1"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_messagebox(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 消息框: 弹出消息框
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_top          外部自定义窗口: 由外部管理生命周期
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_messagebox_wnd(klb_gui_t* p_gui, klb_wnd_t* p_top);


/// @brief 关闭消息框
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_messagebox_end(klb_gui_t* p_gui);


/// @brief 获取 当前messagebox窗口数目
/// @param [in] *p_gui          GUI对象
/// @return int  messagebox 窗口数
KLB_API int klb_gui_messagebox_num(klb_gui_t* p_gui);


/// @brief 向控件(窗口)绑定事件响应函数
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] on_command      事件响应函数(单击,双击等)
/// @param [in] *p_obj          用户附加数据
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj);


/// @brief 外部触发控件(窗口)某个事件
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] msg             事件
/// @return int 0.成功; -1.终止
KLB_API int klb_gui_call_control_and_command(klb_gui_t* p_gui, const char* p_path_name, int msg, const klb_point_t* p_pt1, const klb_point_t* p_pt2, int lparam, int wparam);


/// @brief 向控件(窗口)设置数据: 样式\显示\状态等等
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @param [in] *p_map          map数据集合
/// @return int 0.成功; 非0.失败(错误码)
/// @note map 具体数据格式由控件定义
KLB_API int klb_gui_set(klb_gui_t* p_gui, const char* p_path_name, const klb_map_t* p_map);


/// @brief 向控件(窗口)获取数据: 样式\显示\状态等等
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @param [in] *p_map          map数据集合
/// @return klb_map_t* map数据集合
/// @note map 具体数据格式由控件定义
KLB_API klb_map_t* klb_gui_get(klb_gui_t* p_gui, const char* p_path_name, const klb_map_t* p_map);


/// @brief 设置控件(窗口)的显示状态
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @param [in] *p_map          map数据集合
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_show(klb_gui_t* p_gui, const char* p_path_name, bool show);


/// @brief 基于父窗口移动(相对坐标)
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_move(klb_gui_t* p_gui, const char* p_path_name, int x, int y);


/// @brief 重新设置控件大小
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_resize(klb_gui_t* p_gui, const char* p_path_name, int w, int h);


/// @brief 获取基于画布的窗口区域
KLB_API int klb_gui_wndpos_in_canvas(klb_gui_t* p_gui, const char* p_path_name, klb_rect_t* p_out_rect);


/// @brief 获取基于父窗口的区域
KLB_API int klb_gui_wndpos_in_parent(klb_gui_t* p_gui, const char* p_path_name, klb_rect_t* p_out_rect);


/// @brief 控件建议宽
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_suggestw(klb_gui_t* p_gui, const char* p_path_name, int* p_out_w);


/// @brief 控件建议高
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_suggesth(klb_gui_t* p_gui, const char* p_path_name, int* p_out_h);



/// @brief 获取主画布(主屏幕)的分辨率
/// @param [in]  *p_gui         GUI对象
/// @param [out] *p_out_w       输出窗口宽
/// @param [out] *p_out_h       输出窗口高
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_get_wh(klb_gui_t* p_gui, int* p_out_w, int* p_out_h);


/// @brief 获取 当前聚焦窗口
/// @param [in]  *p_gui         GUI对象
/// @return klb_wnd_t* 聚焦窗口指针
/// @note 可能为 NULL
KLB_API klb_wnd_t* klb_gui_get_focus(klb_gui_t* p_gui);


/// @brief 获取 当前聚焦窗口 的顶层窗口
/// @param [in]  *p_gui         GUI对象
/// @return klb_wnd_t* 聚焦窗口的 顶层窗口指针
/// @note 可能为 NULL
KLB_API klb_wnd_t* klb_gui_get_focus_top(klb_gui_t* p_gui);


/// @brief 设置聚焦延时时间
/// @param [in]  *p_gui         GUI对象
/// @param [in]  timeout        延时时间(单位毫秒ms, 默认600); 范围[0, ~]
/// @return 无
KLB_API void klb_gui_set_focusdelay(klb_gui_t* p_gui, int64_t timeout);


/// @brief 标记所有窗口需要刷新
/// @param [in]  *p_gui         GUI对象
/// @note 仅标记, 由框架决定合适的刷新时机
///     "update"在这里表示页面需要刷新
KLB_API void klb_gui_update(klb_gui_t* p_gui);


/// @brief 获取GUI的当前 系统滴答数(单位毫秒ms)
/// @param [in]  *p_gui         GUI对象
/// @return int64_t 系统滴答数
KLB_API int64_t klb_gui_get_tick_count(klb_gui_t* p_gui);


/// @brief 获取GUI的 内部控件定时器运行间隔 (单位毫秒ms, 默认 500ms)
/// @param [in]  *p_gui         GUI对象
/// @return int64_t 时间间隔(单位毫秒ms)
KLB_API int64_t klb_gui_get_ticker_interval(klb_gui_t* p_gui);


/// @brief 设置GUI的 内部控件定时器运行间隔 (单位毫秒ms)
/// @param [in]  *p_gui         GUI对象
/// @return 无
KLB_API void klb_gui_set_ticker_interval(klb_gui_t* p_gui, int64_t interval);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_H__
//end
