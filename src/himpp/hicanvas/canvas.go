///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   canvas.go
/// @author lishaoliang
/// @brief	canvas
///////////////////////////////////////////////////////////////////////////

package hicanvas

import (
	"image"
	"unsafe"

	"github.com/lishaoliang/klb/src/kimage/kcolor"

	"github.com/lishaoliang/klb/src/himpp"
	"github.com/lishaoliang/klb/src/kimage/kcanvas"
)

// MmzCanvasCreate MmzCanvasCreate
func MmzCanvasCreate(r image.Rectangle, colorFmt int) *MmzCanvas {
	w := r.Dx()
	h := r.Dy()
	bpp := kcolor.ColorBpp(colorFmt)
	stride := w * bpp
	memLen := stride * h

	phyAddr, addr := himpp.SysMmzMalloc(memLen)
	if nil == addr {
		return nil
	}

	return &MmzCanvas{kcanvas.Canvas{r, uintptr(addr), phyAddr, memLen, stride, colorFmt}}
}

// MmzCanvas mmz canvas
type MmzCanvas struct {
	kcanvas.Canvas
}

// Destroy Destroy
func (m *MmzCanvas) Destroy() {
	if 0 != m.Addr {
		himpp.SysMmzFree(m.PhyAddr, unsafe.Pointer(m.Addr))
	}

	*m = MmzCanvas{}
}
