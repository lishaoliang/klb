///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    ffmpeg_include.h
/// @brief   文件简要描述
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __FFMPEG_INCLUDE_H__
#define __FFMPEG_INCLUDE_H__

#if defined(__cplusplus)
extern "C" {
#endif


#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/frame.h"
#include "libavutil/error.h"
#include "libavutil/mem.h""
#include "libswresample/swresample.h"


#ifdef __cplusplus
}
#endif

#endif // __FFMPEG_INCLUDE_H__
//end
