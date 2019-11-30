///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   sys.go
/// @author lishaoliang
/// @brief	sys
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
#include "klb_type.h"
*/
import "C"

import (
	"fmt"
	"unsafe"
)

// SysInit HI_MPI_SYS_Init
// 初始化 MPP 系统
func SysInit() int {
	ret := C.HI_MPI_SYS_Init()
	return int(ret)
}

// SysExit HI_MPI_SYS_Exit
// 去初始化 MPP 系统
func SysExit() int {
	ret := C.HI_MPI_SYS_Exit()
	return int(ret)
}

// SysSetConf HI_MPI_SYS_SetConf
// 设置sys初始化参数; 在SysInit函数之前设置
func SysSetConf(conf *SysConf) int {
	var cConf SYSCONFS
	SysConfCopyC(&cConf, conf)

	ret := C.HI_MPI_SYS_SetConf((*C.MPP_SYS_CONF_S)(&cConf))
	return int(ret)
}

// SysGetConf HI_MPI_SYS_GetConf
// 获取sys初始化参数
func SysGetConf() (*SysConf, error) {
	var cConf SYSCONFS

	ret := C.HI_MPI_SYS_GetConf((*C.MPP_SYS_CONF_S)(&cConf))
	if 0 == ret {
		var conf SysConf
		SysConfCopy(&conf, &cConf)

		return &conf, nil
	}

	return nil, fmt.Errorf("himpp.sys.SysGetConf error!HI_MPI_SYS_GetConf=0x%X", uint(ret))
}

// SysBind HI_MPI_SYS_Bind
// 绑定
func SysBind(src *MppChn, dst *MppChn) int {
	var cSrc MPPCHNS
	MppChnCopyC(&cSrc, src)

	var cDst MPPCHNS
	MppChnCopyC(&cDst, dst)

	ret := C.HI_MPI_SYS_Bind((*C.MPP_CHN_S)(&cSrc), (*C.MPP_CHN_S)(&cDst))
	return int(ret)
}

// SysUnBind HI_MPI_SYS_UnBind
// 解绑
func SysUnBind(src *MppChn, dst *MppChn) int {
	var cSrc MPPCHNS
	MppChnCopyC(&cSrc, src)

	var cDst MPPCHNS
	MppChnCopyC(&cDst, dst)

	ret := C.HI_MPI_SYS_UnBind((*C.MPP_CHN_S)(&cSrc), (*C.MPP_CHN_S)(&cDst))
	return int(ret)
}

// SysGetVersion HI_MPI_SYS_GetVersion
// 获取版本字符串
func SysGetVersion() (string, error) {
	var ver C.MPP_VERSION_S

	ret := C.HI_MPI_SYS_GetVersion(&ver)
	if 0 == ret {
		return C.GoString((*C.char)(unsafe.Pointer(&ver.aVersion[0]))), nil
	}

	return "", fmt.Errorf("himpp.sys.SysGetVersion error!HI_MPI_SYS_GetVersion=0x%X", uint(ret))
}

// SysGetChipID HI_MPI_SYS_GetChipId
// 获取芯片ID
// func SysGetChipID() (uint32, error) {
//	var id C.HI_U32
//
//	ret := C.HI_MPI_SYS_GetChipId(&id)
//	if 0 == ret {
//		return uint32(id), nil
//	}
//
//	return 0, fmt.Errorf("himpp.sys.SysGetChipID error!HI_MPI_SYS_GetChipId=0x%X", uint(ret))
//}

// SysGetCurPts HI_MPI_SYS_GetCurPts
// 获取当前mpp系统的pts
func SysGetCurPts() (uint64, error) {
	var pts C.HI_U64

	ret := C.HI_MPI_SYS_GetCurPts(&pts)
	if 0 == ret {
		return uint64(pts), nil
	}

	return 0, fmt.Errorf("himpp.sys.SysGetCurPts error!HI_MPI_SYS_GetCurPts=0x%X", uint(ret))
}

// SysInitPtsBase HI_MPI_SYS_InitPtsBase
// 初始化mpp系统的pts[微妙]
func SysInitPtsBase(pts uint64) int {
	ret := C.HI_MPI_SYS_InitPtsBase(C.HI_U64(pts))
	return int(ret)
}

// SysSyncPts HI_MPI_SYS_SyncPts
// 同步mpp系统时间[微妙]
func SysSyncPts(pts uint64) int {
	ret := C.HI_MPI_SYS_SyncPts(C.HI_U64(pts))
	return int(ret)
}

// SysMmzMalloc HI_MPI_SYS_MmzAlloc
// 在用户态分配MMZ内存
func SysMmzMalloc(size int) (uintptr, unsafe.Pointer) {
	var phyAddr C.HI_U32
	var addr unsafe.Pointer

	ret := C.HI_MPI_SYS_MmzAlloc(&phyAddr, &addr, nil, nil, C.HI_U32(size))
	if 0 != ret {
		fmt.Printf("himpp.sys.SysMmzMalloc error!HI_MPI_SYS_MmzAlloc:[0x%X],size:[%d]\n", uint(ret), size)
		return 0, nil
	}

	return uintptr(phyAddr), addr
}

// SysMmzFree HI_MPI_SYS_MmzFree
// 释放在用户态分配的MMZ内存
func SysMmzFree(phyAddr uintptr, addr unsafe.Pointer) {
	ret := C.HI_MPI_SYS_MmzFree(C.HI_U32(phyAddr), addr)
	if 0 != ret {
		fmt.Printf("himpp.sys.SysMmzFree error!HI_MPI_SYS_MmzFree=0x%X,PhyAddr:[0x%X],Addr:[%p]\n", uint(ret), phyAddr, addr)
	}
}

// SysMmzMallocCached HI_MPI_SYS_MmzAlloc_Cached
// 在用户态分配MMZ内存,该内存支持 cache 缓存
func SysMmzMallocCached(size int) (uintptr, unsafe.Pointer) {
	var phyAddr C.HI_U32
	var addr unsafe.Pointer

	ret := C.HI_MPI_SYS_MmzAlloc_Cached(&phyAddr, &addr, nil, nil, C.HI_U32(size))
	if 0 != ret {
		fmt.Printf("himpp.sys.SysMmzMallocCached error!HI_MPI_SYS_MmzAlloc_Cached:[0x%X],size:[%d]\n", uint(ret), size)
		return 0, nil
	}

	return uintptr(phyAddr), addr
}

// SysMmzFlushCached HI_MPI_SYS_MmzFlushCache
// 在用户态分配MMZ内存,该内存支持 cache 缓存
func SysMmzFlushCached(phyAddr uintptr, addr unsafe.Pointer, size int) int {
	ret := C.HI_MPI_SYS_MmzFlushCache(C.HI_U32(phyAddr), addr, C.HI_U32(size))
	if 0 != ret {
		fmt.Printf("himpp.sys.SysMmzFlushCached error!HI_MPI_SYS_MmzFlushCache=0x%X,PhyAddr:[0x%X],Addr:[%p],MemLen:[%d]\n", uint(ret), phyAddr, addr, size)
	}

	return int(ret)
}

// SysCloseFd HI_MPI_SYS_CloseFd
// 关闭所有sys使用的文件句柄
func SysCloseFd() int {
	ret := C.HI_MPI_SYS_CloseFd()
	return int(ret)
}

// SysSetTimeZone HI_MPI_SYS_SetTimeZone
// 设置时区
//  tz 范围: [-86400, 86400]秒
//func SysSetTimeZone(tz int) int {
//	ret := C.HI_MPI_SYS_SetTimeZone(C.HI_S32(tz))
//	return int(ret)
//}

// SysGetTimeZone HI_MPI_SYS_GetTimeZone
// 获取时区
//func SysGetTimeZone() (int, error) {
//	var tz C.HI_S32
//	ret := C.HI_MPI_SYS_GetTimeZone(&tz)
//	if 0 == ret {
//		return int(tz), nil
//	}
//
//	return 0, fmt.Errorf("himpp.sys.SysGetTimeZone error!HI_MPI_SYS_GetTimeZone=0x%X", uint(ret))
//}
