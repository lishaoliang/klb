///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   copy.go
/// @author lishaoliang
/// @brief	draw copy
///////////////////////////////////////////////////////////////////////////

package kdraw

import (
	"image"
	"image/draw"

	"github.com/lishaoliang/klb/src/kimage"
)

// FbDrawCopy copy
type FbDrawCopy interface {
	Copy(dstRect image.Rectangle, src kimage.Image, srcPt image.Point)
}

// DrawCopy copy
type DrawCopy interface {
	Copy(dst kimage.Image, dstRect image.Rectangle, src kimage.Image, srcPt image.Point)
}

// Copy draw copy
func Copy(dst kimage.Image, dstRect image.Rectangle, src kimage.Image, srcPt image.Point) {
	draw.Over.Draw(dst, dstRect, src, srcPt)
}
