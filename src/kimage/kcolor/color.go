///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   color.go
/// @author lishaoliang
/// @brief	color interface
///////////////////////////////////////////////////////////////////////////

package kcolor

import (
	"image/color"
)

// Color color
type Color interface {
	// RGBA() (r, g, b, a uint32)
	// range:[0, 65535]
	color.Color // RGBA() (r, g, b, a uint32)

	// Rgba r,g,b,a
	// range:[0,255]
	Rgba() (r, g, b, a uint32)

	// "#000000FF"
	Str() string

	// "rgba(0, 0, 0, 255)"
	StrRgba() string
}

// Color type
const (
	ColorArgb1555 = 1
	ColorArgb4444 = 2
	ColorArgb8888 = 3

	ColorRgba1555 = 11
	ColorRgba4444 = 12
	ColorRgba8888 = 13
)

// ColorBpp color bpp
// 像素占用的字节数
func ColorBpp(colorFmt int) int {
	switch colorFmt {
	case ColorArgb1555:
		return 2

	case ColorArgb4444:
		return 2

	case ColorArgb8888:
		return 4

	case ColorRgba1555:
		return 2

	case ColorRgba4444:
		return 2

	case ColorRgba8888:
		return 4
	}

	return 4
}

// Models for the color types.
var (
	Argb1555Model = color.ModelFunc(argb1555Model)
	Argb4444Model = color.ModelFunc(argb4444Model)
	Argb8888Model = color.ModelFunc(argb8888Model)

	Rgba1555Model = color.ModelFunc(rgba1555Model)
	Rgba4444Model = color.ModelFunc(rgba4444Model)
	Rgba8888Model = color.ModelFunc(rgba8888Model)
)

// ColorModel ColorModel
func ColorModel(colorFmt int) color.Model {
	switch colorFmt {
	case ColorArgb1555:
		return Argb1555Model

	case ColorArgb4444:
		return Argb4444Model

	case ColorArgb8888:
		return Argb8888Model

	case ColorRgba1555:
		return Rgba1555Model

	case ColorRgba4444:
		return Rgba4444Model

	case ColorRgba8888:
		return Rgba8888Model
	}

	return Rgba8888Model
}

func argb1555Model(c color.Color) color.Color {
	if _, ok := c.(*Argb1555); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return ARGB1555(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}

func argb4444Model(c color.Color) color.Color {
	if _, ok := c.(*Argb4444); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return ARGB4444(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}

func argb8888Model(c color.Color) color.Color {
	if _, ok := c.(*Argb8888); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return ARGB8888(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}

func rgba1555Model(c color.Color) color.Color {
	if _, ok := c.(*Rgba1555); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return RGBA1555(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}

func rgba4444Model(c color.Color) color.Color {
	if _, ok := c.(*Rgba4444); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return RGBA4444(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}

func rgba8888Model(c color.Color) color.Color {
	if _, ok := c.(*Rgba8888); ok {
		return c
	}
	r, g, b, a := c.RGBA()
	return RGBA8888(color.RGBA{uint8(r >> 8), uint8(g >> 8), uint8(b >> 8), uint8(a >> 8)})
}
