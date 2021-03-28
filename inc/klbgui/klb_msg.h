///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klb_msg.h
/// @brief   消息定义
/// @author  李绍良
///  \n https://github.com/lishaoliang/klb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLB_MSG_H__
#define __KLB_MSG_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif


/****************************************************************************
*                                                                           *
* winuser.h -- USER procedure declarations, constant definitions and macros *
*                                                                           *
* Copyright (c) Microsoft Corporation. All rights reserved.                 *
*                                                                           *
****************************************************************************/

/****************************************************************************
* 拷贝 windows 标准键盘值. range 0x01 ~ 0xFF                                *
****************************************************************************/

/*
* Virtual Keys, Standard Set
*/
#define KLB_VK_LBUTTON        0x01
#define KLB_VK_RBUTTON        0x02
#define KLB_VK_CANCEL         0x03
#define KLB_VK_MBUTTON        0x04    /* NOT contiguous with L & RBUTTON */

/*
* 0x07 : unassigned
*/

#define KLB_VK_BACK           0x08
#define KLB_VK_TAB            0x09

/*
* 0x0A - 0x0B : reserved
*/

#define KLB_VK_CLEAR          0x0C
#define KLB_VK_RETURN         0x0D

#define KLB_VK_SHIFT          0x10
#define KLB_VK_CONTROL        0x11
#define KLB_VK_MENU           0x12
#define KLB_VK_PAUSE          0x13
#define KLB_VK_CAPITAL        0x14

#define KLB_VK_KANA           0x15
#define KLB_VK_HANGEUL        0x15  /* old name - should be here for compatibility */
#define KLB_VK_HANGUL         0x15
#define KLB_VK_JUNJA          0x17
#define KLB_VK_FINAL          0x18
#define KLB_VK_HANJA          0x19
#define KLB_VK_KANJI          0x19

#define KLB_VK_ESCAPE         0x1B

#define KLB_VK_CONVERT        0x1C
#define KLB_VK_NONCONVERT     0x1D
#define KLB_VK_ACCEPT         0x1E
#define KLB_VK_MODECHANGE     0x1F

#define KLB_VK_SPACE          0x20
#define KLB_VK_PRIOR          0x21
#define KLB_VK_NEXT           0x22
#define KLB_VK_END            0x23
#define KLB_VK_HOME           0x24
#define KLB_VK_LEFT           0x25
#define KLB_VK_UP             0x26
#define KLB_VK_RIGHT          0x27
#define KLB_VK_DOWN           0x28
#define KLB_VK_SELECT         0x29
#define KLB_VK_PRINT          0x2A
#define KLB_VK_EXECUTE        0x2B
#define KLB_VK_SNAPSHOT       0x2C
#define KLB_VK_INSERT         0x2D
#define KLB_VK_DELETE         0x2E
#define KLB_VK_HELP           0x2F

/*
* VK_0 - VK_9 are the same as ASCII '0' - '9' (0x30 - 0x39)
* 0x40 : unassigned
* VK_A - VK_Z are the same as ASCII 'A' - 'Z' (0x41 - 0x5A)
*/

#define KLB_VK_LWIN           0x5B
#define KLB_VK_RWIN           0x5C
#define KLB_VK_APPS           0x5D

/*
* 0x5E : reserved
*/

#define KLB_VK_SLEEP          0x5F

#define KLB_VK_NUMPAD0        0x60
#define KLB_VK_NUMPAD1        0x61
#define KLB_VK_NUMPAD2        0x62
#define KLB_VK_NUMPAD3        0x63
#define KLB_VK_NUMPAD4        0x64
#define KLB_VK_NUMPAD5        0x65
#define KLB_VK_NUMPAD6        0x66
#define KLB_VK_NUMPAD7        0x67
#define KLB_VK_NUMPAD8        0x68
#define KLB_VK_NUMPAD9        0x69
#define KLB_VK_MULTIPLY       0x6A
#define KLB_VK_ADD            0x6B
#define KLB_VK_SEPARATOR      0x6C
#define KLB_VK_SUBTRACT       0x6D
#define KLB_VK_DECIMAL        0x6E
#define KLB_VK_DIVIDE         0x6F
#define KLB_VK_F1             0x70
#define KLB_VK_F2             0x71
#define KLB_VK_F3             0x72
#define KLB_VK_F4             0x73
#define KLB_VK_F5             0x74
#define KLB_VK_F6             0x75
#define KLB_VK_F7             0x76
#define KLB_VK_F8             0x77
#define KLB_VK_F9             0x78
#define KLB_VK_F10            0x79
#define KLB_VK_F11            0x7A
#define KLB_VK_F12            0x7B
#define KLB_VK_F13            0x7C
#define KLB_VK_F14            0x7D
#define KLB_VK_F15            0x7E
#define KLB_VK_F16            0x7F
#define KLB_VK_F17            0x80
#define KLB_VK_F18            0x81
#define KLB_VK_F19            0x82
#define KLB_VK_F20            0x83
#define KLB_VK_F21            0x84
#define KLB_VK_F22            0x85
#define KLB_VK_F23            0x86
#define KLB_VK_F24            0x87

/*
* 0x88 - 0x8F : unassigned
*/

#define KLB_VK_NUMLOCK        0x90
#define KLB_VK_SCROLL         0x91

/*
* NEC PC-9800 kbd definitions
*/
#define KLB_VK_OEM_NEC_EQUAL  0x92   // '=' key on numpad

/*
* Fujitsu/OASYS kbd definitions
*/
#define KLB_VK_OEM_FJ_JISHO   0x92   // 'Dictionary' key
#define KLB_VK_OEM_FJ_MASSHOU 0x93   // 'Unregister word' key
#define KLB_VK_OEM_FJ_TOUROKU 0x94   // 'Register word' key
#define KLB_VK_OEM_FJ_LOYA    0x95   // 'Left OYAYUBI' key
#define KLB_VK_OEM_FJ_ROYA    0x96   // 'Right OYAYUBI' key

/*
* 0x97 - 0x9F : unassigned
*/

/*
* VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
* Used only as parameters to GetAsyncKeyState() and GetKeyState().
* No other API or message will distinguish left and right keys in this way.
*/
#define KLB_VK_LSHIFT         0xA0
#define KLB_VK_RSHIFT         0xA1
#define KLB_VK_LCONTROL       0xA2
#define KLB_VK_RCONTROL       0xA3
#define KLB_VK_LMENU          0xA4
#define KLB_VK_RMENU          0xA5

/*
* 0xB8 - 0xB9 : reserved
*/

#define KLB_VK_OEM_1          0xBA   // ';:' for US
#define KLB_VK_OEM_PLUS       0xBB   // '+' any country
#define KLB_VK_OEM_COMMA      0xBC   // ',' any country
#define KLB_VK_OEM_MINUS      0xBD   // '-' any country
#define KLB_VK_OEM_PERIOD     0xBE   // '.' any country
#define KLB_VK_OEM_2          0xBF   // '/?' for US
#define KLB_VK_OEM_3          0xC0   // '`~' for US

/*
* 0xC1 - 0xD7 : reserved
*/

/*
* 0xD8 - 0xDA : unassigned
*/

#define KLB_VK_OEM_4          0xDB  //  '[{' for US
#define KLB_VK_OEM_5          0xDC  //  '\|' for US
#define KLB_VK_OEM_6          0xDD  //  ']}' for US
#define KLB_VK_OEM_7          0xDE  //  ''"' for US
#define KLB_VK_OEM_8          0xDF

/*
* 0xE0 : reserved
*/

/*
* Various extended or enhanced keyboards
*/
#define KLB_VK_OEM_AX         0xE1  //  'AX' key on Japanese AX kbd
#define KLB_VK_OEM_102        0xE2  //  "<>" or "\|" on RT 102-key kbd.
#define KLB_VK_ICO_HELP       0xE3  //  Help key on ICO
#define KLB_VK_ICO_00         0xE4  //  00 key on ICO

#define KLB_VK_ICO_CLEAR      0xE6

/*
* 0xE8 : unassigned
*/


/*
* Nokia/Ericsson definitions
*/
#define KLB_VK_OEM_RESET      0xE9
#define KLB_VK_OEM_JUMP       0xEA
#define KLB_VK_OEM_PA1        0xEB
#define KLB_VK_OEM_PA2        0xEC
#define KLB_VK_OEM_PA3        0xED
#define KLB_VK_OEM_WSCTRL     0xEE
#define KLB_VK_OEM_CUSEL      0xEF
#define KLB_VK_OEM_ATTN       0xF0
#define KLB_VK_OEM_FINISH     0xF1
#define KLB_VK_OEM_COPY       0xF2
#define KLB_VK_OEM_AUTO       0xF3
#define KLB_VK_OEM_ENLW       0xF4
#define KLB_VK_OEM_BACKTAB    0xF5

#define KLB_VK_ATTN           0xF6
#define KLB_VK_CRSEL          0xF7
#define KLB_VK_EXSEL          0xF8
#define KLB_VK_EREOF          0xF9
#define KLB_VK_PLAY           0xFA
#define KLB_VK_ZOOM           0xFB
#define KLB_VK_NONAME         0xFC
#define KLB_VK_PA1            0xFD
#define KLB_VK_OEM_CLEAR      0xFE

/*
* 0xFF : reserved
*/

#define KLB_WM_MOUSEFIRST       0x0200
#define KLB_WM_MOUSEMOVE        0x0200
#define KLB_WM_LBUTTONDOWN      0x0201
#define KLB_WM_LBUTTONUP        0x0202
#define KLB_WM_LBUTTONDBLCLK    0x0203
#define KLB_WM_RBUTTONDOWN      0x0204
#define KLB_WM_RBUTTONUP        0x0205
#define KLB_WM_RBUTTONDBLCLK    0x0206
#define KLB_WM_MBUTTONDOWN      0x0207
#define KLB_WM_MBUTTONUP        0x0208
#define KLB_WM_MBUTTONDBLCLK    0x0209
#define KLB_WM_MOUSEHWHEEL      0x020E
#define KLB_WM_MOUSELAST        0x020E

/*
* NOTE: All Message Numbers below 0x0400 are RESERVED.
*
* Private Window Messages Start Here:
*/
#define KLB_WM_USER          0x0400



/// @def   KLB_GUI_CTRL_PAINT
/// @brief 绘制消息
#define KLB_GUI_CTRL_PAINT   0x0401


/// @def   KLB_GUI_WM_USER
/// @brief GUI用户消息
///  [0x0000, 0x0400]   参考微软定义
///  (0x0400, 0x1000)   GUI核心定义消息
///  [0x1000, +)        界面消息
#define KLB_GUI_WM_USER      0x1000



#ifdef __cplusplus
}
#endif

#endif // __KLB_MSG_H__
//end