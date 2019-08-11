///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/10
//
/// @file    klua.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	klua
///////////////////////////////////////////////////////////////////////////
package klua

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
#include "stdlib.h"
#include "klua/klua.h"
*/
import "C"
import (
	"unsafe"
)

// init
func init() {
	// int com, at com.go
	comInit()

	// init ctxMan, at ctx_man.go
	ctxManInit()

	// init envManInit, at env_man.go
	envManInit()
}

// LoadLib load lib
func LoadLib(lua *LuaState, f LuaCFunction, name string) {
	cStr := C.CString(name)
	defer C.free(unsafe.Pointer(cStr))

	C.klua_loadlib((*C.lua_State)(unsafe.Pointer(lua)), f, cStr)
}

// Open C libs
// eg. "k*"

// OpenKos kos
func OpenKos(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kos)), name)
}

// OpenKtime ktime
func OpenKtime(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_ktime)), name)
}

// OpenKthread kthread
func OpenKthread(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kthread)), name)
}

// Open Go libs
// eg. "kg_*" "ka_*" "kx_*"
