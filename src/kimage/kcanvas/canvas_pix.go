///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   canvas_pix.go
/// @author lishaoliang
/// @brief	canvas pix
///////////////////////////////////////////////////////////////////////////

package kcanvas

/*
#include <stdint.h>     /// int*_t uint*_t
#include <stddef.h>     /// intptr_t uintptr_t size_t

uint32_t kimage_kcanvas_canvas_pix_go_get_uint32(uint8_t* p_addr, int offset)
{
	uint32_t* ptr = (uint32_t*)(p_addr + offset);
	return *ptr;
}

void kimage_kcanvas_canvas_pix_go_set_uint32(uint8_t* p_addr, int offset, uint32_t v)
{
	uint32_t* ptr = (uint32_t*)(p_addr + offset);
	*ptr = v;
}

uint16_t kimage_kcanvas_canvas_pix_go_get_uint16(uint8_t* p_addr, int offset)
{
	uint16_t* ptr = (uint16_t*)(p_addr + offset);
	return *ptr;
}

void kimage_kcanvas_canvas_pix_go_set_uint16(uint8_t* p_addr, int offset, uint16_t v)
{
	uint16_t* ptr = (uint16_t*)(p_addr + offset);
	*ptr = v;
}
*/
import "C"

import (
	"image/color"
	"unsafe"

	"github.com/lishaoliang/klb/src/kimage/kcolor"
)

func (m *Canvas) getPixUint32(offset int) uint32 {
	return uint32(C.kimage_kcanvas_canvas_pix_go_get_uint32((*C.uint8_t)(unsafe.Pointer(m.Addr)), (C.int)(offset)))
}

func (m *Canvas) setPixUint32(offset int, v uint32) {
	C.kimage_kcanvas_canvas_pix_go_set_uint32((*C.uint8_t)(unsafe.Pointer(m.Addr)), (C.int)(offset), (C.uint32_t)(v))
}

func (m *Canvas) getPixUint16(offset int) uint16 {
	return uint16(C.kimage_kcanvas_canvas_pix_go_get_uint16((*C.uint8_t)(unsafe.Pointer(m.Addr)), (C.int)(offset)))
}

func (m *Canvas) setPixUint16(offset int, v uint16) {
	C.kimage_kcanvas_canvas_pix_go_set_uint16((*C.uint8_t)(unsafe.Pointer(m.Addr)), (C.int)(offset), (C.uint16_t)(v))
}

func (m *Canvas) getPixRgba(offset int) (r, g, b, a uint32) {
	switch m.ColorFmt {
	case kcolor.ColorArgb1555:
		v := m.getPixUint16(offset)
		c := kcolor.Argb1555{v}
		return c.Rgba()

	case kcolor.ColorArgb4444:
		v := m.getPixUint16(offset)
		c := kcolor.Argb4444{v}
		return c.Rgba()

	case kcolor.ColorArgb8888:
		v := m.getPixUint32(offset)
		c := kcolor.Argb8888{v}
		return c.Rgba()

	case kcolor.ColorRgba1555:
		v := m.getPixUint16(offset)
		c := kcolor.Rgba1555{v}
		return c.Rgba()

	case kcolor.ColorRgba4444:
		v := m.getPixUint16(offset)
		c := kcolor.Rgba4444{v}
		return c.Rgba()

	case kcolor.ColorRgba8888:
		v := m.getPixUint32(offset)
		c := kcolor.Rgba8888{v}
		return c.Rgba()
	}

	return 0, 0, 0, 0
}

func (m *Canvas) setPixRgba(offset int, c color.RGBA) {
	switch m.ColorFmt {
	case kcolor.ColorArgb1555:
		c1 := kcolor.ARGB1555(c)
		m.setPixUint16(offset, c1.C)

	case kcolor.ColorArgb4444:
		c1 := kcolor.ARGB4444(c)
		m.setPixUint16(offset, c1.C)

	case kcolor.ColorArgb8888:
		c1 := kcolor.ARGB8888(c)
		m.setPixUint32(offset, c1.C)

	case kcolor.ColorRgba1555:
		c1 := kcolor.RGBA1555(c)
		m.setPixUint16(offset, c1.C)

	case kcolor.ColorRgba4444:
		c1 := kcolor.RGBA4444(c)
		m.setPixUint16(offset, c1.C)

	case kcolor.ColorRgba8888:
		c1 := kcolor.RGBA8888(c)
		m.setPixUint32(offset, c1.C)
	}
}
