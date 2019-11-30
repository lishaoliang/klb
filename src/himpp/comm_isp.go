///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_isp.go
/// @author lishaoliang
/// @brief	comm isp
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/kutil"
)

// AlgLib isp alg lib
// isp sensor alg lib
type AlgLib struct {
	ID   int
	Name string
}

// ALGLIBS C.ALG_LIB_S
type ALGLIBS C.ALG_LIB_S

// AlgLibCopy copy C to Go
// 从C结构体拷贝到AlgLib
// copy语意约定为从其他语言拷贝到 GO
func AlgLibCopy(dst *AlgLib, src *ALGLIBS) {
	dst.ID = int(src.s32Id)
	dst.Name = C.GoString((*C.char)(&src.acLibName[0]))
}

// AlgLibCopyC copy Go to C
// 从AlgLib拷贝到C结构体
func AlgLibCopyC(dst *ALGLIBS, src *AlgLib) {
	dst.s32Id = C.HI_S32(src.ID)

	kutil.StrCpyC(unsafe.Pointer(&dst.acLibName[0]), int(C.ALG_LIB_NAME_SIZE_MAX), src.Name)
}

// IspWdrMode isp wdr mode
type IspWdrMode struct {
	WdrMode int
}

// ISPWDRMODES C.ISP_WDR_MODE_S
type ISPWDRMODES C.ISP_WDR_MODE_S

// IspWdrModeCopy copy C to Go
// copy语意约定为从其他语言拷贝到 GO
func IspWdrModeCopy(dst *IspWdrMode, src *ISPWDRMODES) {
	dst.WdrMode = int(src.enWDRMode)
}

// IspWdrModeCopyC copy Go to C
func IspWdrModeCopyC(dst *ISPWDRMODES, src *IspWdrMode) {
	dst.enWDRMode = C.WDR_MODE_E(src.WdrMode)
}
