///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   select.go
/// @author lishaoliang
/// @brief	select
///////////////////////////////////////////////////////////////////////////

package klbc

/*
#include "go/klb_select.h"
*/
import "C"

// TimeVal time val
type TimeVal struct {
	TvSec  int // seconds
	TvUsec int // microseconds
}

// TIMEVAL C.timeval
type TIMEVAL C.struct_timeval

// TimeValCopy copy C to Go
// 从C结构体拷贝到Point
func TimeValCopy(dst *TimeVal, src *TIMEVAL) {
	dst.TvSec = int(src.tv_sec)
	dst.TvUsec = int(src.tv_usec)
}

// TimeValCopyC copy Go to C
// 从Point拷贝到C结构体
func TimeValCopyC(dst *TIMEVAL, src *TimeVal) {
	dst.tv_sec = C.long(src.TvSec)
	dst.tv_usec = C.long(src.TvUsec)
}

// FDSET C.fd_set
type FDSET C.fd_set

// Select C select
// select在轻量级环境下,使用更为简便
func Select(nfds uintptr, readfds, writefds, exceptfds *FDSET, timeout *TimeVal) int {
	var tv C.struct_timeval
	TimeValCopyC((*TIMEVAL)(&tv), timeout)

	ret := C.KLB_SELECT(C.int(nfds), (*C.fd_set)(readfds), (*C.fd_set)(writefds), (*C.fd_set)(exceptfds), (*C.struct_timeval)(&tv))
	return int(ret)
}

// FdZero FD_ZERO
// 清零
func FdZero(fdset *FDSET) {
	C.klb_fd_zero((*C.fd_set)(fdset))
}

// FdSet FD_SET
// 设置某个fd到fdset中
func FdSet(fd uintptr, fdset *FDSET) {
	C.klb_fd_set(C.uintptr_t(fd), (*C.fd_set)(fdset))
}

// FdClr FD_CLR
// 在fdset中清除某个fd
func FdClr(fd uintptr, fdset *FDSET) {
	C.klb_fd_clr(C.uintptr_t(fd), (*C.fd_set)(fdset))
}

// FdIsSet FD_ISSET
// 某个fd是否在fdset中
func FdIsSet(fd uintptr, fdset *FDSET) int {
	ret := C.klb_fd_is_set(C.uintptr_t(fd), (*C.fd_set)(fdset))
	return int(ret)
}
