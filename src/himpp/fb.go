///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb.go
/// @author lishaoliang
/// @brief	fb
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include <stdlib.h>

int klua_go_hifb_open_fb(const char* p_path)
{
	return open(p_path, O_RDWR, 0);
}

int klua_go_hifb_close_fb(int fd)
{
	return close(fd);
}

*/
import "C"

import (
	"fmt"
	"image"
	"unsafe"

	"github.com/lishaoliang/klb/src/kimage/kcanvas"
	"github.com/lishaoliang/klb/src/kimage/kcolor"
	"github.com/lishaoliang/klb/src/kutil"
)

// fb path
const (
	Fb0Path = "/dev/fb0"
	Fb1Path = "/dev/fb1"
	Fb2Path = "/dev/fb2"
	Fb3Path = "/dev/fb3"
	Fb4Path = "/dev/fb4"
	Fb5Path = "/dev/fb5"
)

// Fb hi linux fb
type Fb struct {
	Path string // fb 路径; "/dev/fb0"

	canvasBase kcanvas.Canvas // 全画布
	canvas0    kcanvas.Canvas // 画布0
	canvas1    kcanvas.Canvas // 画布1

	fd int // 文件句柄
}

// FbCreate create fb
// path : "/dev/fb0"
// w, h : 屏幕宽高
// colorFmt : 颜色格式: [kcolor.ColorArgb8888, kcolor.ColorArgb1555]
func FbCreate(path string, w, h, colorFmt int) *Fb {
	kutil.Assert(kcolor.ColorArgb8888 == colorFmt ||
		kcolor.ColorArgb1555 == colorFmt)

	var m Fb

	m.Path = path

	cStr := C.CString(path)
	defer C.free(unsafe.Pointer(cStr))

	// open "/dev/fb0"
	// go 的 open 属于保留字段, 使用自定义c函数替换
	fd := C.klua_go_hifb_open_fb(cStr)
	if fd < 0 {
		fmt.Printf("hifb.FbCreate open fb error!path:[%s]\n", path)
	} else {
		fmt.Printf("hifb.FbCreate open fb ok!path:[%s],fd:[%d]\n", path, fd)
	}

	m.fd = int(fd)

	// 设置原点
	m.ioctlSetOrigin(0, 0)

	// 屏幕信息
	var varScInfo C.struct_fb_var_screeninfo
	var fixScInfo C.struct_fb_fix_screeninfo

	// 设置屏幕信息
	m.ioctlSetVarScreen(&varScInfo, w, h, h*2, colorFmt)

	// 获取屏幕固定信息
	m.ioctlGetFixScreen(&fixScInfo)

	// 设置显示区域默认偏移
	varScInfo.xoffset = 0
	varScInfo.yoffset = 0

	// 设置0缓存
	m.ioctlSetLayerBufNone()

	// 映射虚拟地址
	addr := C.mmap(nil, fixScInfo.smem_len, C.PROT_READ|C.PROT_WRITE, C.MAP_SHARED, C.int(m.fd), 0)

	m.canvasBase.Rect = image.Rect(0, 0, w, h*2)
	m.canvasBase.Addr = uintptr(addr)
	m.canvasBase.PhyAddr = uintptr(fixScInfo.smem_start)
	m.canvasBase.MemLen = int(fixScInfo.smem_len)
	m.canvasBase.Stride = int(fixScInfo.line_length)
	m.canvasBase.ColorFmt = colorFmt

	memlen := m.canvasBase.Stride * h

	m.canvas0 = m.canvasBase
	m.canvas0.Rect = image.Rect(0, 0, w, h)
	m.canvas0.MemLen = memlen

	m.canvas1 = m.canvasBase
	m.canvas0.Rect = image.Rect(0, 0, w, h)
	m.canvas1.Addr = m.canvasBase.Addr + uintptr(memlen)
	m.canvas1.PhyAddr = m.canvasBase.PhyAddr + uintptr(memlen)
	m.canvas1.MemLen = memlen

	// 打印fb信息
	fmt.Printf("fb info ---->>>> path:[%s],fd:[%d]\n", m.Path, m.fd)
	fmt.Printf("fb WH:[%d,%d],Stride:[%d]\n", m.canvasBase.Rect.Dx(), m.canvasBase.Rect.Dy(), m.canvasBase.Stride)
	fmt.Printf("fb Addr:[0x%X],PhyAddr:[0x%X],MemLen:[%dK]\n", m.canvasBase.Addr, m.canvasBase.PhyAddr, m.canvasBase.MemLen/1024)
	fmt.Printf("fb info <<<<----\n")

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *Fb) Destroy() {
	if 0 <= m.fd {

		// 解除虚拟地址映射
		C.munmap(unsafe.Pointer(m.canvasBase.Addr), C.size_t(m.canvasBase.MemLen))

		fmt.Printf("hifb.Destroy close fb!path:[%s],fd:[%d]\n", m.Path, m.fd)
		C.klua_go_hifb_close_fb(C.int(m.fd))
		m.fd = -1
	}
}

// GetCanvas0 获取画布0
func (m *Fb) GetCanvas0() kcanvas.Canvas {
	return m.canvas0
}

// GetCanvas1 获取画布1
func (m *Fb) GetCanvas1() kcanvas.Canvas {
	return m.canvas1
}

// Refresh 刷新
func (m *Fb) Refresh(r image.Rectangle) {
	m.ioctlRefresh(&m.canvas0, r)
}
