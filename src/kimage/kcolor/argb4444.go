///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   argb4444.go
/// @author lishaoliang
/// @brief	color argb4444
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Argb4444 Argb4444
type Argb4444 struct {
	// color.Color
	C uint16
}

// ARGB4444 ARGB4444
func ARGB4444(c color.RGBA) *Argb4444 {
	a := (uint16(c.A) & 0xFF) >> 4
	r := (uint16(c.R) & 0xFF) >> 4
	g := (uint16(c.G) & 0xFF) >> 4
	b := (uint16(c.B) & 0xFF) >> 4

	return &Argb4444{uint16((a << 12) | (r << 8) | (g << 4) | b)}
}

// StrArgb4444 Argb4444
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrArgb4444(c string) *Argb4444 {
	rgba := StrToRGBA(c)
	return ARGB4444(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Argb4444) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	a |= a << 8
	r |= r << 8
	g |= g << 8
	b |= b << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Argb4444) Rgba() (r, g, b, a uint32) {
	a = uint32((m.C>>12)&0xF) << 4
	r = uint32((m.C>>8)&0xF) << 4
	g = uint32((m.C>>4)&0xF) << 4
	b = uint32((m.C)&0xF) << 4

	return
}

// Str string, "#000000FF"
func (m *Argb4444) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Argb4444) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
