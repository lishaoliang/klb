///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   venc_chn.go
/// @author lishaoliang
/// @brief	venc channel
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import (
	"fmt"
	"unsafe"

	"github.com/lishaoliang/klb/src/kutil"
)

// VencChnAttr venc channel attr
type VencChnAttr C.VENC_CHN_ATTR_S

// VENC_PACK_S结构体缓存缓存最大个数
const constVencPacksMax = 64

// VencChn hi venc channel
// venc channel
type VencChn struct {
	Chn int     // Venc 通道号
	FD  uintptr // fd

	enable   bool // 使能
	bRecvPic bool // 是否开始接收数据

	bind        bool     // 是否绑定
	bindVpssChn *VpssChn // 绑定到 VpssChn

	bufPacks *C.VENC_PACK_S // VENC_PACK_S结构体缓存
}

// VencChnCreate create venc chn
// 创建
func VencChnCreate(chn int, attr *VencChnAttr) *VencChn {
	var m VencChn

	m.bufPacks = (*C.VENC_PACK_S)(kutil.Malloc(int(C.sizeof_VENC_PACK_S) * constVencPacksMax))

	m.Chn = chn
	m.enable = false
	m.bRecvPic = false
	m.bind = false

	ret := C.HI_MPI_VENC_CreateChn(C.VENC_CHN(m.Chn), (*C.VENC_CHN_ATTR_S)(attr))
	if 0 == ret {
		m.FD = m.GetFd()
		fmt.Printf("himpp.venc_chn.VencChnCreate ok!HI_MPI_VENC_CreateChn venc chn=%d,fd=%d\n", m.Chn, m.FD)
	} else {
		fmt.Printf("himpp.venc_chn.VencChnCreate error!HI_MPI_VENC_CreateChn=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *VencChn) Destroy() {
	m.Disable()

	ret := C.HI_MPI_VENC_DestroyChn(C.VENC_CHN(m.Chn))
	if 0 == ret {
		fmt.Printf("himpp.venc_chn.Destroy ok!HI_MPI_VENC_DestroyChn venc chn=%d\n", m.Chn)
	} else {
		fmt.Printf("himpp.venc_chn.Destroy error!HI_MPI_VENC_DestroyChn=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}

	m.FD = 0

	kutil.Free(unsafe.Pointer(m.bufPacks))
	m.bufPacks = nil

	kutil.Assert(false == m.bRecvPic)
	kutil.Assert(nil == m.bindVpssChn)
	kutil.Assert(false == m.bind)
	kutil.Assert(false == m.enable)
}

// SetChnAttr HI_MPI_VENC_SetChnAttr
// 设置属性
func (m *VencChn) SetChnAttr(attr *VencChnAttr) int {
	ret := C.HI_MPI_VENC_SetChnAttr(C.VENC_CHN(m.Chn), (*C.VENC_CHN_ATTR_S)(attr))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.SetChnAttr error!HI_MPI_VENC_SetChnAttr=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}

	return int(ret)
}

// Enable enable
// 使能
func (m *VencChn) Enable() int {
	if m.enable {
		return 0
	}

	m.enable = true
	return 0
}

// Disable disable
// 不使能
func (m *VencChn) Disable() int {
	if !m.enable {
		return 0
	}

	m.StopRecvPic()
	m.UnBind()

	m.enable = false
	return 0
}

// UnBind unbind
// 解绑
func (m *VencChn) UnBind() int {
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
func (m *VencChn) BindVpssChn(vpssChn *VpssChn) int {
	if !m.enable || m.bind {
		fmt.Printf("himpp.venc_chn.BindVpssChn error!not enable or repeat,venc chn=%d\n", m.Chn)
		return -1
	}

	var src, dst MppChn

	src.ModID = IDVPSS // HI_ID_VPSS
	src.DevID = vpssChn.Grp.Grp
	src.ChnID = vpssChn.Chn

	dst.ModID = IDVENC // HI_ID_VENC
	dst.DevID = 0
	dst.ChnID = m.Chn

	ret := SysBind(&src, &dst)
	if 0 == ret {
		m.bind = true
		m.bindVpssChn = vpssChn

		fmt.Printf("himpp.venc_chn.BindVpssChn ok!HI_MPI_SYS_Bind vpss=[%d,%d],venc chn=%d\n", vpssChn.Grp.Grp, vpssChn.Chn, m.Chn)
	} else {
		fmt.Printf("himpp.venc_chn.BindVpssChn error!HI_MPI_SYS_Bind=0x%X,vpss=[%d,%d],venc chn=%d\n", uint(ret), vpssChn.Grp.Grp, vpssChn.Chn, m.Chn)
	}

	return ret
}

// StartRecvPic start recv pic
func (m *VencChn) StartRecvPic() int {
	if !m.enable {
		fmt.Printf("himpp.venc_chn.StartRecvPic error!not enable,venc chn=%d\n", m.Chn)
		return -1
	}

	if m.bRecvPic {
		return 0 // 已经打开
	}

	ret := C.HI_MPI_VENC_StartRecvPic(C.VENC_CHN(m.Chn))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.StartRecvPic error!HI_MPI_VENC_StartRecvPic=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	} else {
		m.bRecvPic = true
	}

	return int(ret)
}

// StopRecvPic stop recv pic
func (m *VencChn) StopRecvPic() int {
	if !m.bRecvPic {
		return 0
	}

	ret := C.HI_MPI_VENC_StopRecvPic(C.VENC_CHN(m.Chn))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.StopRecvPic error!HI_MPI_VENC_StopRecvPic=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	} else {
		m.bRecvPic = false
	}

	return int(ret)
}

// ResetChn reset
func (m *VencChn) ResetChn() int {
	ret := C.HI_MPI_VENC_ResetChn(C.VENC_CHN(m.Chn))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.ResetChn error!HI_MPI_VENC_ResetChn=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}
	return int(ret)
}

// RequestIDR request IDR
func (m *VencChn) RequestIDR(bInstant bool) int {
	bflag := C.HI_FALSE
	if bInstant {
		bflag = C.HI_TRUE
	}

	ret := C.HI_MPI_VENC_RequestIDR(C.VENC_CHN(m.Chn), C.HI_BOOL(bflag))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.RequestIDR error!HI_MPI_VENC_RequestIDR=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}
	return int(ret)
}

// GetFd get fd
func (m *VencChn) GetFd() uintptr {
	fd := C.HI_MPI_VENC_GetFd(C.VENC_CHN(m.Chn))
	return uintptr(fd)
}

// Query query
// 查询venc状态
func (m *VencChn) Query() (*VencChnStat, error) {
	var cStat VENCCHNSTATS
	ret := C.HI_MPI_VENC_Query(C.VENC_CHN(m.Chn), (*C.VENC_CHN_STAT_S)(&cStat))
	if 0 == ret {
		var stat VencChnStat
		VencChnStatCopy(&stat, &cStat)
		return &stat, nil
	}

	return nil, fmt.Errorf("himpp.venc_chn.Query error!HI_MPI_VENC_Query=0x%X,venc chn=%d", uint(ret), m.Chn)
}

// VENCSTREAMS C.VENC_STREAM_S
// venc stream格式
type VENCSTREAMS C.VENC_STREAM_S

// GetStream get stream
// 获取码流
func (m *VencChn) GetStream(outStream *VENCSTREAMS, waitMilliSec int) int {
	ret := C.HI_MPI_VENC_GetStream(C.VENC_CHN(m.Chn), (*C.VENC_STREAM_S)(outStream), (C.HI_S32)(waitMilliSec))
	//if 0 != ret {
	//	fmt.Printf("himpp.venc_chn.GetStream error!HI_MPI_VENC_GetStream=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	//}
	return int(ret)
}

// RecvStream recv stream
// 获取码流
func (m *VencChn) RecvStream(outStream *VENCSTREAMS, waitMilliSec int) (int, *VencChnStat) {
	stat, _ := m.Query()
	if nil != stat && 0 < stat.CurPacks {
		stream := (*C.VENC_STREAM_S)(outStream)
		stream.pstPack = m.bufPacks
		stream.u32PackCount = (C.HI_U32)(kutil.Min(int(stat.CurPacks), constVencPacksMax))

		return m.GetStream(outStream, waitMilliSec), stat
	}

	return -1, stat // 无数据
}

// ReleaseStream release stream
// 释放码流
func (m *VencChn) ReleaseStream(stream *VENCSTREAMS) int {
	ret := C.HI_MPI_VENC_ReleaseStream(C.VENC_CHN(m.Chn), (*C.VENC_STREAM_S)(stream))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.ReleaseStream error!HI_MPI_VENC_ReleaseStream=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}
	return int(ret)
}

// SendFrame send frame
// 发送码流
func (m *VencChn) SendFrame(frame *VIDEOFRAMEINFOS, waitMilliSec int) int {
	ret := C.HI_MPI_VENC_SendFrame(C.VENC_CHN(m.Chn), (*C.VIDEO_FRAME_INFO_S)(frame), (C.HI_S32)(waitMilliSec))
	if 0 != ret {
		fmt.Printf("himpp.venc_chn.SendFrame error!HI_MPI_VENC_SendFrame=0x%X,venc chn=%d\n", uint(ret), m.Chn)
	}
	return int(ret)
}

///////////////////////////////////////////////////////////////////////////////

// unBindVpssChn unbind vpss chn
// 解绑
func (m *VencChn) unBindVpssChn() int {
	kutil.Assert(m.bind)
	kutil.Assert(nil != m.bindVpssChn)

	var src, dst MppChn

	src.ModID = IDVPSS // HI_ID_VPSS
	src.DevID = m.bindVpssChn.Grp.Grp
	src.ChnID = m.bindVpssChn.Chn

	dst.ModID = IDVENC // HI_ID_VENC
	dst.DevID = 0
	dst.ChnID = m.Chn

	ret := SysUnBind(&src, &dst)

	if 0 == ret {
		fmt.Printf("himpp.venc_chn.unBindVpssChn ok!HI_MPI_SYS_UnBind vpss=[%d,%d],venc chn=%d\n", m.bindVpssChn.Grp.Grp, m.bindVpssChn.Chn, m.Chn)

		m.bind = false
		m.bindVpssChn = nil
	} else {
		fmt.Printf("himpp.venc_chn.unBindVpssChn error!HI_MPI_SYS_UnBind=0x%X,vpss=[%d,%d],venc chn=%d\n", uint(ret), m.bindVpssChn.Grp.Grp, m.bindVpssChn.Chn, m.Chn)
	}

	return ret
}
