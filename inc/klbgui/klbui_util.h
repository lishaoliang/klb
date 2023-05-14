///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    klbui_util.h
/// @brief   其他和UI有相关性的定义或函数
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __KLBUI_UTIL_H__
#define __KLBUI_UTIL_H__

#include "klb_type.h"

#if defined(__cplusplus)
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////
// define

//utf8 -> unicode
#define KLBUI_utf8_to_unicode(UTF8_, UNICODE_, LEN_) \
{ \
    int nnnn = 0; \
    uint8_t uft8_ch = UTF8_[nnnn++]; \
    if((uft8_ch & 0x80) == 0) \
    { \
        UNICODE_ = uft8_ch; \
        LEN_ = 1; \
    } \
    else if((uft8_ch & 0xe0) == 0xe0) \
    { \
        UNICODE_ = (uft8_ch & 0x1F) << 12; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 3; \
    } \
    else \
    { \
        UNICODE_ = (uft8_ch & 0x3F) << 6; \
        uft8_ch = UTF8_[nnnn++]; \
        UNICODE_ |= (uft8_ch & 0x3F); \
        LEN_ = 2; \
    } \
}


#if defined(__cplusplus)
}
#endif

#endif // __KLBUI_UTIL_H__
//end
