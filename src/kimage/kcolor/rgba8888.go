///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rgba8888.go
/// @author lishaoliang
/// @brief	color rgba8888
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Rgba8888 rgba8888
type Rgba8888 struct {
	// color.Color
	C uint32
}

// RGBA8888 rgba8888
func RGBA8888(c color.RGBA) *Rgba8888 {
	return &Rgba8888{((uint32(c.R) << 24) | (uint32(c.G) << 16) | (uint32(c.B) << 8) | (uint32(c.A)))}
}

// StrRgba8888 rgba8888
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrRgba8888(c string) *Rgba8888 {
	rgba := StrToRGBA(c)
	return RGBA8888(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Rgba8888) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	r |= r << 8
	g |= g << 8
	b |= b << 8
	a |= a << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Rgba8888) Rgba() (r, g, b, a uint32) {

	r = (m.C >> 24) & 0xFF
	g = (m.C >> 16) & 0xFF
	b = (m.C >> 8) & 0xFF
	a = (m.C) & 0xFF

	return
}

// Str string, "#000000FF"
func (m *Rgba8888) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Rgba8888) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
