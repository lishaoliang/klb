///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rgba1555.go
/// @author lishaoliang
/// @brief	color rgba1555
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Rgba1555 rgba1555
type Rgba1555 struct {
	// color.Color
	C uint16
}

// RGBA1555 Rgba1555
func RGBA1555(c color.RGBA) *Rgba1555 {
	r := (uint16(c.R) & 0xFF) >> 3
	g := (uint16(c.G) & 0xFF) >> 3
	b := (uint16(c.B) & 0xFF) >> 3

	a := (uint16(c.A) & 0xFF)
	if 0 != a {
		a = 0x1
	}

	return &Rgba1555{uint16((r << 11) | (g << 6) | (b << 1) | (a))}
}

// StrRgba1555 Rgba1555
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrRgba1555(c string) *Rgba1555 {
	rgba := StrToRGBA(c)
	return RGBA1555(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Rgba1555) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	r |= r << 8
	g |= g << 8
	b |= b << 8
	a |= a << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Rgba1555) Rgba() (r, g, b, a uint32) {

	r = uint32((m.C>>11)&0x1F) << 3
	g = uint32((m.C>>6)&0x1F) << 3
	b = uint32((m.C>>1)&0x1F) << 3

	a = uint32((m.C) & 0x1)
	if 0 != a {
		a = 0xFF // [0, 255]
	}

	return
}

// Str string, "#000000FF"
func (m *Rgba1555) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Rgba1555) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
