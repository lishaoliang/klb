///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vpss_chn.go
/// @author lishaoliang
/// @brief	vpss channel
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// VpssChnAttr vpss chn attr
type VpssChnAttr C.VPSS_CHN_ATTR_S

// VpssChnMode vpss chn mode
type VpssChnMode C.VPSS_CHN_MODE_S

// VpssChn hi vpss channel
// vpss channel
type VpssChn struct {
	Grp *VpssGrp
	Chn int

	enable bool
}

// VpssChnCreate create vpss chn
// 创建
func VpssChnCreate(grp *VpssGrp, chn int, attr *VpssChnAttr, mode *VpssChnMode) *VpssChn {
	var m VpssChn

	m.Grp = grp
	m.Chn = chn
	m.enable = false

	m.SetChnAttr(attr)
	m.SetChnMode(mode)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VpssChn) Destroy() {
	m.Disable()
}

// SetChnAttr HI_MPI_VPSS_SetChnAttr
// 设置属性
func (m *VpssChn) SetChnAttr(attr *VpssChnAttr) int {
	ret := C.HI_MPI_VPSS_SetChnAttr(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn), (*C.VPSS_CHN_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vpss_chn.SetChnAttr error!HI_MPI_VPSS_SetChnAttr=0x%X,vpss grp=%d,chn=%d\n", uint(ret), m.Grp.Grp, m.Chn)
	}

	return int(ret)
}

// SetChnMode HI_MPI_VPSS_SetChnMode
func (m *VpssChn) SetChnMode(mode *VpssChnMode) int {
	ret := C.HI_MPI_VPSS_SetChnMode(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn), (*C.VPSS_CHN_MODE_S)(mode))
	if 0 != ret {
		fmt.Printf("himpp.vpss_chn.SetChnMode error!HI_MPI_VPSS_SetChnMode=0x%X,vpss grp=%d,chn=%d\n", uint(ret), m.Grp.Grp, m.Chn)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *VpssChn) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VPSS_EnableChn(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vpss_chn.Enable ok!HI_MPI_VPSS_EnableChn vpss grp=%d,chn=%d\n", m.Grp.Grp, m.Chn)
	} else {
		fmt.Printf("himpp.vpss_chn.Enable error!HI_MPI_VPSS_EnableChn=0x%X,vpss grp=%d,chn=%d\n", uint(ret), m.Grp.Grp, m.Chn)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *VpssChn) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VPSS_DisableChn(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vpss_chn.Disable ok!HI_MPI_VPSS_DisableChn vpss grp=%d,chn=%d\n", m.Grp.Grp, m.Chn)
	} else {
		fmt.Printf("himpp.vpss_chn.Disable error!HI_MPI_VPSS_DisableChn=0x%X,vpss grp=%d,chn=%d\n", uint(ret), m.Grp.Grp, m.Chn)
	}

	return int(ret)
}

// SetCrop 设置裁剪
func (m *VpssChn) SetCrop(x, y, w, h int) int {
	var crop C.VPSS_CROP_INFO_S

	crop.bEnable = C.HI_TRUE
	crop.enCropCoordinate = C.VPSS_CROP_ABS_COOR

	crop.stCropRect.s32X = C.HI_S32(x)
	crop.stCropRect.s32Y = C.HI_S32(y)
	crop.stCropRect.u32Width = C.HI_U32(w)
	crop.stCropRect.u32Height = C.HI_U32(h)

	ret := C.HI_MPI_VPSS_SetChnCrop(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn), &crop)
	if 0 != ret {
		fmt.Printf("himpp.vpss_chn.SetCrop error!HI_MPI_VPSS_SetChnCrop:[0x%X],vpss grp:[%d],chn:[%d],xywh:[%d,%d,%d,%d]\n", uint(ret), m.Grp.Grp, m.Chn,
			x, y, w, h)
	}

	return int(ret)
}

// SetDepth 设置深度
func (m *VpssChn) SetDepth(depth int) int {
	ret := C.HI_MPI_VPSS_SetDepth(C.VPSS_GRP(m.Grp.Grp), C.VPSS_CHN(m.Chn), C.HI_U32(depth))
	if 0 != ret {
		fmt.Printf("himpp.vpss_chn.SetDepth error!HI_MPI_VPSS_SetDepth:[0x%X],vpss grp:[%d],chn:[%d],depth:[%d]\n", uint(ret), m.Grp.Grp, m.Chn, depth)
	}

	return int(ret)
}
