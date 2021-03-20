///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_gui.h
/// @brief   GUI接口定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_GUI_H__
#define __KLB_GUI_H__

#include "klb_type.h"
#include "image/klb_rect.h"
#include "image/klb_canvas.h"
#include "gui/klb_msg.h"
#include "gui/klb_wnd.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;
typedef struct klb_wnd_t_ klb_wnd_t;


/// @brief 创建一个GUI对象
/// @param [in] *p_canvas       主显存画布
/// @return klb_gui_t* GUI对象
klb_gui_t* klb_gui_create(klb_canvas_t* p_canvas);


/// @brief 销毁GUI对象
/// @param [in] *p_gui          GUI对象
/// @return 无
void klb_gui_destroy(klb_gui_t* p_gui);


/// @brief 启动线程
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败
int klb_gui_start(klb_gui_t* p_gui);


/// @brief 关闭线程
/// @param [in] *p_gui          GUI对象
/// @return 无
void klb_gui_stop(klb_gui_t* p_gui);


/// @brief 处理消息(事件)流程
/// @param [in] *p_gui          GUI对象
/// @param [in] tc              系统滴答数(毫秒)
/// @return int 0.有消息(事件处理); 非0.无消息处理
/// @note 此函数为替代 start/stop 流程, 由外部申请线程, 处理UI流程
///  \n 与 start/stop 不能同时使用
///  \n 当有消息(事件)时调用; 定时器定时调用
int klb_gui_loop_once(klb_gui_t* p_gui, int64_t tc);


/// @brief 附加到某个显存画布
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_canvas       画布
/// @return 无
void klb_gui_attach_canvas(klb_gui_t* p_gui, klb_canvas_t* p_canvas);


/// @brief 向GUI中放入消息
/// @param [in] *p_gui          GUI对象
/// @param [in] msg             消息类型: "./gui/klb_msg.h"
/// @return 无
void klb_gui_push(klb_gui_t* p_gui, int msg, int x1, int y1, int x2, int y2, int lparam, int wparam);


/// @brief 创建窗口
/// @param [in] x               相对父窗口X坐标
/// @param [in] y               相对父窗口Y坐标
/// @param [in] w               宽
/// @param [in] h               高
/// @return klb_wnd_t* 窗口对象
typedef klb_wnd_t* (*klb_wnd_create_cb)(int x, int y, int w, int h);


/// @brief 注册窗口类型
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @param [in] create          创建函数
/// @return int 0.成功; 非0.失败(错误码)
///  \n 标准控件类型名命名规则为 "k*", eg."kbutton", "kdialog"
int klb_gui_register(klb_gui_t* p_gui, const char* p_type, klb_wnd_create_cb cb_create);


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
int klb_gui_append(klb_gui_t* p_gui, const char* p_type, const char* p_path_name, int x, int y, int w, int h, klb_wnd_t** p_out_wnd);


/// @brief 移除窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_remove(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 模态显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_do_model(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 关闭显示窗口
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_end_model(klb_gui_t* p_gui, const char* p_path_name);


/// @brief 关闭所有显示窗口
/// @param [in] *p_gui          GUI对象
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_end_model_all(klb_gui_t* p_gui);


/// @brief 向控件(窗口)绑定事件响应函数
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix文件路径): eg."/home/btn1"
/// @param [in] on_command      事件响应函数(单击,双击等)
/// @param [in] *p_obj          用户附加数据
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_bind_command(klb_gui_t* p_gui, const char* p_path_name, klb_wnd_on_command_cb on_command, void* p_obj);


/// @brief 向控件(窗口)设置数据: 样式\显示\状态等等
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @param [in] *p_json         JSON格式数据
/// @return int 0.成功; 非0.失败(错误码)
int klb_gui_set(klb_gui_t* p_gui, const char* p_path_name, const char* p_json);


/// @brief 向控件(窗口)获取数据: 样式\显示\状态等等
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_path_name    窗口路径(类unix): eg."/home"
/// @param [in] *p_json         JSON格式数据
/// @return char* JSON串或NULL
char* klb_gui_get(klb_gui_t* p_gui, const char* p_path_name, const char* p_json);


#ifdef __cplusplus
}
#endif

#endif // __KLB_GUI_H__
//end
