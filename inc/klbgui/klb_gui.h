///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_gui.h
/// @brief   GUI接口定义
///   默认配色采用 visual studio 深色系风格
///   图标来源于1: http://iconpark.oceanengine.com/official
///   图标来源于2: https://remixicon.com/
/// @version 0.2
/// @history 修改历史
///   \n [2023-1] 调整绘制窗体类型: "modal" - "popup" - "messagebox" - "tip"
///   \n [2023-1] 添加扩展机制: 将由扩展来处理ui的部分功能
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
#include "klbgui/klbui_css.h"
#include "klbgui/klb_wnd.h"

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


/// @struct klb_gui_extension_t
/// @brief  gui扩展
typedef struct klb_gui_extension_t_
{
    /// @brief 创建扩展
    /// @param [in] *p_gui          gui对象
    /// @return void* 扩展的指针
    void* (*cb_create)(klb_gui_t* p_gui);

    /// @brief 销毁扩展
    /// @param [in] *ptr            扩展的指针
    /// @return 无
    void(*cb_destroy)(void* ptr, klb_gui_t* p_gui);

    /// @brief 常规调用一次
    /// @param [in] *ptr            扩展的指针
    /// @param [in] *p_gui          gui对象
    /// @param [in] now             当前滴答数
    /// @return int 0
    int(*cb_loop_once)(void* ptr, klb_gui_t* p_gui, int64_t now);
}klb_gui_extension_t;


/// @brief 注册gui扩展
/// @param [in] *p_gui              gui对象
/// @param [in] *p_name             名称
/// @param [in] *p_extension        扩展的接口函数
/// @return int 0
KLB_API int klb_gui_register_extension(klb_gui_t* p_gui, const char* p_name, const klb_gui_extension_t* p_extension);


/// @brief 获取gui扩展
/// @param [in] *p_gui              gui对象
/// @param [in] *p_name             名称
/// @return void* 扩展的指针
KLB_API void* klb_gui_get_extension(klb_gui_t* p_gui, const char* p_name);


/// @brief 附加到 klua_env_t*
KLB_API int klb_gui_attach_klua_env(klb_gui_t* p_gui, klua_env_t* p_env);


/// @brief 获取附加的 klua_env_t*
KLB_API klua_env_t* klb_gui_get_klua_env(klb_gui_t* p_gui);


/// @brief 处理消息(事件)流程
/// @param [in] *p_gui          GUI对象
/// @param [in] tc              系统滴答数(毫秒)
/// @return int 0.有消息(事件处理); 非0.无消息处理
/// @note 此函数为替代 start/stop 流程, 由外部申请线程, 处理UI流程
///  \n 与 start/stop 不能同时使用
///  \n 当有消息(事件)时调用; 定时器定时调用
KLB_API int klb_gui_loop_once(klb_gui_t* p_gui, int64_t tc);


/// @brief 附加到某个显存画布
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_canvas       画布
/// @return 无
KLB_API void klb_gui_attach_canvas(klb_gui_t* p_gui, klb_canvas_t* p_canvas);


/// @brief 向GUI中放入消息
/// @param [in] *p_gui          GUI对象
/// @param [in] msg             消息类型: "./gui/klb_msg.h"
/// @return 无
KLB_API void klb_gui_push_msg(klb_gui_t* p_gui, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam);


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
KLB_API int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create);


/// @brief 加载图片资源
KLB_API int klb_gui_load_image(klb_gui_t* p_gui, const char* p_key, const char* p_img_path);


/// @brief 添加窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] x               相对父窗口X坐标
/// @param [in] y               相对父窗口Y坐标
/// @param [in] w               宽
/// @param [in] h               高
/// @param [out] **p_out_wnd    窗口指针: 仅可访问
/// @return int 0.成功; 非0.失败(错误码)
/// @note 前父窗口必须存在; eg. "/home/btn1" 则需要 "/home" 必须存在, 才能添加
KLB_API int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style);


/// @brief 移除窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 模态显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_do_model(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 关闭显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_end_model(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 关闭所有显示窗口
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_end_model_all(klb_gui_t* p_gui);


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


/// @brief 消息框: 弹出消息框
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/messagebox1"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_messagebox(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 关闭消息框
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_messagebox_end(klb_gui_t* p_gui);


/// @brief 向控件(窗口)绑定事件响应函数
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] on_command      事件响应函数(单击,双击等)
/// @param [in] *p_obj          用户附加数据
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj);


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


/// @brief 重置控件大小
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
KLB_API int klb_gui_resize(klb_gui_t* p_gui, const char* p_path_name, int w, int h);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_H__
//end
