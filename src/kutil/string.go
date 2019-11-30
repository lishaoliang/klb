///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   string.go
/// @author lishaoliang
/// @brief	string
///////////////////////////////////////////////////////////////////////////

package kutil

/*
#include "string.h"
#include "stdlib.h"

void* klua_go_kutil_pointer_add(void* ptr, int off)
{
	return (char*)(ptr) + off;
}

*/
import "C"

import (
	"unsafe"
)

// StrCpyC copy
// 从字符串拷贝到 C指针, 末尾置0
func StrCpyC(dst unsafe.Pointer, dstMax int, src string) {
	if nil == dst || dstMax <= 0 {
		return
	}

	copyLen := Min(dstMax-1, len(src))
	if 0 < copyLen {
		b := []byte(src)
		C.memcpy(dst, unsafe.Pointer(&b[0]), C.size_t(copyLen))
	}

	ptr := (*C.char)(PointerAdd(dst, copyLen))
	*ptr = 0
}

// Memcpy memcpy from void* to []byte
// 从任意C指针拷贝数据到[]byte
// copy语意约定为从其他语言拷贝到 GO
func Memcpy(dst []byte, src unsafe.Pointer, size int) {
	if nil == dst || nil == src || size <= 0 {
		return
	}

	C.memcpy(unsafe.Pointer(&dst[0]), src, C.size_t(size))
}

// MemcpyB memcpy from []byte to void*
// 从[]byte拷贝数据到任意C指针
func MemcpyB(dst unsafe.Pointer, src []byte) {
	size := len(src)
	if nil == dst || size <= 0 {
		return
	}

	C.memcpy(dst, unsafe.Pointer(&src[0]), C.size_t(size))
}

// PointerAdd pointer add, step 1 Byte
// 指针加法,步长1字节
func PointerAdd(ptr unsafe.Pointer, off int) unsafe.Pointer {
	return C.klua_go_kutil_pointer_add(ptr, C.int(off))
}

// Malloc malloc mem
// 申请C内存
func Malloc(size int) unsafe.Pointer {
	ptr := C.malloc(C.size_t(size))
	return unsafe.Pointer(ptr)
}

// Free free mem
// 释放C内存
func Free(ptr unsafe.Pointer) {
	if nil != ptr {
		C.free(ptr)
	}
}
