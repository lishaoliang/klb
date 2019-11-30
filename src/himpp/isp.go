///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   isp.go
/// @author lishaoliang
/// @brief	isp
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// Isp isp
type Isp struct {
	SensorID int
	enable   bool

	ispAttr IspPubAttr

	ipsSns *IspSns
}

// IspCreate create isp
// eg.
// ispAttr.BayerFmt = int(C.BAYER_RGGB)
// ispAttr.FrameRate = 30.0
// ispAttr.WndRec.W = 1920
// ispAttr.WndRec.H = 1080
// ispAttr.SnsSize.W = 1920
// ispAttr.SnsSize.H = 1080
func IspCreate(sensorID int, ispAttr *IspPubAttr, obj *IspSnsObj) *Isp {
	var m Isp

	m.SensorID = sensorID
	m.ispAttr = *ispAttr
	m.ipsSns = IspSnsCreate(sensorID, obj)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (m *Isp) Destroy() {
	m.Disable()
	m.ipsSns.Destroy()
}

// Enable enable
func (m *Isp) Enable() int {
	if m.enable {
		return 0
	}

	// 注册 sns
	m.ipsSns.Register()

	// isp 内存初始化
	ret := C.HI_MPI_ISP_MemInit(C.ISP_DEV(m.SensorID))
	if 0 != ret {
		fmt.Printf("himpp.isp.Enable error!HI_MPI_ISP_MemInit=0x%X,isp sensor id=%d\n", uint(ret), m.SensorID)
		return int(ret)
	}

	// 必须在初始化前 设置WDR 与 PubAttr, 否则 HI_MPI_ISP_Init=0xA01C8042

	// 设置SetWDRMode
	var wdr IspWdrMode
	wdr.WdrMode = int(C.WDR_MODE_NONE)
	m.SetWDRMode(&wdr)

	// SetPubAttr
	m.SetPubAttr(&m.ispAttr)

	// isp 初始化
	ret = C.HI_MPI_ISP_Init(C.ISP_DEV(m.SensorID))

	if 0 != ret {
		fmt.Printf("himpp.isp.Enable error!HI_MPI_ISP_Init=0x%X,isp sensor id=%d\n", uint(ret), m.SensorID)
		return int(ret)
	}

	m.enable = true
	fmt.Printf("himpp.isp.Enable ok!HI_MPI_ISP_Init isp sensor id=%d\n", m.SensorID)

	// 初始化成功后, 启动线程
	go func(sensorID int) {
		C.HI_MPI_ISP_Run(C.ISP_DEV(sensorID))
	}(m.SensorID)

	return int(ret)
}

// Disable disable
func (m *Isp) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_ISP_Exit(C.ISP_DEV(m.SensorID))

	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.isp.Disable ok!HI_MPI_ISP_Exit isp sensor id=%d\n", m.SensorID)
	} else {
		fmt.Printf("himpp.isp.Disable error!HI_MPI_ISP_Exit=0x%X,isp sensor id=%d\n", uint(ret), m.SensorID)
	}

	// 反注册sns
	m.ipsSns.UnRegister()

	return int(ret)
}

// SetWDRMode set wdr mode
func (m *Isp) SetWDRMode(wdr *IspWdrMode) int {
	var cWdr ISPWDRMODES
	IspWdrModeCopyC(&cWdr, wdr)

	ret := C.HI_MPI_ISP_SetWDRMode(C.ISP_DEV(m.SensorID), (*C.ISP_WDR_MODE_S)(&cWdr))
	if 0 != ret {
		fmt.Printf("himpp.isp.SetWDRMode error!HI_MPI_ISP_SetWDRMode=0x%X,isp sensor id=%d\n", uint(ret), m.SensorID)
	}

	return int(ret)
}

// SetPubAttr set pub attr
func (m *Isp) SetPubAttr(pubAttr *IspPubAttr) int {
	var cAttr ISPPUBATTRS
	IspPubAttrCopyC(&cAttr, pubAttr)

	ret := C.HI_MPI_ISP_SetPubAttr(C.ISP_DEV(m.SensorID), (*C.ISP_PUB_ATTR_S)(&cAttr))
	if 0 != ret {
		fmt.Printf("himpp.isp.SetPubAttr error!HI_MPI_ISP_SetPubAttr=0x%X,isp sensor id=%d\n", uint(ret), m.SensorID)
	}

	return int(ret)
}
