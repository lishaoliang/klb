///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rect.go
/// @author lishaoliang
/// @brief	draw rect
///////////////////////////////////////////////////////////////////////////

package kdraw

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage"
)

// FbDrawRect fb draw rect
type FbDrawRect interface {
	Rect(r image.Rectangle, pix int, c color.Color)
	RectRGBA(r image.Rectangle, pix int, c color.RGBA)
}

// DrawRect draw rect
type DrawRect interface {
	Rect(dst kimage.Image, r image.Rectangle, pix int, c color.Color)
	RectRGBA(dst kimage.Image, r image.Rectangle, pix int, c color.RGBA)
}

// Rect draw rect
func Rect(dst kimage.Image, r image.Rectangle, pix int, c color.Color) {
	if pix <= 0 {
		return
	}

	Fill(dst, image.Rect(r.Min.X, r.Min.Y, r.Max.X, r.Min.Y+pix), c) // top
	Fill(dst, image.Rect(r.Max.X-pix, r.Min.Y, r.Max.X, r.Max.Y), c) // right
	Fill(dst, image.Rect(r.Min.X, r.Max.Y-pix, r.Max.X, r.Max.Y), c) // bottom
	Fill(dst, image.Rect(r.Min.X, r.Min.Y, r.Min.X+pix, r.Max.Y), c) // left
}

// RectRGBA draw rect
func RectRGBA(dst kimage.Image, r image.Rectangle, pix int, c color.RGBA) {
	if pix <= 0 {
		return
	}

	FillRGBA(dst, image.Rect(r.Min.X, r.Min.Y, r.Max.X, r.Min.Y+pix), c) // top
	FillRGBA(dst, image.Rect(r.Max.X-pix, r.Min.Y, r.Max.X, r.Max.Y), c) // right
	FillRGBA(dst, image.Rect(r.Min.X, r.Max.Y-pix, r.Max.X, r.Max.Y), c) // bottom
	FillRGBA(dst, image.Rect(r.Min.X, r.Min.Y, r.Min.X+pix, r.Max.Y), c) // left
}
