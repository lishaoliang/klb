#ifndef __KLB_WND_IN_H__
#define __KLB_WND_IN_H__


#include "klbgui/klb_wnd.h"


#if defined(__cplusplus)
extern "C" {
#endif


//int klb_wnd_push_child(klb_wnd_t* p_parent, klb_wnd_t* p_wnd);


klb_wnd_t* klb_wnd_get_top(klb_wnd_t* p_wnd);

int klb_wnd_set_top(klb_wnd_t* p_wnd);
bool klb_wnd_is_top(klb_wnd_t* p_wnd);

// 设置聚焦
void klb_wnd_set_focus(klb_wnd_t* p_wnd, bool focus);

// 更新并计算 屏幕坐标
void klb_wnd_calculate_canvas_rect(klb_wnd_t* p_wnd, int offset_x, int offset_y);

// 绘制窗口
int klb_wnd_draw(klb_wnd_t* p_wnd);


/// @brief 点(x,y)所处在的第一个窗口
/// @param [in] *p_wnd  窗口
/// @param [in] x       X坐标
/// @param [in] y       Y坐标
/// @return klb_wnd_t* 窗口
klb_wnd_t* klb_wnd_pt_in(klb_wnd_t* p_wnd, int x, int y);


#ifdef __cplusplus
}
#endif


#endif // __KLB_WND_IN_H__
//end
