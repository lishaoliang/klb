///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   rgba8888.go
/// @author lishaoliang
/// @brief	rgba8888 mem
///////////////////////////////////////////////////////////////////////////

package kimage

import "image"

// NewRgba8888 Rgba8888 image
func NewRgba8888(r image.Rectangle) *image.RGBA {
	return image.NewRGBA(r)
}
