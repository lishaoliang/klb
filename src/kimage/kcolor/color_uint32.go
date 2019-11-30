///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   color_uint32.go
/// @author lishaoliang
/// @brief	color RGBA to uint32
///////////////////////////////////////////////////////////////////////////

package kcolor

import "image/color"

// RGBAtoUint32 RGBA to uint32
func RGBAtoUint32(c color.RGBA, colorFmt int) (v uint32, bpp int) {
	switch colorFmt {
	case ColorArgb1555:
		argb := ARGB1555(c)
		return uint32(argb.C), 2

	case ColorArgb4444:
		argb := ARGB4444(c)
		return uint32(argb.C), 2

	case ColorArgb8888:
		argb := ARGB8888(c)
		return argb.C, 4

	case ColorRgba1555:
		rgba := RGBA1555(c)
		return uint32(rgba.C), 2

	case ColorRgba4444:
		rgba := RGBA4444(c)
		return uint32(rgba.C), 2

	case ColorRgba8888:
		rgba := RGBA8888(c)
		return rgba.C, 4
	}

	return 0, 4
}
