#ifndef __KLB_WND_IN_H__
#define __KLB_WND_IN_H__

#include "gui/klb_wnd.h"


int klb_wnd_push_child(klb_wnd_t* p_parent, klb_wnd_t* p_wnd);


klb_wnd_t* klb_wnd_get_top(klb_wnd_t* p_wnd);

int klb_wnd_set_top(klb_wnd_t* p_wnd, klb_gui_t* p_gui);
bool klb_wnd_is_top(klb_wnd_t* p_wnd);

void klb_wnd_set_hide(klb_wnd_t* p_wnd, bool hide);
void klb_wnd_set_focus(klb_wnd_t* p_wnd, bool focus);
void klb_wnd_set_calculate(klb_wnd_t* p_wnd, bool calculate);

int klb_wnd_draw(klb_wnd_t* p_wnd);

int klb_wnd_on_draw(klb_wnd_t* p_wnd);
int klb_wnd_on_paint(klb_wnd_t* p_wnd);

int klb_wnd_try_calculate_rect(klb_wnd_t* p_wnd);

/// @brief 重新计算基于画布的区域
/// @param [in] *p_wnd  窗口对象
/// @return 无
///  \n 重新计算 klb_wnd_t.pos.rect_in_canvas
void klb_wnd_calculate_rect_in_canvas(klb_wnd_t* p_wnd);


/// @brief 点(x,y)所处在的第一个窗口
/// @param [in] *p_wnd  窗口
/// @param [in] x       X坐标
/// @param [in] y       Y坐标
/// @return klb_wnd_t* 窗口
klb_wnd_t* klb_wnd_pt_in(klb_wnd_t* p_wnd, int x, int y);



#endif // __KLB_WND_IN_H__
//end
