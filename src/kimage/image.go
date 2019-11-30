///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   image.go
/// @author lishaoliang
/// @brief	image interface
///////////////////////////////////////////////////////////////////////////

package kimage

import (
	"image/color"
	"image/draw"
)

// Image Image
type Image interface {
	draw.Image

	// SetRGBA set color
	SetRGBA(x, y int, c color.RGBA)
}
