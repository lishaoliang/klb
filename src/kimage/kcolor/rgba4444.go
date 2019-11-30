///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rgba4444.go
/// @author lishaoliang
/// @brief	color rgba4444
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"fmt"
	"image/color"
)

// Rgba4444 Rgba4444
type Rgba4444 struct {
	// color.Color
	C uint16
}

// RGBA4444 Rgba4444
func RGBA4444(c color.RGBA) *Rgba4444 {
	r := (uint16(c.R) & 0xFF) >> 4
	g := (uint16(c.G) & 0xFF) >> 4
	b := (uint16(c.B) & 0xFF) >> 4
	a := (uint16(c.A) & 0xFF) >> 4

	return &Rgba4444{uint16((r << 12) | (g << 8) | (b << 4) | (a))}
}

// StrRgba4444 Rgba4444
// fmt : "#000000FF"
// fmt : "#000000"
// fmt : "rgba(0, 0, 0, 255)"
// fmt : "rgb(0, 0, 0)"
func StrRgba4444(c string) *Rgba4444 {
	rgba := StrToRGBA(c)
	return RGBA4444(rgba)
}

// RGBA r,g,b,a; range: [0, 65535]
func (m *Rgba4444) RGBA() (r, g, b, a uint32) {
	r, g, b, a = m.Rgba()

	r |= r << 8
	g |= g << 8
	b |= b << 8
	a |= a << 8

	return
}

// Rgba r,g,b,a; range: [0, 255]
func (m *Rgba4444) Rgba() (r, g, b, a uint32) {
	r = uint32((m.C>>12)&0xF) << 4
	g = uint32((m.C>>8)&0xF) << 4
	b = uint32((m.C>>4)&0xF) << 4
	a = uint32((m.C)&0xF) << 4

	return
}

// Str string, "#000000FF"
func (m *Rgba4444) Str() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("#%02X%02X%02X%02X", r, g, b, a)
}

// StrRgba "rgba(0, 0, 0, 255)"
func (m *Rgba4444) StrRgba() string {
	r, g, b, a := m.Rgba()
	return fmt.Sprintf("rgba(%d,%d,%d,%d)", r, g, b, a)
}
