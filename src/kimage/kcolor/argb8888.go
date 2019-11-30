///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   argb8888.go
/// @author lishaoliang
/// @brief	color argb8888
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Argb8888 argb8888
type Argb8888 struct {
	// color.Color
	C uint32
}

// ARGB8888 Argb8888
func ARGB8888(c color.RGBA) *Argb8888 {
	return &Argb8888{((uint32(c.A) << 24) | (uint32(c.R) << 16) | (uint32(c.G) << 8) | uint32(c.B))}
}

// StrArgb8888 Argb8888
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrArgb8888(c string) *Argb8888 {
	rgba := StrToRGBA(c)
	return ARGB8888(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Argb8888) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	a |= a << 8
	r |= r << 8
	g |= g << 8
	b |= b << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Argb8888) Rgba() (r, g, b, a uint32) {
	a = (m.C >> 24) & 0xFF
	r = (m.C >> 16) & 0xFF
	g = (m.C >> 8) & 0xFF
	b = (m.C) & 0xFF

	return
}

// Str string, "#000000FF"
func (m *Argb8888) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Argb8888) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
