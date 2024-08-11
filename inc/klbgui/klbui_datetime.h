///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_datetime.h
/// @author  随风(https://gitee.com/klua/klb)
/// @brief   日期时间等相关定义
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_DATETIME_H__
#define __KLBUI_DATETIME_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct klb_gui_t_ klb_gui_t;


//////////////////////////////////////////////////////////////////////////
// 日期/时间格式


// 日期格式
#define KLBUI_DATE_YMD1         0       // 年月日: "YYYY/MM/DD"
#define KLBUI_DATE_MDY1         1       // 月日年: "MM/DD/YYYY"
#define KLBUI_DATE_DMY1         2       // 日月年: "DD/MM/YYYY"

#define KLBUI_DATE_YMD2         3       // 年月日: "YYYY-MM-DD"
#define KLBUI_DATE_MDY2         4       // 月日年: "MM-DD-YYYY"
#define KLBUI_DATE_DMY2         5       // 日月年: "DD-MM-YYYY"


// 时间格式
#define KLBUI_TIME_24H          0       // 24小时制: "24H"
#define KLBUI_TIME_12H          1       // 12小时制: "12H"


/// @brief 设置/获取日期格式: KLBUI_DATE_YMD1
KLB_API void klb_gui_set_datefmt(klb_gui_t* p_gui, int fmt);
KLB_API int klb_gui_get_datefmt(klb_gui_t* p_gui);


/// @brief 设置/获取时间格式: KLBUI_TIME_24H
KLB_API void klb_gui_set_timefmt(klb_gui_t* p_gui, int fmt);
KLB_API int klb_gui_get_timefmt(klb_gui_t* p_gui);


//////////////////////////////////////////////////////////////////////////
// 日历相关


/// @brief 获取某年某月, 拥有的天数
KLB_API int klbui_month_days(int year, int month);


/// @brief 获取某年某月某日, 为星期几
KLB_API int klbui_weekday(int year, int month, int day);


/// @brief 获取某年某月的 前一个月份
KLB_API void klbui_prev_month(int year, int month, int* p_prev_year, int* p_prev_month);


/// @brief 获取某年某月的 后一个月份
KLB_API void klbui_next_month(int year, int month, int* p_next_year, int* p_next_month);


#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_DATETIME_H__
//end
