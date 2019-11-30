///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   comm_sys.go
/// @author lishaoliang
/// @brief	comm sys
///////////////////////////////////////////////////////////////////////////

package himpp

/*
#include "himpp/klb_himpp.h"
*/
import "C"

// SysConf sys config
// sys 配置参数
type SysConf struct {
	Align int // 图像对齐参数, 取值: [1,1024] && 16的倍数
}

// SYSCONFS C.MPP_SYS_CONF_S
type SYSCONFS C.MPP_SYS_CONF_S

// SysConfCopy copy C to Go
// 从C结构体拷贝到SysConf
// copy语意约定为从其他语言拷贝到 Go
func SysConfCopy(dst *SysConf, src *SYSCONFS) {
	dst.Align = int(src.u32AlignWidth)
}

// SysConfCopyC copy Go to C
// 从SysConf拷贝到C结构体
func SysConfCopyC(dst *SYSCONFS, src *SysConf) {
	dst.u32AlignWidth = C.HI_U32(src.Align)
}
