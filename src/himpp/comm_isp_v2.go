///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_isp_v2.go
/// @author lishaoliang
/// @brief	comm isp v2
///  usage: go build -tags="himppv2"
///////////////////////////////////////////////////////////////////////////

// +build himppv2

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// IspPubAttr isp pub attr
type IspPubAttr struct {
	WndRec Rect
	//SnsSize   Size
	FrameRate float32
	BayerFmt  int
}

// ISPPUBATTRS C.ISP_PUB_ATTR_S
type ISPPUBATTRS C.ISP_PUB_ATTR_S

// IspPubAttrCopy copy C to Go
// copy语意约定为从其他语言拷贝到 GO
func IspPubAttrCopy(dst *IspPubAttr, src *ISPPUBATTRS) {
	RectCopy(&dst.WndRec, (*RECTS)(&src.stWndRect))
	//SizeCopy(&dst.SnsSize, (*SIZES)(&src.stSnsSize))

	dst.FrameRate = float32(src.f32FrameRate)
	dst.BayerFmt = int(src.enBayer)
}

// IspPubAttrCopyC copy Go to C
func IspPubAttrCopyC(dst *ISPPUBATTRS, src *IspPubAttr) {
	RectCopyC((*RECTS)(&dst.stWndRect), &src.WndRec)
	//SizeCopyC((*SIZES)(&dst.stSnsSize), &src.SnsSize)

	dst.f32FrameRate = C.float(src.FrameRate)
	dst.enBayer = C.ISP_BAYER_FORMAT_E(src.BayerFmt)
}
