///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vi_chn.go
/// @author lishaoliang
/// @brief	vi channel
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"
import "fmt"

// ViChnAttr C.VI_CHN_ATTR_S
type ViChnAttr C.VI_CHN_ATTR_S

// ViChn hi vi chn
// vi通道
type ViChn struct {
	Dev *ViDev

	Chn    int
	enable bool

	//attr ViChnAttr
}

// ViChnCreate create vi chn
// 创建
func ViChnCreate(dev *ViDev, chn int, attr *ViChnAttr) *ViChn {
	var m ViChn

	m.Dev = dev
	m.Chn = chn
	m.enable = false

	m.SetChnAttr(attr)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *ViChn) Destroy() {
	m.Disable()
}

// SetChnAttr set vi chn attr
func (m *ViChn) SetChnAttr(attr *ViChnAttr) int {

	ret := C.HI_MPI_VI_SetChnAttr(C.VI_CHN(m.Chn), (*C.VI_CHN_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vi_chn.SetChnAttr error!HI_MPI_VI_SetChnAttr=0x%X,vi chn=%d\n", uint(ret), m.Chn)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *ViChn) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VI_EnableChn(C.VI_CHN(m.Chn))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vi_chn.Enable ok!HI_MPI_VI_EnableChn vi chn=%d\n", m.Chn)
	} else {
		fmt.Printf("himpp.vi_chn.Enable error!HI_MPI_VI_EnableChn=0x%X,vi chn=%d\n", uint(ret), m.Chn)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *ViChn) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VI_DisableChn(C.VI_CHN(m.Chn))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vi_chn.Disable ok!HI_MPI_VI_DisableChn vi chn=%d\n", m.Chn)
	} else {
		fmt.Printf("himpp.vi_chn.Disable error!HI_MPI_VI_DisableChn=0x%X,vi chn=%d\n", uint(ret), m.Chn)
	}

	return int(ret)
}
