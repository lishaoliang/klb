///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbshw_combomenu.h
/// @brief   klb share window combo menu, "kcombo"控件专用弹出(菜单)
/// @version 0.1
/// @history 修改历史
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBSHW_COMBOMENU_H__
#define __KLBSHW_COMBOMENU_H__

#include "klb_type.h"
#include "klbgui/klb_gui.h"
#include "klbutil/klb_map.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define KLBSHW_combomenu            "/klbui/combomenu"


/// @brief 获取"kcombo"控件的专用弹出(菜单)
KLB_API klb_wnd_t* klbui_shwnd_get_combomenu(klb_gui_t* p_gui);


/// @brief 结束之后的响应函数
/// @param [in] *p_wnd      窗体对象
/// @param [in] msg         消息命令
/// @param [in] *p_p1       点1
/// @param [in] *p_p2       点2
/// @return int 0
typedef int(*klbshw_combomenu_result_cb)(void* ptr, bool ok, const sds value, const sds title);


/// @brief 绑定数据, 及响应等
KLB_API int klbshw_combomenu_bind(klb_wnd_t* p_wnd, klbshw_combomenu_result_cb cb_result, void* ptr, klb_map_t* p_data_array, int* p_out_w, int* p_out_h);



#if defined(__cplusplus)
}
#endif

#endif // __KLBSHW_COMBOMENU_H__
//end
