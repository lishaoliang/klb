///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb2d_mem.go
/// @author lishaoliang
/// @brief	fb2d mem
///////////////////////////////////////////////////////////////////////////

package kfb2d

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage"
	"github.com/lishaoliang/klb/src/kimage/kcolor"
	"github.com/lishaoliang/klb/src/kimage/kdraw"
)

// NewFb2d NewFb2d
func NewFb2d(r image.Rectangle, colorFmt int) Fb2d {
	var img kimage.Image

	switch colorFmt {
	case kcolor.ColorArgb1555:
		img = kimage.NewArgb1555(r)

	case kcolor.ColorArgb4444:
		colorFmt = kcolor.ColorRgba8888
		img = kimage.NewRgba8888(r)

	case kcolor.ColorArgb8888:
		img = kimage.NewArgb8888(r)

	case kcolor.ColorRgba1555:
		img = kimage.NewRgba1555(r)

	case kcolor.ColorRgba4444:
		colorFmt = kcolor.ColorRgba8888
		img = kimage.NewRgba8888(r)

	case kcolor.ColorRgba8888:
		img = kimage.NewRgba8888(r)

	default:
		colorFmt = kcolor.ColorRgba8888
		img = kimage.NewRgba8888(r)

	}

	return &fb2dMem{img, colorFmt}
}

// fb2dMem fb2dMem
type fb2dMem struct {
	img      kimage.Image
	colorFmt int // 颜色格式
}

// ColorModel color
func (m *fb2dMem) ColorModel() color.Model {
	return kcolor.ColorModel(m.colorFmt)
}

// Bounds bounds
func (m *fb2dMem) Bounds() image.Rectangle {
	return m.img.Bounds()
}

// At at
func (m *fb2dMem) At(x, y int) color.Color {
	return m.img.At(x, y)
}

// Set set color
func (m *fb2dMem) Set(x, y int, c color.Color) {
	m.img.Set(x, y, c)
}

// SetRGBA set color
func (m *fb2dMem) SetRGBA(x, y int, c color.RGBA) {
	m.img.SetRGBA(x, y, c)
}

func (m *fb2dMem) Fb2dV1() string {
	return "fb2dmem"
}

func (m *fb2dMem) GetVersion() string {
	return m.Fb2dV1()
}

func (m *fb2dMem) Refresh(r image.Rectangle) {

}

func (m *fb2dMem) Line(p1, p2 image.Point, pix int, c color.Color) {
	kdraw.Line(m.img, p1, p2, pix, c)
}

func (m *fb2dMem) LineRGBA(p1, p2 image.Point, pix int, c color.RGBA) {
	kdraw.LineRGBA(m.img, p1, p2, pix, c)
}

func (m *fb2dMem) Fill(r image.Rectangle, c color.Color) {
	kdraw.Fill(m.img, r, c)
}

func (m *fb2dMem) FillRGBA(r image.Rectangle, c color.RGBA) {
	kdraw.FillRGBA(m.img, r, c)
}

func (m *fb2dMem) Rect(r image.Rectangle, pix int, c color.Color) {
	kdraw.Rect(m.img, r, pix, c)
}

func (m *fb2dMem) RectRGBA(r image.Rectangle, pix int, c color.RGBA) {
	kdraw.RectRGBA(m.img, r, pix, c)
}

func (m *fb2dMem) Copy(dstRect image.Rectangle, src kimage.Image, srcPt image.Point) {
	kdraw.Copy(m.img, dstRect, src, srcPt)
}
