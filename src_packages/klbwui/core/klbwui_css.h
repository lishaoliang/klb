// Doc Encode : UTF-8 BOM, Unix(LF)
///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbwui_css.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   klbwui wui 扩展 CSS 类型与帮助函数 (core 专用)
/// @version 0.1
/// @history 修改历史
///   \n 2026 0.1 创建桩
///   \n [2026] 合并 klbwui_cssex; 补充 klbwuicss_draw_background
///   \n [2026] 补充 background-image-mode / background-image-color-key
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBWUI_CSS_H__
#define __KLBWUI_CSS_H__


#include "klb_type.h"
#include "klbgui/klbui_css.h"
#include "klbgui/klb_wnd.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// CSS 辅助函数

/// @brief 背景图画法 background-image-mode
/// @param [in] *p_bg       背景 CSS
/// @param [in] *p_wnd      窗口; 可 NULL (globalcss)
/// @param [in] method      KLBUI_CSSEX_get / KLBUI_CSSEX_set
/// @param [in] *p_in       入参 map
/// @param [in, out] *p_out 出参 map
/// @return 无
/// @note 取值: "default" / "scale9"
void klbwuicss_background_image_mode(klbuicss_background_t* p_bg, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);

/// @brief 背景图关键色透明 background-image-color-key
/// @param [in] *p_bg       背景 CSS
/// @param [in] *p_wnd      窗口; 可 NULL (globalcss)
/// @param [in] method      KLBUI_CSSEX_get / KLBUI_CSSEX_set
/// @param [in] *p_in       入参 map
/// @param [in, out] *p_out 出参 map
/// @return 无
void klbwuicss_background_image_color_key(klbuicss_background_t* p_bg, klb_wnd_t* p_wnd, int method, const klb_map_t* p_in, klb_map_t* p_out);


//////////////////////////////////////////////////////////////////////////
// CSS 扩展绘图函数

/// @brief 按 CSS 背景属性绘制背景
/// @param [in] *p_wnd      窗口
/// @param [in] *p_rect     绘制区域
/// @param [in] *p_bg_css   背景 CSS; 可 NULL (不绘制)
/// @return 无
/// @note 有 background-image 时按 image_mode / image_flags 选绘制路径; 否则填充 background-color
void klbwuicss_draw_background(klb_wnd_t* p_wnd, klb_rect_t* p_rect, klbuicss_background_t* p_bg_css);


#if defined(__cplusplus)
}
#endif


#endif // __KLBWUI_CSS_H__
// end