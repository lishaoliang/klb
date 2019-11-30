///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vpss_grp.go
/// @author lishaoliang
/// @brief	vpss group
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

// VpssGrpAttr vpss grp attr
type VpssGrpAttr C.VPSS_GRP_ATTR_S

// VpssGrp hi vpss grp
// vpss grp
type VpssGrp struct {
	Grp int // vpss组编号

	enable bool // 是否使能
	bind   bool // 是否绑定

	bindViChn *ViChn // 绑定到vi chn
}

// VpssGrpCreate create vpss grp
// 创建
func VpssGrpCreate(grp int, attr *VpssGrpAttr) *VpssGrp {
	var m VpssGrp

	m.Grp = grp
	m.enable = false

	ret := C.HI_MPI_VPSS_CreateGrp(C.VPSS_GRP(m.Grp), (*C.VPSS_GRP_ATTR_S)(attr))
	if 0 == ret {
		fmt.Printf("himpp.vpss_grp.VpssGrpCreate ok!HI_MPI_VPSS_CreateGrp vpss grp=%d\n", m.Grp)
	} else {
		fmt.Printf("himpp.vpss_grp.VpssGrpCreate error!HI_MPI_VPSS_CreateGrp=0x%X,vpss grp=%d\n", uint(ret), m.Grp)
	}

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VpssGrp) Destroy() {
	m.Disable()

	ret := C.HI_MPI_VPSS_DestroyGrp(C.VPSS_GRP(m.Grp))
	if 0 == ret {
		fmt.Printf("himpp.vpss_grp.Destroy ok!HI_MPI_VPSS_DestroyGrp vpss grp=%d\n", m.Grp)
	} else {
		fmt.Printf("himpp.vpss_grp.Destroy error!HI_MPI_VPSS_DestroyGrp=0x%X,vpss grp=%d\n", uint(ret), m.Grp)
	}

	kutil.Assert(nil == m.bindViChn)
	kutil.Assert(false == m.bind)
	kutil.Assert(false == m.enable)
}

// Enable enable
// 使能
func (m *VpssGrp) Enable() int {
	if m.enable {
		return 0
	}

	ret := C.HI_MPI_VPSS_StartGrp(C.VPSS_GRP(m.Grp))
	if 0 == ret {
		m.enable = true
		fmt.Printf("himpp.vpss_grp.Enable ok!HI_MPI_VPSS_StartGrp vpss grp=%d\n", m.Grp)
	} else {
		fmt.Printf("himpp.vpss_grp.Enable error!HI_MPI_VPSS_StartGrp=0x%X,vpss grp=%d\n", uint(ret), m.Grp)
	}

	return int(ret)
}

// Disable disable
// 不使能
func (m *VpssGrp) Disable() int {
	if !m.enable {
		return 0
	}

	// 解绑
	m.UnBind()

	// 关闭
	ret := int(C.HI_MPI_VPSS_StopGrp(C.VPSS_GRP(m.Grp)))
	if 0 == ret {
		m.enable = false
		fmt.Printf("himpp.vpss_grp.Disable ok!HI_MPI_VPSS_StopGrp vpss grp=%d\n", m.Grp)
	} else {
		fmt.Printf("himpp.vpss_grp.Disable error!HI_MPI_VPSS_StopGrp=0x%X,vpss grp=%d\n", uint(ret), m.Grp)
	}

	return int(ret)
}

// UnBind unbind
// 解绑
func (m *VpssGrp) UnBind() int {
	if !m.bind {
		return 0
	}

	if nil != m.bindViChn {
		return m.unBindViChn()
	}

	kutil.Assert(false)
	return -1
}

// BindViChn vpss grp bind vi chn
// 绑定到VI
func (m *VpssGrp) BindViChn(viChn *ViChn) int {
	if !m.enable || m.bind {
		fmt.Printf("himpp.vpss_grp.BindViChn error!not enable or repeat,vpss grp=%d\n", m.Grp)
		return -1
	}

	var src, dst MppChn

	src.ModID = IDVIU // HI_ID_VIU
	src.DevID = viChn.Dev.DevID
	src.ChnID = viChn.Chn

	dst.ModID = IDVPSS // HI_ID_VPSS
	dst.DevID = m.Grp
	dst.ChnID = 0

	ret := SysBind(&src, &dst)
	if 0 == ret {
		m.bind = true
		m.bindViChn = viChn

		fmt.Printf("himpp.vpss_grp.BindViChn ok!HI_MPI_SYS_Bind vi=[%d,%d],vpss grp=%d\n", viChn.Dev.DevID, viChn.Chn, m.Grp)
	} else {
		fmt.Printf("himpp.vpss_grp.BindViChn error!HI_MPI_SYS_Bind=0x%X,vi=[%d,%d],vpss grp=%d\n", uint(ret), viChn.Dev.DevID, viChn.Chn, m.Grp)
	}

	return ret
}

///////////////////////////////////////////////////////////////////////////////

// unBindViChn unbind vi
// 解绑vi
func (m *VpssGrp) unBindViChn() int {
	kutil.Assert(m.bind)
	kutil.Assert(nil != m.bindViChn)

	var src, dst MppChn

	src.ModID = IDVIU // HI_ID_VIU
	src.DevID = m.bindViChn.Dev.DevID
	src.ChnID = m.bindViChn.Chn

	dst.ModID = IDVPSS // HI_ID_VPSS
	dst.DevID = m.Grp
	dst.ChnID = 0

	ret := SysUnBind(&src, &dst)

	if 0 == ret {
		fmt.Printf("himpp.vpss_grp.unbindVi ok!HI_MPI_SYS_UnBind vi=[%d,%d],vpss grp=%d\n", m.bindViChn.Dev.DevID, m.bindViChn.Chn, m.Grp)

		m.bind = false
		m.bindViChn = nil
	} else {
		fmt.Printf("himpp.vpss_grp.unbindVi error!HI_MPI_SYS_UnBind=0x%X,vi=[%d,%d],vpss grp=%d\n", uint(ret), m.bindViChn.Dev.DevID, m.bindViChn.Chn, m.Grp)
	}

	return ret
}
