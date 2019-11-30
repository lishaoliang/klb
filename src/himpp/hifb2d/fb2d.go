///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb2d.go
/// @author lishaoliang
/// @brief	fb2d
///////////////////////////////////////////////////////////////////////////

package hifb2d

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage/kcolor"
	"github.com/lishaoliang/klb/src/kimage/kdraw"

	"github.com/lishaoliang/klb/src/himpp"
	"github.com/lishaoliang/klb/src/kimage"
	"github.com/lishaoliang/klb/src/kimage/kcanvas"
	"github.com/lishaoliang/klb/src/kimage/kfb2d"
)

// NewFb2d new fb2d
func NewFb2d(canvas kcanvas.Canvas) *Fb2d {
	var m Fb2d

	m.canvas = canvas
	m.tde = himpp.NewTde()

	return &m
}

// Fb2d fb
type Fb2d struct {
	kfb2d.Fb2d

	canvas kcanvas.Canvas
	tde    *himpp.Tde
}

// Fb2dV1 fb2dv1
func (m *Fb2d) Fb2dV1() string {
	return "himpp,argb888,fb2dv1"
}

// GetVersion version
func (m *Fb2d) GetVersion() string {
	return m.Fb2dV1()
}

// Refresh refresh
func (m *Fb2d) Refresh(r image.Rectangle) {

}

// Line draw line
func (m *Fb2d) Line(p1, p2 image.Point, pix int, c color.Color) {

}

// LineRGBA draw line
func (m *Fb2d) LineRGBA(p1, p2 image.Point, pix int, c color.RGBA) {

}

// Fill draw fill
func (m *Fb2d) Fill(rect image.Rectangle, c color.Color) {
	r, g, b, a := c.RGBA()
	m.FillRGBA(rect, color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)})
}

// FillRGBA draw fill
func (m *Fb2d) FillRGBA(rect image.Rectangle, c color.RGBA) {
	v, _ := kcolor.RGBAtoUint32(c, m.canvas.ColorFmt)

	m.tde.BeginJob()
	m.tde.Fill(&m.canvas, rect, v)
	m.tde.EndJob()
}

// Rect draw rect
func (m *Fb2d) Rect(rect image.Rectangle, pix int, c color.Color) {
	r, g, b, a := c.RGBA()
	m.RectRGBA(rect, pix, color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)})
}

// RectRGBA draw rect
func (m *Fb2d) RectRGBA(rect image.Rectangle, pix int, c color.RGBA) {
	if pix <= 0 {
		return
	}

	v, _ := kcolor.RGBAtoUint32(c, m.canvas.ColorFmt)

	m.tde.BeginJob()
	m.tde.Fill(&m.canvas, image.Rect(rect.Min.X, rect.Min.Y, rect.Max.X, rect.Min.Y+pix), v) // top
	m.tde.Fill(&m.canvas, image.Rect(rect.Max.X-pix, rect.Min.Y, rect.Max.X, rect.Max.Y), v) // right
	m.tde.Fill(&m.canvas, image.Rect(rect.Min.X, rect.Max.Y-pix, rect.Max.X, rect.Max.Y), v) // bottom
	m.tde.Fill(&m.canvas, image.Rect(rect.Min.X, rect.Min.Y, rect.Min.X+pix, rect.Max.Y), v) // left
	m.tde.EndJob()
}

// Copy copy
func (m *Fb2d) Copy(dstRect image.Rectangle, src kimage.Image, srcPt image.Point) {
	if srcFb2d, ok := src.(*Fb2d); ok {
		m.tde.BeginJob()
		m.tde.Copy(&m.canvas, dstRect, &srcFb2d.canvas, srcPt)
		m.tde.EndJob()

		return
	}

	kdraw.Copy(m, dstRect, src, srcPt)
}
