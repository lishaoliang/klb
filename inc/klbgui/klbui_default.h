///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_default.h
/// @brief   gui部分的默认值
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_DEFAULT_H__
#define __KLBUI_DEFAULT_H__


#include "klb_type.h"
#include "klbthird/sds.h"


#if defined(__cplusplus)
extern "C" {
#endif


/// @struct klbui_default_t
/// @brief  gui部分的默认值
typedef struct klbui_default_t_
{
    // dialog / dlg



    // button / btn
    uint32_t    btn_color_bg_normal;        // button color background normal
    uint32_t    btn_color_bg_focus;         // button color background focus
    uint32_t    btn_color_bg_disable;       // button color background disable
}klbui_default_t;


#ifdef __cplusplus
}
#endif

#endif // __KLBUI_DEFAULT_H__
//end
