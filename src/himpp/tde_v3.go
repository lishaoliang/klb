///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   tde_v3.go
/// @author lishaoliang
/// @brief	tde v3
///  usage: go build -tags="himppv3"
///////////////////////////////////////////////////////////////////////////

// +build himppv3

package himpp

/*
#include "klb_type.h"
#include "himpp/klb_himpp.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/fb.h>

#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"

*/
import "C"
import "github.com/lishaoliang/klb/src/kimage/kcolor"

func (m *Tde) tdeColorFmt(colorFmt int) C.int {
	switch colorFmt {
	case kcolor.ColorArgb1555:
		return C.TDE2_COLOR_FMT_ARGB1555
	case kcolor.ColorArgb8888:
		return C.TDE2_COLOR_FMT_ARGB8888
	case kcolor.ColorArgb4444:
		return C.TDE2_COLOR_FMT_ARGB4444
	case kcolor.ColorRgba1555:
		return C.TDE2_COLOR_FMT_RGBA1555
	case kcolor.ColorRgba8888:
		return C.TDE2_COLOR_FMT_RGBA8888
	case kcolor.ColorRgba4444:
		return C.TDE2_COLOR_FMT_RGBA4444
	default:
		return C.TDE2_COLOR_FMT_ARGB8888
	}
}
