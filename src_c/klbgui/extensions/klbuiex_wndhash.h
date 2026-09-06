///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbuiex_wndhash.h
/// @brief   klb gui extensions window hash : 存储wnd创建函数,存储wnd,及hash查找表
/// @version 0.1
/// @history 修改历史
///   \n [2023-8]将窗口查找方式从一级, 调整为采用二级索引
///      调整后: 第一级从"根目录"(即顶层窗口路径, eg."/home")做hash
///              第二级以单个顶层窗口为单位做hash
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUIEX_WNDHASH_H__
#define __KLBUIEX_WNDHASH_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/extensions/klbuiex_flex.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klbuiex_wndhash_t_ klbuiex_wndhash_t;


int klbuiex_register_wndhash(klb_gui_t* p_gui);
klbuiex_wndhash_t* klbuiex_get_wndhash(klb_gui_t* p_gui);

int klbuiex_wndhash_set_flex(klbuiex_wndhash_t* p_wndhash, klbuiex_flex_t* p_flex);

/// @brief 注册窗口类型
/// @param [in] *p_type         窗口类型名
/// @param [in] cb_create       创建窗口
/// @return int 0.成功; 非0.失败(错误码)
///  \n 标准控件类型名命名规则为 "k*", eg."kbutton", "kdialog"
int klbuiex_wndhash_register(klbuiex_wndhash_t* p_wndhash, const char* p_type, klb_wnd_create_cb cb_create);

/// @brief 获取注册的窗口类型的 创建函数
/// @param [in] *p_gui          GUI对象
/// @param [in] *p_type         窗口类型名
/// @return klb_wnd_create_cb 创建函数; 或 NULL
klb_wnd_create_cb klbuiex_wndhash_get_creater(klbuiex_wndhash_t* p_wndhash, const char* p_type);


/// @brief 添加窗口
int klbuiex_wndhash_append(klbuiex_wndhash_t* p_wndhash, const char* p_type, const char* p_path_name, int x, int y, int w, int h, uint32_t style, klb_wnd_t** p_out_wnd);

/// @brief 移除窗口
int klbuiex_wndhash_remove(klbuiex_wndhash_t* p_wndhash, const char* p_path_name);

/// @brief 查找窗口
klb_wnd_t* klbuiex_wndhash_find(klbuiex_wndhash_t* p_wndhash, const char* p_path_name);


#ifdef __cplusplus
}
#endif

#endif // __KLBUIEX_WNDHASH_H__
//end
