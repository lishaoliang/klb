///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   argb1555.go
/// @author lishaoliang
/// @brief	color argb1555
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Argb1555 argb1555
type Argb1555 struct {
	C uint16
}

// ARGB1555 Argb1555
func ARGB1555(c color.RGBA) *Argb1555 {
	a := (uint16(c.A) & 0xFF)
	if 0 != a {
		a = 0x1
	}

	r := (uint16(c.R) & 0xFF) >> 3
	g := (uint16(c.G) & 0xFF) >> 3
	b := (uint16(c.B) & 0xFF) >> 3

	return &Argb1555{uint16((a << 15) | (r << 10) | (g << 5) | b)}
}

// StrArgb1555 Argb1555
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrArgb1555(c string) *Argb1555 {
	rgba := StrToRGBA(c)
	return ARGB1555(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Argb1555) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	a |= a << 8
	r |= r << 8
	g |= g << 8
	b |= b << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Argb1555) Rgba() (r, g, b, a uint32) {
	a = uint32((m.C >> 15) & 0x1)
	if 0 != a {
		a = 0xFF // [0, 255]
	}

	r = uint32((m.C>>10)&0x1F) << 3
	g = uint32((m.C>>5)&0x1F) << 3
	b = uint32((m.C)&0x1F) << 3

	return
}

// Str string, "#000000FF"
func (m *Argb1555) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Argb1555) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
