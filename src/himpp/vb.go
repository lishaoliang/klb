///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   vb.go
/// @author lishaoliang
/// @brief	vb
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

import (
	"fmt"

	"github.com/lishaoliang/klb/src/kutil"
)

// VbInit HI_MPI_VB_Init
// vb 初始化
func VbInit() int {
	ret := C.HI_MPI_VB_Init()
	return int(ret)
}

// VbExit HI_MPI_VB_Exit
// vb 退出
func VbExit() int {
	ret := C.HI_MPI_VB_Exit()
	return int(ret)
}

// VbSetConf HI_MPI_VB_SetConf
// 设置vb初始化参数; 在VbInit之前设置
func VbSetConf(conf *VbConf) int {
	var cConf VBCONFS
	VbConfCopyC(&cConf, conf)

	ret := C.HI_MPI_VB_SetConf((*C.VB_CONF_S)(&cConf))
	return int(ret)
}

// VbGetConf HI_MPI_VB_GetConf
// 获取vb初始化参数
func VbGetConf() (*VbConf, error) {
	var cConf VBCONFS

	ret := C.HI_MPI_VB_GetConf((*C.VB_CONF_S)(&cConf))
	if 0 == ret {
		var conf VbConf
		VbConfCopy(&conf, &cConf)

		return &conf, nil
	}

	return nil, fmt.Errorf("himpp.vb.VbGetConf error!HI_MPI_VB_GetConf=0x%X", uint(ret))
}

// VbPrintConf 打印vb参数
func VbPrintConf(conf *VbConf) {
	fmt.Println("vb conf ---->>>>")
	fmt.Println("vb MaxPool:", conf.MaxPool)

	for i := 0; i < len(conf.Pools); i++ {
		fmt.Printf("vb Pools[%d],BlkSize:[%dK],BlkCnt:[%d],Total:[%dM]\n", i, conf.Pools[i].BlkSize/1024, conf.Pools[i].BlkCnt,
			conf.Pools[i].BlkSize*conf.Pools[i].BlkCnt/(1024*1024))
	}
	fmt.Println("vb conf <<<<----")
}

// ceiLing2Power C.CEILING_2_POWER ( ((x) + ((a) - 1) ) & ( ~((a) - 1) ) )
func ceiLing2Power(x, a int) int {
	return ((x + a - 1) / a) * a
}

// VbCalcBlkSize calc vb block size
func VbCalcBlkSize(w, h, fmt, align int) int {
	//align := 64
	kutil.Assert(16 == align || 32 == align || 64 == align)

	size := float64(ceiLing2Power(w, align)) * float64(ceiLing2Power(h, align))

	if PixelFormatYuvSemiplanar422 == fmt {
		size *= 2
		size += float64(16 * h * 2) // VB_PIC_HEADER_SIZE
	} else {
		size *= 1.5
		size += (float64(16*h*3) / 2) // VB_PIC_HEADER_SIZE
	}

	return int(size)
}
