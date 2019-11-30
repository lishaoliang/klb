///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   fill.go
/// @author lishaoliang
/// @brief	draw fill
///////////////////////////////////////////////////////////////////////////

package kdraw

import (
	"image"
	"image/color"

	"github.com/lishaoliang/klb/src/kimage"
)

// FbDrawFill fill
type FbDrawFill interface {
	// Fill
	Fill(r image.Rectangle, c color.Color)

	// FillRGBA
	FillRGBA(r image.Rectangle, c color.RGBA)
}

// DrawFill fill
type DrawFill interface {
	// Fill
	Fill(dst kimage.Image, r image.Rectangle, c color.Color)

	// FillRGBA
	FillRGBA(dst kimage.Image, r image.Rectangle, c color.RGBA)
}

// Fill fill
func Fill(dst kimage.Image, r image.Rectangle, c color.Color) {
	d := dst.Bounds()
	r = r.Intersect(d)
	if r.Empty() {
		return
	}

	for y := r.Min.Y; y < r.Max.Y; y++ {
		for x := r.Min.X; x < r.Max.X; x++ {
			dst.Set(x, y, c)
		}
	}
}

// FillRGBA fill
func FillRGBA(dst kimage.Image, r image.Rectangle, c color.RGBA) {
	d := dst.Bounds()
	r = r.Intersect(d)
	if r.Empty() {
		return
	}

	for y := r.Min.Y; y < r.Max.Y; y++ {
		for x := r.Min.X; x < r.Max.X; x++ {
			dst.SetRGBA(x, y, c)
		}
	}
}
