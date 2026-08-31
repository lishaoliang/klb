// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_combo_menu.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   combo 弹出菜单 共享窗口
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_COMBO_MENU_H__
#define __KLBSHW_COMBO_MENU_H__


#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbgui/klbui_shwnd.h"
#include "klbutil/klb_map.h"
#include "klbutil/klb_sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_combo_menu                "/klbui/combo-menu"


/// @brief 获取 combo 弹出菜单
/// @param [in] *p_gui      GUI 对象
/// @return klb_wnd_t* 共享窗口; 首次调用时创建并 push
klb_wnd_t* klbui_shwnd_get_combo_menu(klb_gui_t* p_gui);


/// @brief Popup 弹窗结束之前的回调
/// @param [in] *ptr        绑定的指针
/// @param [in] ok          是否选定一项
/// @param [in] value       选中项的值; 取消时可为 NULL
/// @param [in] title       选中项的标题; 取消时可为 NULL
/// @return int
typedef int(*klbshw_combo_menu_cb)(void* ptr, bool ok, const sds value, const sds title);


/// @brief 绑定数据与结束回调, 并估算弹出宽高
/// @param [in] *p_wnd          菜单窗口
/// @param [in] cb              回调
/// @param [in] *ptr            绑定指针
/// @param [in] *p_data_array   选项数组; 每项为 map, key=value, data=title
/// @param [out] *p_out_w       估算宽; 可 NULL
/// @param [out] *p_out_h       估算高; 可 NULL
/// @param [in] is_more_col     是否允许多列
/// @param [in] menu_min_w      最小宽度; <=0 时用默认 390
/// @return int 0.成功
int klbshw_combo_menu_bind(klb_wnd_t* p_wnd, klbshw_combo_menu_cb cb, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h, bool is_more_col, int menu_min_w);


/// @brief 按当前绑定数据重新布局
void klbshw_combo_menu_layout(klb_wnd_t* p_wnd);


#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_COMBO_MENU_H__

// end
