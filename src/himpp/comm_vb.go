///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_vb.go
/// @author lishaoliang
/// @brief	comm vb
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

// VbCpool hiVB_CPOOL_S
type VbCpool struct {
	BlkSize int    // 块大小; block size
	BlkCnt  int    // 块数目; block count
	MmzName string // mmz名称 < 16
}

// VbConf VB_CONF_S
// vb 配置参数
type VbConf struct {
	MaxPool int       // 最大pool数 <= 256
	Pools   []VbCpool // vb的mmz内存池配置
}

// VBCONFS C.VB_CONF_S
type VBCONFS C.VB_CONF_S

// VbConfCopy copy C to Go
// 从C结构体拷贝到VbConf
// copy语意约定为为从其他语言拷贝到 GO
func VbConfCopy(dst *VbConf, src *VBCONFS) {
	dst.MaxPool = int(src.u32MaxPoolCnt)
	dst.Pools = nil

	for i := 0; i < int(C.VB_MAX_COMM_POOLS); i++ {
		var pool VbCpool

		pool.BlkSize = int(src.astCommPool[C.int(i)].u32BlkSize)
		pool.BlkCnt = int(src.astCommPool[C.int(i)].u32BlkCnt)

		if 0 < pool.BlkSize && 0 < pool.BlkCnt {
			pool.MmzName = C.GoString((*C.char)(&(src.astCommPool[C.int(i)].acMmzName[0])))

			dst.Pools = append(dst.Pools, pool)
		}
	}
}

// VbConfCopyC copy Go to C
// 从VbConf拷贝到C结构体
func VbConfCopyC(dst *VBCONFS, src *VbConf) {
	dst.u32MaxPoolCnt = C.HI_U32(src.MaxPool)

	for i := 0; i < len(src.Pools); i++ {
		dst.astCommPool[C.int(i)].u32BlkSize = C.HI_U32(src.Pools[i].BlkSize)
		dst.astCommPool[C.int(i)].u32BlkCnt = C.HI_U32(src.Pools[i].BlkCnt)

		kutil.StrCpyC(unsafe.Pointer(&(dst.astCommPool[C.int(i)].acMmzName[0])), int(C.MAX_MMZ_NAME_LEN), src.Pools[i].MmzName)
	}
}
