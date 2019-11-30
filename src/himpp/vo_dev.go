///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vo_dev.go
/// @author lishaoliang
/// @brief	vo dev
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// VoDevAttr C.VO_PUB_ATTR_S
// 原始结构较复杂, 且与版本平台有关
// 这里直接使用原始C结构体, 不做转换;
type VoDevAttr C.VO_PUB_ATTR_S

// VoDev hi vo dev
// vo设备
type VoDev struct {
	DevID  int  // vo 设备ID
	enable bool // 是否使能
}

// VoDevCreate create vo dev
// 创建
func VoDevCreate(id int, attr *VoDevAttr) *VoDev {
	var m VoDev

	m.DevID = id
	m.enable = false

	m.SetDevAttr(attr)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VoDev) Destroy() {
	m.Disable()
}

// SetDevAttr set attr
func (m *VoDev) SetDevAttr(attr *VoDevAttr) int {

	ret := C.HI_MPI_VO_SetPubAttr(C.VO_DEV(m.DevID), (*C.VO_PUB_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vo_dev.SetDevAttr error!HI_MPI_VO_SetPubAttr=0x%X,vo dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *VoDev) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_Enable(C.VO_DEV(m.DevID))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vo_dev.Enable ok!HI_MPI_VO_Enable vo dev=%d\n", m.DevID)
	} else {
		fmt.Printf("himpp.vo_dev.Enable error!HI_MPI_VO_Enable=0x%X,vo dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *VoDev) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_Disable(C.VO_DEV(m.DevID))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vo_dev.Disable ok!HI_MPI_VO_Disable vo dev=%d\n", m.DevID)
	} else {
		fmt.Printf("himpp.vo_dev.Disable error!HI_MPI_VO_Disable=0x%X,vo dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}
