///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vo_layer.go
/// @author lishaoliang
/// @brief	vo layer
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import "fmt"

// VoLayerAttr C.VO_VIDEO_LAYER_ATTR_S
type VoLayerAttr C.VO_VIDEO_LAYER_ATTR_S

// VoLayer hi vo layer
// vo layer
type VoLayer struct {
	Layer  int
	enable bool
}

// VoLayerCreate create vo layer
// 创建
// bufLen : 缓冲长度, 取值范围: [0] 直通; [3,15] 非直通, 使用VGS模块;
func VoLayerCreate(layer int, attr *VoLayerAttr, bufLen int) *VoLayer {
	var m VoLayer

	m.Layer = layer
	m.enable = false

	m.SetDispBufLen(bufLen)

	m.SetLayerAttr(attr)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VoLayer) Destroy() {
	m.Disable()
}

// SetLayerAttr set vo layer attr
func (m *VoLayer) SetLayerAttr(attr *VoLayerAttr) int {
	ret := C.HI_MPI_VO_SetVideoLayerAttr(C.VO_LAYER(m.Layer), (*C.VO_VIDEO_LAYER_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vo_layer.SetLayerAttr error!HI_MPI_VO_SetVideoLayerAttr=0x%X,vo layer=%d\n", uint(ret), m.Layer)
	}

	return int(ret)
}

// SetDispBufLen 设置 vo layer 缓冲长度
// bufLen : 取值范围: [0] 直通; [3,15] 非直通, 使用VGS模块;
func (m *VoLayer) SetDispBufLen(bufLen int) int {
	ret := C.HI_MPI_VO_SetDispBufLen(C.VO_LAYER(m.Layer), (C.HI_U32)(bufLen))
	if 0 != ret {
		fmt.Printf("himpp.vo_layer.SetDispBufLen error!HI_MPI_VO_SetDispBufLen=0x%X,vo layer=%d,bufLen=%d\n", uint(ret), m.Layer, bufLen)
	} else {
		fmt.Printf("himpp.vo_layer.SetDispBufLen ok!vo layer=%d,bufLen=%d\n", m.Layer, bufLen)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *VoLayer) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_EnableVideoLayer(C.VO_LAYER(m.Layer))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vo_layer.Enable ok!HI_MPI_VO_EnableVideoLayer vo layer=%d\n", m.Layer)
	} else {
		fmt.Printf("himpp.vo_layer.Enable error!HI_MPI_VO_EnableVideoLayer=0x%X,vo layer=%d\n", uint(ret), m.Layer)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *VoLayer) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_DisableVideoLayer(C.VO_LAYER(m.Layer))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vo_layer.Disable ok!HI_MPI_VO_DisableVideoLayer vo layer=%d\n", m.Layer)
	} else {
		fmt.Printf("himpp.vo_layer.Disable error!HI_MPI_VO_DisableVideoLayer=0x%X,vo layer=%d\n", uint(ret), m.Layer)
	}

	return int(ret)
}
