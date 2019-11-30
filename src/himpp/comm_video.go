///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_video.go
/// @author lishaoliang
/// @brief	comm video
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// Point POINT_S
// 点
type Point struct {
	X int
	Y int
}

// POINTS C.POINT_S
type POINTS C.POINT_S

// Size SIZE_S
// 大小
type Size struct {
	W int
	H int
}

// SIZES C.SIZE_S
type SIZES C.SIZE_S

// Rect RECT_S
// 区域
type Rect struct {
	X int
	Y int
	W int
	H int
}

// RECTS C.RECT_S
type RECTS C.RECT_S

// VIDEOFRAMEINFOS C.VIDEO_FRAME_INFO_S
// video frame格式
type VIDEOFRAMEINFOS C.VIDEO_FRAME_INFO_S

// PIXEL_FORMAT_E
const (
	PixelFormatYuvSemiplanar422 = int(C.PIXEL_FORMAT_YUV_SEMIPLANAR_422)
	PixelFormatYuvSemiplanar420 = int(C.PIXEL_FORMAT_YUV_SEMIPLANAR_420)
)
