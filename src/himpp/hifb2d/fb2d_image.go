///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fb2d_image.go
/// @author lishaoliang
/// @brief	fb2d image
///////////////////////////////////////////////////////////////////////////

package hifb2d

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage/kcanvas"
	"github.com/lishaoliang/klb/src/kutil"
)

// ColorModel color
func (m *Fb2d) ColorModel() color.Model {
	return m.canvas.ColorModel()
}

// Bounds bounds
func (m *Fb2d) Bounds() image.Rectangle {
	return m.canvas.Bounds()
}

// At at
func (m *Fb2d) At(x, y int) color.Color {
	return m.canvas.At(x, y)
}

// RGBAAt at
func (m *Fb2d) RGBAAt(x, y int) color.RGBA {
	return m.canvas.RGBAAt(x, y)
}

// PixOffset returns the index of the first element of Pix that corresponds to
// the pixel at (x, y).
func (m *Fb2d) PixOffset(x, y int) int {
	return m.canvas.PixOffset(x, y)
}

// Set set color
func (m *Fb2d) Set(x, y int, c color.Color) {
	m.canvas.Set(x, y, c)
}

// SetRGBA set rgba
func (m *Fb2d) SetRGBA(x, y int, c color.RGBA) {
	m.canvas.SetRGBA(x, y, c)
}

// SubImage returns an image representing the portion of the image p visible
func (m *Fb2d) SubImage(r image.Rectangle) image.Image {
	canvas := m.canvas.SubImage(r)
	if c, ok := canvas.(*kcanvas.Canvas); ok {
		return NewFb2d(*c)
	}

	kutil.Assert(false)
	return NewFb2d(kcanvas.Canvas{})
}

// Opaque scans the entire image and reports whether it is fully opaque.
func (m *Fb2d) Opaque() bool {
	return m.canvas.Opaque()
}
