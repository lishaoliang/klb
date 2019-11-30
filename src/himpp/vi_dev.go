///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vi_dev.go
/// @author lishaoliang
/// @brief	vi dev
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// ViDevAttr C.VI_DEV_ATTR_S
// 原始结构较复杂, 且与版本平台有关
// 这里直接使用原始C结构体, 不做转换;
type ViDevAttr C.VI_DEV_ATTR_S

// ViDev hi vi dev
// vi设备
type ViDev struct {
	DevID  int  // vi 设备ID
	enable bool // 是否使能
}

// ViDevCreate create vi dev
// 创建
func ViDevCreate(id int, attr *ViDevAttr) *ViDev {
	var m ViDev

	m.DevID = id
	m.enable = false

	m.SetDevAttr(attr)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *ViDev) Destroy() {
	m.Disable()
}

// SetDevAttr set attr
func (m *ViDev) SetDevAttr(attr *ViDevAttr) int {

	ret := C.HI_MPI_VI_SetDevAttr(C.VI_DEV(m.DevID), (*C.VI_DEV_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vi_dev.SetDevAttr error!HI_MPI_VI_SetDevAttr=0x%X,vi dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *ViDev) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VI_EnableDev(C.VI_DEV(m.DevID))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vi_dev.Enable ok!HI_MPI_VI_EnableDev vi dev=%d\n", m.DevID)
	} else {
		fmt.Printf("himpp.vi_dev.Enable error!HI_MPI_VI_EnableDev=0x%X,vi dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *ViDev) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VI_DisableDev(C.VI_DEV(m.DevID))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vi_dev.Disable ok!HI_MPI_VI_DisableDev vi dev=%d\n", m.DevID)
	} else {
		fmt.Printf("himpp.vi_dev.Disable error!HI_MPI_VI_DisableDev=0x%X,vi dev=%d\n", uint(ret), m.DevID)
	}

	return int(ret)
}
