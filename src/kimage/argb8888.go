///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   argb8888.go
/// @author lishaoliang
/// @brief	argb8888 mem
///  modify $(GO_PATH)/src/image/image.go
///////////////////////////////////////////////////////////////////////////

package kimage

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage/kcolor"
)

// NewArgb8888 Argb8888
func NewArgb8888(r image.Rectangle) *Argb8888 {
	w, h := r.Dx(), r.Dy()
	buf := make([]uint8, 4*w*h)
	return &Argb8888{buf, 4 * w, r}
}

// Argb8888 argb8888
type Argb8888 struct {
	// Pix holds the image's pixels, in R, G, B, A order and big-endian format. The pixel at
	// (x, y) starts at Pix[(y-Rect.Min.Y)*Stride + (x-Rect.Min.X)*8].
	Pix []uint8

	// Stride is the Pix stride (in bytes) between vertically adjacent pixels.
	Stride int

	// Rect is the image's bounds.
	Rect image.Rectangle
}

// ColorModel color
func (m *Argb8888) ColorModel() color.Model {
	return kcolor.Argb8888Model
}

// Bounds bounds
func (m *Argb8888) Bounds() image.Rectangle {
	return m.Rect
}

// At at
func (m *Argb8888) At(x, y int) color.Color {
	return m.RGBAAt(x, y)
}

// RGBAAt at
func (m *Argb8888) RGBAAt(x, y int) color.RGBA {
	if !(image.Point{x, y}.In(m.Rect)) {
		return color.RGBA{}
	}
	i := m.PixOffset(x, y)
	s := m.Pix[i : i+4 : i+4] // Small cap improves performance, see https://golang.org/issue/27857
	return color.RGBA{s[1], s[2], s[3], s[0]}
}

// PixOffset returns the index of the first element of Pix that corresponds to
// the pixel at (x, y).
func (m *Argb8888) PixOffset(x, y int) int {
	return (y-m.Rect.Min.Y)*m.Stride + (x-m.Rect.Min.X)*4
}

// Set set color
func (m *Argb8888) Set(x, y int, c color.Color) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	c1 := color.RGBAModel.Convert(c).(color.RGBA)
	s := m.Pix[i : i+4 : i+4] // Small cap improves performance, see https://golang.org/issue/27857
	s[0] = c1.A
	s[1] = c1.R
	s[2] = c1.G
	s[3] = c1.B
}

// SetRGBA set rgba
func (m *Argb8888) SetRGBA(x, y int, c color.RGBA) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	s := m.Pix[i : i+4 : i+4] // Small cap improves performance, see https://golang.org/issue/27857
	s[0] = c.A
	s[1] = c.R
	s[2] = c.G
	s[3] = c.B
}

// SubImage returns an image representing the portion of the image p visible
// through r. The returned value shares pixels with the original image.
func (m *Argb8888) SubImage(r image.Rectangle) image.Image {
	r = r.Intersect(m.Rect)
	// If r1 and r2 are Rectangles, r1.Intersect(r2) is not guaranteed to be inside
	// either r1 or r2 if the intersection is empty. Without explicitly checking for
	// this, the Pix[i:] expression below can panic.
	if r.Empty() {
		return &Argb8888{}
	}
	i := m.PixOffset(r.Min.X, r.Min.Y)
	return &Argb8888{
		Pix:    m.Pix[i:],
		Stride: m.Stride,
		Rect:   r,
	}
}

// Opaque scans the entire image and reports whether it is fully opaque.
func (m *Argb8888) Opaque() bool {
	if m.Rect.Empty() {
		return true
	}
	i0, i1 := 0, m.Rect.Dx()*4
	for y := m.Rect.Min.Y; y < m.Rect.Max.Y; y++ {
		for i := i0; i < i1; i += 4 {
			if m.Pix[i] != 0xff {
				return false
			}
		}
		i0 += m.Stride
		i1 += m.Stride
	}
	return true
}
