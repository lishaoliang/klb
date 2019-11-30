///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb_ioctl.go
/// @author lishaoliang
/// @brief	fb ioctl
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "hifb.h"

#include <stdlib.h>

void klua_go_hifb_set_fb_bitfield(struct fb_bitfield* p_bitfield, int offset, int length, int msb_right)
{
	p_bitfield->offset = offset;
	p_bitfield->length = length;
	p_bitfield->msb_right = msb_right;
}

*/
import "C"
import (
	"fmt"
	"image"
	"syscall"
	"unsafe"

	"github.com/lishaoliang/klb/src/kimage/kcanvas"
	"github.com/lishaoliang/klb/src/kimage/kcolor"
)

// 设置fb颜色信息
func (m *Fb) setFbBitfield(varScInfo *C.struct_fb_var_screeninfo, colorFmt int) {

	switch colorFmt {
	case kcolor.ColorArgb1555:
		{
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.transp, 15, 1, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.red, 10, 5, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.green, 5, 5, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.blue, 0, 5, 0)

			varScInfo.bits_per_pixel = 16
		}
	case kcolor.ColorArgb8888:
		{
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.transp, 24, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.red, 16, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.green, 8, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.blue, 0, 8, 0)

			varScInfo.bits_per_pixel = 32
		}
	case kcolor.ColorArgb4444:
		{
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.transp, 12, 4, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.red, 8, 4, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.green, 4, 4, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.blue, 0, 4, 0)

			varScInfo.bits_per_pixel = 16
		}
	default:
		{
			// ARGB8888
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.transp, 24, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.red, 16, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.green, 8, 8, 0)
			C.klua_go_hifb_set_fb_bitfield(&varScInfo.blue, 0, 8, 0)

			varScInfo.bits_per_pixel = 32
		}
	}
}

// ioctlSetOrigin 设置fb屏幕原点坐标
func (m *Fb) ioctlSetOrigin(x, y int) {
	var pt C.HIFB_POINT_S
	pt.s32XPos = (C.int)(x)
	pt.s32YPos = (C.int)(y)

	_, _, err := syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIOPUT_SCREEN_ORIGIN_HIFB), uintptr(unsafe.Pointer(&pt)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlSetOrigin ioctl FBIOPUT_SCREEN_ORIGIN_HIFB error!err:[%d]\n", err)
	}
}

// ioctlSetVarScreen 设置fb屏幕信息
func (m *Fb) ioctlSetVarScreen(varScInfo *C.struct_fb_var_screeninfo, w, h, hMax, colorFmt int) int {
	_, _, err := syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIOGET_VSCREENINFO), uintptr(unsafe.Pointer(varScInfo)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlSetVarScreen ioctl FBIOGET_VSCREENINFO error!err:[%d]\n", err)
		return -1
	}

	m.setFbBitfield(varScInfo, colorFmt)

	varScInfo.activate = C.FB_ACTIVATE_NOW
	varScInfo.xres = C.uint(w)
	varScInfo.xres_virtual = C.uint(w)
	varScInfo.yres = C.uint(h)
	varScInfo.yres_virtual = C.uint(hMax)

	_, _, err = syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIOPUT_VSCREENINFO), uintptr(unsafe.Pointer(varScInfo)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlSetVarScreen ioctl FBIOPUT_VSCREENINFO error!err:[%d]\n", err)
		return -1
	}

	return 0
}

// ioctlGetFixScreen 获取fb屏幕的fix信息
func (m *Fb) ioctlGetFixScreen(fixScInfo *C.struct_fb_fix_screeninfo) {
	_, _, err := syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIOGET_FSCREENINFO), uintptr(unsafe.Pointer(fixScInfo)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlGetFixScreen ioctl FBIOGET_FSCREENINFO error!err:[%d]\n", err)
	}
}

// ioctlSetLayerBufNone 设置图层所使用0缓存方式
func (m *Fb) ioctlSetLayerBufNone() {
	var info C.HIFB_LAYER_INFO_S
	info.BufMode = C.HIFB_LAYER_BUF_NONE
	info.u32Mask = C.HIFB_LAYERMASK_BUFMODE

	_, _, err := syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIOPUT_LAYER_INFO), uintptr(unsafe.Pointer(&info)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlSetLayerBufNone ioctl FBIOPUT_LAYER_INFO error!err:[%d]\n", err)
	}
}

func (m *Fb) setHiFbBuffer(buf *C.HIFB_BUFFER_S, canvas *kcanvas.Canvas) {
	buf.stCanvas.u32PhyAddr = C.HI_U32(canvas.PhyAddr)
	buf.stCanvas.u32Width = C.uint(canvas.Rect.Dx())
	buf.stCanvas.u32Height = C.uint(canvas.Rect.Dy())
	buf.stCanvas.u32Pitch = C.uint(canvas.Stride)

	if kcolor.ColorArgb1555 == canvas.ColorFmt {
		buf.stCanvas.enFmt = C.HIFB_FMT_ARGB1555
	} else if kcolor.ColorArgb8888 == canvas.ColorFmt {
		buf.stCanvas.enFmt = C.HIFB_FMT_ARGB8888
	} else if kcolor.ColorArgb4444 == canvas.ColorFmt {
		buf.stCanvas.enFmt = C.HIFB_FMT_ARGB4444
	} else {
		buf.stCanvas.enFmt = C.HIFB_FMT_ARGB8888
	}

	//buf.UpdateRect.x = C.int(canvas.Rect.Min.X)
	//buf.UpdateRect.y = C.int(canvas.Rect.Min.Y)
	//buf.UpdateRect.w = C.int(canvas.Rect.Dx())
	//buf.UpdateRect.h = C.int(canvas.Rect.Dy())
}

// ioctlRefresh 刷新
func (m *Fb) ioctlRefresh(canvas *kcanvas.Canvas, r image.Rectangle) {
	var buf C.HIFB_BUFFER_S
	m.setHiFbBuffer(&buf, canvas)

	buf.UpdateRect.x = C.int(r.Min.X)
	buf.UpdateRect.y = C.int(r.Min.Y)
	buf.UpdateRect.w = C.int(r.Dx())
	buf.UpdateRect.h = C.int(r.Dy())

	_, _, err := syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIO_REFRESH), uintptr(unsafe.Pointer(&buf)))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlRefresh ioctl FBIO_REFRESH error!err:[%d]\n", err)
		return
	}

	_, _, err = syscall.Syscall(syscall.SYS_IOCTL, uintptr(m.fd), uintptr(C.FBIO_WAITFOR_FREFRESH_DONE), uintptr(0))
	if 0 != err {
		fmt.Printf("hifb.fb_ioctl.ioctlRefresh ioctl FBIO_WAITFOR_FREFRESH_DONE error!err:[%d]\n", err)
	}
}
