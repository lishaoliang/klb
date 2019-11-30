///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   line.go
/// @author lishaoliang
/// @brief	draw line
///////////////////////////////////////////////////////////////////////////

package kdraw

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage"
)

// FbDrawLine line
type FbDrawLine interface {
	Line(p1, p2 image.Point, pix int, c color.Color)
	LineRGBA(p1, p2 image.Point, pix int, c color.RGBA)
}

// DrawLine line
type DrawLine interface {
	Line(dst kimage.Image, p1, p2 image.Point, pix int, c color.Color)
	LineRGBA(dst kimage.Image, p1, p2 image.Point, pix int, c color.RGBA)
}

// Line draw line
func Line(dst kimage.Image, p1, p2 image.Point, pix int, c color.Color) {
	c1 := color.RGBAModel.Convert(c).(color.RGBA)
	LineRGBA(dst, p1, p2, pix, c1)
}

// LineRGBA draw line
func LineRGBA(dst kimage.Image, p1, p2 image.Point, pix int, c color.RGBA) {
	r := image.Rect(p1.X, p1.Y, p2.X, p2.Y)

	if r.Min.X == r.Max.X {
		FillRGBA(dst, image.Rect(r.Min.X, r.Min.Y, r.Min.X+pix, r.Max.Y), c)
	} else if r.Min.Y == r.Max.Y {
		FillRGBA(dst, image.Rect(r.Min.X, r.Min.Y, r.Max.X, r.Min.Y+pix), c)
	} else {

	}
}
