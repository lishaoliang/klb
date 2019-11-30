///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   argb1555.go
/// @author lishaoliang
/// @brief	argb1555 mem
///  modify $(GO_PATH)/src/image/image.go
///////////////////////////////////////////////////////////////////////////

package kimage

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage/kcolor"
)

// NewArgb1555 Argb1555
func NewArgb1555(r image.Rectangle) *Argb1555 {
	w, h := r.Dx(), r.Dy()
	buf := make([]uint8, 2*w*h)
	return &Argb1555{buf, 2 * w, r}
}

// Argb1555 Argb1555
type Argb1555 struct {
	// Pix holds the image's pixels, in R, G, B, A order and big-endian format. The pixel at
	// (x, y) starts at Pix[(y-Rect.Min.Y)*Stride + (x-Rect.Min.X)*8].
	Pix []uint8

	// Stride is the Pix stride (in bytes) between vertically adjacent pixels.
	Stride int

	// Rect is the image's bounds.
	Rect image.Rectangle
}

// ColorModel color
func (m *Argb1555) ColorModel() color.Model {
	return kcolor.Argb1555Model
}

// Bounds bounds
func (m *Argb1555) Bounds() image.Rectangle {
	return m.Rect
}

// At at
func (m *Argb1555) At(x, y int) color.Color {
	return m.RGBAAt(x, y)
}

// RGBAAt at
func (m *Argb1555) RGBAAt(x, y int) color.RGBA {
	if !(image.Point{x, y}.In(m.Rect)) {
		return color.RGBA{}
	}
	i := m.PixOffset(x, y)
	s := m.Pix[i : i+2 : i+2] // Small cap improves performance, see https://golang.org/issue/27857
	v := (uint16(s[0])) | (uint16(s[1]) << 8)
	c := kcolor.Argb1555{v}
	r, g, b, a := c.Rgba()

	return color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)}
}

// PixOffset returns the index of the first element of Pix that corresponds to
// the pixel at (x, y).
func (m *Argb1555) PixOffset(x, y int) int {
	return (y-m.Rect.Min.Y)*m.Stride + (x-m.Rect.Min.X)*2
}

// Set set color
func (m *Argb1555) Set(x, y int, c color.Color) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	c1 := kcolor.Argb1555Model.Convert(c).(*kcolor.Argb1555)
	s := m.Pix[i : i+2 : i+2] // Small cap improves performance, see https://golang.org/issue/27857
	s[0] = uint8(c1.C)
	s[1] = uint8(c1.C >> 8)
}

// SetRGBA set rgba
func (m *Argb1555) SetRGBA(x, y int, c color.RGBA) {
	if !(image.Point{x, y}.In(m.Rect)) {
		return
	}
	i := m.PixOffset(x, y)
	c1 := kcolor.Argb1555Model.Convert(c).(*kcolor.Argb1555)
	s := m.Pix[i : i+2 : i+2] // Small cap improves performance, see https://golang.org/issue/27857
	s[0] = uint8(c1.C)
	s[1] = uint8(c1.C >> 8)
}

// SubImage returns an image representing the portion of the image p visible
// through r. The returned value shares pixels with the original image.
func (m *Argb1555) SubImage(r image.Rectangle) image.Image {
	r = r.Intersect(m.Rect)
	// If r1 and r2 are Rectangles, r1.Intersect(r2) is not guaranteed to be inside
	// either r1 or r2 if the intersection is empty. Without explicitly checking for
	// this, the Pix[i:] expression below can panic.
	if r.Empty() {
		return &Argb1555{}
	}
	i := m.PixOffset(r.Min.X, r.Min.Y)
	return &Argb1555{
		Pix:    m.Pix[i:],
		Stride: m.Stride,
		Rect:   r,
	}
}

// Opaque scans the entire image and reports whether it is fully opaque.
func (m *Argb1555) Opaque() bool {
	if m.Rect.Empty() {
		return true
	}
	i0, i1 := 0, m.Rect.Dx()*2
	for y := m.Rect.Min.Y; y < m.Rect.Max.Y; y++ {
		for i := i0; i < i1; i += 2 {
			if ((m.Pix[i] >> 7) & 0x1) != 0x1 {
				return false
			}
		}
		i0 += m.Stride
		i1 += m.Stride
	}
	return true
}
