///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   tde.go
/// @author lishaoliang
/// @brief	tde
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "klb_type.h"
#include "himpp/klb_himpp.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"

*/
import "C"

import (
	"fmt"
	"image"
	"sync"

	"github.com/lishaoliang/klb/src/kimage/kcanvas"
	"github.com/lishaoliang/klb/src/kutil"
)

// Tde hi tde
type Tde struct {
	handle C.TDE_HANDLE
}

// tde 是否需要初始化
var gTdeOpenCount = 0
var gTdeOpenMutex sync.Mutex

// TdeOpen open tde,使用前必须打开tde, 打开一次
func TdeOpen() {
	gTdeOpenMutex.Lock()
	defer gTdeOpenMutex.Unlock()

	if 0 < gTdeOpenCount {
		gTdeOpenCount++
		return
	}

	kutil.Assert(0 == gTdeOpenCount)

	ret := C.HI_TDE2_Open()
	if 0 != ret {
		fmt.Printf("hifb.tde.TdeOpen error!HI_TDE2_Open=0x%X\n", uint(ret))
	} else {
		gTdeOpenCount++
		fmt.Println("hifb.tde.TdeOpen ok!HI_TDE2_Open")
	}
}

// TdeClose close tde, 关闭一次
func TdeClose() {
	gTdeOpenMutex.Lock()
	defer gTdeOpenMutex.Unlock()

	if 0 < gTdeOpenCount {
		gTdeOpenCount--

		if gTdeOpenCount <= 0 {
			C.HI_TDE2_Close()
			fmt.Println("hifb.tde.TdeClose ok!HI_TDE2_Close")
			gTdeOpenCount = 0
		}
	} else {
		kutil.Assert(false)
	}
}

// NewTde new tde
func NewTde() *Tde {
	var m Tde
	m.handle = -1

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// BeginJob 开启任务
func (m *Tde) BeginJob() {
	ret := C.HI_TDE2_BeginJob()
	if C.HI_ERR_TDE_INVALID_HANDLE == ret || C.HI_ERR_TDE_DEV_NOT_OPEN == ret {
		// 打开失败
		fmt.Printf("hifb.tde.BeginJob error!HI_TDE2_BeginJob=0x%X\n", uint(ret))
		return
	}

	m.handle = ret
}

// EndJob 结束任务
func (m *Tde) EndJob() {
	if m.handle < 0 {
		fmt.Println("hifb.tde.EndJob error!must BeginJob")
		return
	}

	ret := C.HI_TDE2_EndJob(m.handle, C.HI_FALSE, C.HI_FALSE, 0)
	if 0 != ret {
		fmt.Printf("hifb.tde.EndJob error!HI_TDE2_EndJob=0x%X\n", uint(ret))
		ret = C.HI_TDE2_CancelJob(m.handle)
		if 0 != ret {
			fmt.Printf("hifb.tde.EndJob error!HI_TDE2_CancelJob1=0x%X\n", uint(ret))
		}

		m.handle = -1
		return
	}

	ret = C.HI_TDE2_WaitForDone(m.handle)
	if 0 != ret {
		fmt.Printf("hifb.tde.EndJob error!HI_TDE2_WaitForDone=0x%X\n", uint(ret))
		ret = C.HI_TDE2_CancelJob(m.handle)
		if 0 != ret {
			fmt.Printf("hifb.tde.EndJob error!HI_TDE2_CancelJob2=0x%X\n", uint(ret))
		}
	}

	m.handle = -1
}

// setSurface
func (m *Tde) setSurfaceValue(dst *C.TDE2_SURFACE_S, src *kcanvas.Canvas) {
	dst.u32PhyAddr = C.HI_U32(src.PhyAddr)
	dst.u32Width = C.HI_U32(src.Rect.Dx())
	dst.u32Height = C.HI_U32(src.Rect.Dy())
	dst.u32Stride = C.HI_U32(src.Stride)

	dst.enColorFmt = (C.TDE2_COLOR_FMT_E)(m.tdeColorFmt(src.ColorFmt))
}

// rect
func (m *Tde) setRectValue(rect *C.TDE2_RECT_S, r image.Rectangle) {
	rect.s32Xpos = C.HI_S32(r.Min.X)
	rect.s32Ypos = C.HI_S32(r.Min.Y)
	rect.u32Width = C.HI_U32(r.Dx())
	rect.u32Height = C.HI_U32(r.Dy())
}

func (m *Tde) setRectValueInt(rect *C.TDE2_RECT_S, x, y, w, h int) {
	rect.s32Xpos = C.HI_S32(x)
	rect.s32Ypos = C.HI_S32(y)
	rect.u32Width = C.HI_U32(w)
	rect.u32Height = C.HI_U32(h)
}

// Fill 区域填充
func (m *Tde) Fill(dst *kcanvas.Canvas, r image.Rectangle, color uint32) {
	var rect C.TDE2_RECT_S
	m.setRectValue(&rect, r)

	var surface C.TDE2_SURFACE_S
	m.setSurfaceValue(&surface, dst)

	c := C.uint32_t(color)

	ret := C.HI_TDE2_QuickFill(m.handle, &surface, &rect, c)
	if 0 != ret {
		fmt.Printf("hifb.tde.Fill error!HI_TDE2_QuickFill=0x%X\n", uint(ret))
	}
}

// Copy 区域拷贝
// 不支持格式转换, 源位图和目标位图格式必须一致
func (m *Tde) Copy(dst *kcanvas.Canvas, dstR image.Rectangle, src *kcanvas.Canvas, srcPt image.Point) {
	var dstRect C.TDE2_RECT_S
	m.setRectValue(&dstRect, dstR)

	var dstSurface C.TDE2_SURFACE_S
	m.setSurfaceValue(&dstSurface, dst)

	var srcRect C.TDE2_RECT_S
	m.setRectValueInt(&srcRect, srcPt.X, srcPt.Y, dstR.Dx(), dstR.Dy())

	var srcSurface C.TDE2_SURFACE_S
	m.setSurfaceValue(&srcSurface, src)

	ret := C.HI_TDE2_QuickCopy(m.handle, &srcSurface, &srcRect, &dstSurface, &dstRect)
	if 0 != ret {
		fmt.Printf("hifb.tde.Copy error!HI_TDE2_QuickCopy=0x%X\n", uint(ret))
	}
}
