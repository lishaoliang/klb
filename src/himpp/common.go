///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   common.go
/// @author lishaoliang
/// @brief	common
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// MOD_ID_E
// 模块ID
const (
	IDCMPI = int(C.HI_ID_CMPI)
	IDVB   = int(C.HI_ID_VB)
	IDSYS  = int(C.HI_ID_SYS)
	IDRGN  = int(C.HI_ID_RGN)
	IDCHNL = int(C.HI_ID_CHNL)

	IDVDEC    = int(C.HI_ID_VDEC)
	IDGROUP   = int(C.HI_ID_GROUP)
	IDVPSS    = int(C.HI_ID_VPSS)
	IDVENC    = int(C.HI_ID_VENC)
	IDVDA     = int(C.HI_ID_VDA)
	IDH264E   = int(C.HI_ID_H264E)
	IDJPEGE   = int(C.HI_ID_JPEGE)
	IDMPEG4E  = int(C.HI_ID_MPEG4E)
	IDH264D   = int(C.HI_ID_H264D)
	IDJPEGD   = int(C.HI_ID_JPEGD)
	IDVOU     = int(C.HI_ID_VOU)
	IDVIU     = int(C.HI_ID_VIU)
	IDDSU     = int(C.HI_ID_DSU)
	IDVALG    = int(C.HI_ID_VALG)
	IDRC      = int(C.HI_ID_RC)
	IDAIO     = int(C.HI_ID_AIO)
	IDAI      = int(C.HI_ID_AI)
	IDAO      = int(C.HI_ID_AO)
	IDAENC    = int(C.HI_ID_AENC)
	IDADEC    = int(C.HI_ID_ADEC)
	IDAVENC   = int(C.HI_ID_AVENC)
	IDPCIV    = int(C.HI_ID_PCIV)
	IDPCIVFMW = int(C.HI_ID_PCIVFMW)
	IDISP     = int(C.HI_ID_ISP)
	IDIVE     = int(C.HI_ID_IVE)

	IDDCCM   = int(C.HI_ID_DCCM)
	IDDCCS   = int(C.HI_ID_DCCS)
	IDPROC   = int(C.HI_ID_PROC)
	IDLOG    = int(C.HI_ID_LOG)
	IDMSTLOG = int(C.HI_ID_MST_LOG)
	IDVD     = int(C.HI_ID_VD)

	IDVCMP    = int(C.HI_ID_VCMP)
	IDFB      = int(C.HI_ID_FB)
	IDHDMI    = int(C.HI_ID_HDMI)
	IDVOIE    = int(C.HI_ID_VOIE)
	IDTDE     = int(C.HI_ID_TDE)
	IDUSR     = int(C.HI_ID_USR)
	IDVEDU    = int(C.HI_ID_VEDU)
	IDVGS     = int(C.HI_ID_VGS)
	IDH265E   = int(C.HI_ID_H265E)
	IDFD      = int(C.HI_ID_FD)
	IDODT     = int(C.HI_ID_ODT)
	IDVQA     = int(C.HI_ID_VQA)
	IDLPR     = int(C.HI_ID_LPR)
	IDFISHEYE = int(C.HI_ID_FISHEYE)
	//IDPHOTO   = int(C.HI_ID_PHOTO)

	//IDEXTAO = int(C.HI_ID_EXTAO)
	//IDBUTT  = int(C.HI_ID_BUTT)
)

// MppChn MPP_CHN_S
// mpp通道描述
type MppChn struct {
	ModID int // 模块ID
	DevID int // 设备ID
	ChnID int // 通道ID
}

// MPPCHNS C.MPP_CHN_S
type MPPCHNS C.MPP_CHN_S

// MppChnCopy copy C to Go
// 从C结构体拷贝到MppChn
// copy语意定义为从其他语言拷贝到 Go
func MppChnCopy(dst *MppChn, src *MPPCHNS) {
	dst.ModID = int(src.enModId)
	dst.DevID = int(src.s32DevId)
	dst.ChnID = int(src.s32ChnId)
}

// MppChnCopyC copy Go to C
// 从MppChn拷贝到C结构体
func MppChnCopyC(dst *MPPCHNS, src *MppChn) {
	dst.enModId = C.MOD_ID_E(src.ModID)
	dst.s32DevId = C.HI_S32(src.DevID)
	dst.s32ChnId = C.HI_S32(src.ChnID)
}
