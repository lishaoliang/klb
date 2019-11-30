///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vo_chn.go
/// @author lishaoliang
/// @brief	vo channel
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import (
	"fmt"

	"github.com/lishaoliang/klb/src/kutil"
)

// VoChnAttr C.VO_CHN_ATTR_S
type VoChnAttr C.VO_CHN_ATTR_S

// VoChn hi vo chn
// vo通道
type VoChn struct {
	Layer *VoLayer

	Chn    int
	enable bool

	bind        bool     // 是否绑定
	bindVpssChn *VpssChn // 绑定到 VpssChn
}

// VoChnCreate create vo chn
// 创建
func VoChnCreate(layer *VoLayer, chn int, attr *VoChnAttr) *VoChn {
	var m VoChn

	m.Layer = layer
	m.Chn = chn
	m.enable = false

	m.SetChnAttr(attr)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VoChn) Destroy() {
	m.UnBind()
	m.Disable()
}

// SetChnAttr set vo chn attr
func (m *VoChn) SetChnAttr(attr *VoChnAttr) int {

	ret := C.HI_MPI_VO_SetChnAttr(C.VO_LAYER(m.Layer.Layer), C.VO_CHN(m.Chn), (*C.VO_CHN_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.vo_chn.SetChnAttr error!HI_MPI_VO_SetChnAttr=0x%X,vo layer=%d,vo chn=%d\n", uint(ret), m.Layer.Layer, m.Chn)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *VoChn) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_EnableChn(C.VO_LAYER(m.Layer.Layer), C.VO_CHN(m.Chn))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vo_chn.Enable ok!HI_MPI_VO_EnableChn vo layer=%d,vo chn=%d\n", m.Layer.Layer, m.Chn)
	} else {
		fmt.Printf("himpp.vo_chn.Enable error!HI_MPI_VO_EnableChn=0x%X,vo layer=%d,vo chn=%d\n", uint(ret), m.Layer.Layer, m.Chn)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *VoChn) Disable() int {
	if !m.enable {
		return 0
	}

	ret := C.HI_MPI_VO_DisableChn(C.VO_LAYER(m.Layer.Layer), C.VO_CHN(m.Chn))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vo_chn.Disable ok!HI_MPI_VO_DisableChn vo layer=%d,vo chn=%d\n", m.Layer.Layer, m.Chn)
	} else {
		fmt.Printf("himpp.vo_chn.Disable error!HI_MPI_VO_DisableChn=0x%X,vo layer=%d,vo chn=%d\n", uint(ret), m.Layer.Layer, m.Chn)
	}

	return int(ret)
}

// UnBind unbind
// 解绑
func (m *VoChn) UnBind() int {
	if !m.bind {
		return 0
	}

	if nil != m.bindVpssChn {
		return m.unBindVpssChn()
	}

	kutil.Assert(false)
	return -1
}

// BindVpssChn bind vpss chn
// 绑定到 vpss chn
func (m *VoChn) BindVpssChn(vpssChn *VpssChn) int {
	if !m.enable || m.bind {
		fmt.Printf("himpp.vo_chn.BindVpssChn error!not enable or repeat,vo layer=%d,vo chn=%d\n", m.Layer.Layer, m.Chn)
		return -1
	}

	var src, dst MppChn

	src.ModID = IDVPSS // HI_ID_VPSS
	src.DevID = vpssChn.Grp.Grp
	src.ChnID = vpssChn.Chn

	dst.ModID = IDVOU // HI_ID_VOU
	dst.DevID = m.Layer.Layer
	dst.ChnID = m.Chn

	ret := SysBind(&src, &dst)
	if 0 == ret {
		m.bind = true
		m.bindVpssChn = vpssChn

		fmt.Printf("himpp.vo_chn.BindVpssChn ok!HI_MPI_SYS_Bind vpss=[%d,%d],vo layer=%d,vo chn=%d\n", vpssChn.Grp.Grp, vpssChn.Chn, m.Layer.Layer, m.Chn)
	} else {
		fmt.Printf("himpp.vo_chn.BindVpssChn error!HI_MPI_SYS_Bind=0x%X,vpss=[%d,%d],vo layer=%d,vo chn=%d\n", uint(ret), vpssChn.Grp.Grp, vpssChn.Chn, m.Layer.Layer, m.Chn)
	}

	return ret
}

///////////////////////////////////////////////////////////////////////////////

// unBindVpssChn unbind vpss chn
// 解绑
func (m *VoChn) unBindVpssChn() int {
	kutil.Assert(m.bind)
	kutil.Assert(nil != m.bindVpssChn)

	var src, dst MppChn

	src.ModID = IDVPSS // HI_ID_VPSS
	src.DevID = m.bindVpssChn.Grp.Grp
	src.ChnID = m.bindVpssChn.Chn

	dst.ModID = IDVOU // HI_ID_VOU
	dst.DevID = m.Layer.Layer
	dst.ChnID = m.Chn

	ret := SysUnBind(&src, &dst)

	if 0 == ret {
		fmt.Printf("himpp.vo_chn.unBindVpssChn ok!HI_MPI_SYS_UnBind vpss=[%d,%d],vo layer=%d,vo chn=%d\n", m.bindVpssChn.Grp.Grp, m.bindVpssChn.Chn, m.Layer.Layer, m.Chn)

		m.bind = false
		m.bindVpssChn = nil
	} else {
		fmt.Printf("himpp.vo_chn.unBindVpssChn error!HI_MPI_SYS_UnBind=0x%X,vpss=[%d,%d],vo layer=%d,vo chn=%d\n", uint(ret), m.bindVpssChn.Grp.Grp, m.bindVpssChn.Chn, m.Layer.Layer, m.Chn)
	}

	return ret
}
