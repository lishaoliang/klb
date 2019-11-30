///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_video_s.go
/// @author lishaoliang
/// @brief	comm vide struct
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// PointCopy copy C to Go
// 从C结构体拷贝到Point
func PointCopy(dst *Point, src *POINTS) {
	dst.X = int(src.s32X)
	dst.Y = int(src.s32Y)
}

// PointCopyC copy Go to C
// 从Point拷贝到C结构体
func PointCopyC(dst *POINTS, src *Point) {
	dst.s32X = C.HI_S32(src.X)
	dst.s32Y = C.HI_S32(src.Y)
}

// SizeCopy copy C to Go
// 从C结构体拷贝到Size
func SizeCopy(dst *Size, src *SIZES) {
	dst.W = int(src.u32Width)
	dst.H = int(src.u32Height)
}

// SizeCopyC copy Go to C
// 从Size拷贝到C结构体
func SizeCopyC(dst *SIZES, src *Size) {
	dst.u32Width = C.HI_U32(src.W)
	dst.u32Height = C.HI_U32(src.H)
}

// RectCopy copy C to Go
// 从C结构体拷贝到Rect
func RectCopy(dst *Rect, src *RECTS) {
	dst.X = int(src.s32X)
	dst.Y = int(src.s32Y)
	dst.W = int(src.u32Width)
	dst.H = int(src.u32Height)
}

// RectCopyC copy Go to C
// 从Rect拷贝到C结构体
func RectCopyC(dst *RECTS, src *Rect) {
	dst.s32X = C.HI_S32(src.X)
	dst.s32Y = C.HI_S32(src.Y)
	dst.u32Width = C.HI_U32(src.W)
	dst.u32Height = C.HI_U32(src.H)
}
