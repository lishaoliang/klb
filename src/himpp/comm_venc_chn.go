///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_venc_chn.go
/// @author lishaoliang
/// @brief	comm venc chn
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// VencChnStat venc chn statue
type VencChnStat struct {
	LeftPics         uint // 待编码的图片数
	LeftStreamBytes  uint // 码流buffer剩余的byte数
	LeftStreamFrames uint // 码流buffer剩余的帧数
	CurPacks         uint // 当前帧的码流包个数
	LeftRecvPics     uint // 剩余待接收的帧数, HI_MPI_VENC_StartRecvPicEx有效
	LeftEncPics      uint // 剩余待编码的帧数, HI_MPI_VENC_StartRecvPicEx有效
}

// VENCCHNSTATS C.VENC_CHN_STAT_S
type VENCCHNSTATS C.VENC_CHN_STAT_S

// VencChnStatCopy copy C to Go
// copy语意约定为从其他语言拷贝到 Go
func VencChnStatCopy(dst *VencChnStat, src *VENCCHNSTATS) {
	dst.LeftPics = uint(src.u32LeftPics)
	dst.LeftStreamBytes = uint(src.u32LeftStreamBytes)
	dst.LeftStreamFrames = uint(src.u32LeftStreamFrames)
	dst.CurPacks = uint(src.u32CurPacks)
	dst.LeftRecvPics = uint(src.u32LeftRecvPics)
	dst.LeftEncPics = uint(src.u32LeftEncPics)
}

// VencChnStatCopyC copy Go to C
func VencChnStatCopyC(dst *VENCCHNSTATS, src *VencChnStat) {
	dst.u32LeftPics = C.HI_U32(src.LeftPics)
	dst.u32LeftStreamBytes = C.HI_U32(src.LeftStreamBytes)
	dst.u32LeftStreamFrames = C.HI_U32(src.LeftStreamFrames)
	dst.u32CurPacks = C.HI_U32(src.CurPacks)
	dst.u32LeftRecvPics = C.HI_U32(src.LeftRecvPics)
	dst.u32LeftEncPics = C.HI_U32(src.LeftEncPics)
}
