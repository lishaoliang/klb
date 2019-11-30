///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   canvas.go
/// @author lishaoliang
/// @brief	canvas
///////////////////////////////////////////////////////////////////////////

package kcanvas

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage/kcolor"
)

// Canvas 画布
type Canvas struct {
	Rect     image.Rectangle // Rect is the image's bounds.
	Addr     uintptr         // 显存映射的虚拟地址
	PhyAddr  uintptr         // 显存物理地址,无物理地址时填0
	MemLen   int             // 显存长度
	Stride   int             // Stride is the Pix stride (in bytes) between vertically adjacent pixels.
	ColorFmt int             // 颜色格式
}

// ColorModel ColorModel
func (m *Canvas) ColorModel() color.Model {
	return kcolor.ColorModel(m.ColorFmt)
}

// Bounds bounds
func (m *Canvas) Bounds() image.Rectangle {
	return m.Rect
}

// At at
func (m *Canvas) At(x, y int) color.Color {
	return m.RGBAAt(x, y)
}

// RGBAAt at
func (m *Canvas) RGBAAt(x, y int) color.RGBA {
	if !(image.Point{x, y}.In(m.Rect)) {
		return color.RGBA{}
	}
	i := m.PixOffset(x, y)
	r, g, b, a := m.getPixRgba(i)
	return color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)}
}

// PixOffset returns the index of the first element of Pix that corresponds to
// the pixel at (x, y).
func (m *Canvas) PixOffset(x, y int) int {
	return (y-m.Rect.Min.Y)*m.Stride + (x-m.Rect.Min.X)*kcolor.ColorBpp(m.ColorFmt)
}

// Set set color
func (m *Canvas) Set(x, y int, c color.Color) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	c1 := color.RGBAModel.Convert(c).(color.RGBA)
	m.setPixRgba(i, c1)
}

// SetRGBA set rgba
func (m *Canvas) SetRGBA(x, y int, c color.RGBA) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	m.setPixRgba(i, c)
}

// SubImage returns an image representing the portion of the image p visible
// through r. The returned value shares pixels with the original image.
func (m *Canvas) SubImage(r image.Rectangle) image.Image {
	r = r.Intersect(m.Rect)
	// If r1 and r2 are Rectangles, r1.Intersect(r2) is not guaranteed to be inside
	// either r1 or r2 if the intersection is empty. Without explicitly checking for
	// this, the Pix[i:] expression below can panic.
	if r.Empty() {
		return &Canvas{}
	}
	i := m.PixOffset(r.Min.X, r.Min.Y)

	return &Canvas{
		r,
		m.Addr + uintptr(i),
		m.PhyAddr + uintptr(i),
		m.MemLen - i,
		m.Stride,
		m.ColorFmt}
}

// Opaque scans the entire image and reports whether it is fully opaque.
func (m *Canvas) Opaque() bool {
	if m.Rect.Empty() {
		return true
	}
	for y := m.Rect.Min.Y; y < m.Rect.Max.Y; y++ {
		for x := m.Rect.Min.X; x < m.Rect.Max.X; x++ {
			c := m.RGBAAt(x, y)
			if c.A != 0xff {
				return false
			}
		}
	}
	return true
}
