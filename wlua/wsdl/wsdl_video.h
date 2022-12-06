///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2022, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    wsdl_video.h
/// @brief   视频解码处理
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __WSDL_VIDEO_H__
#define __WSDL_VIDEO_H__


#include "klb_type.h"
#include "klbmem/klb_buf.h"
#include "klbbase/klb_mnp.h"

#include "ffmpeg_include.h"
#include "ffmpeg_dec.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct wsdl_video_t_
{
    ffmpeg_dec_t*       p_dec;
}wsdl_video_t;


wsdl_video_t* wsdl_video_create();
void wsdl_video_destroy(wsdl_video_t* p_video);


int wsdl_video_decode(wsdl_video_t* p_video, int chnn, int sidx, klb_buf_t* p_media, AVFrame** p_out);


#ifdef __cplusplus
}
#endif

#endif // __WSDL_VIDEO_H__
//end
