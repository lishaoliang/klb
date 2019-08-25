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
	// init ctxMan, at ctx_man.go
	ctxManInit()
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

// Ctx func

// Wait wait for done
func Wait() {
	gCtxMan.wg.Wait()
}

// Cancel cancel
func Cancel() {
	gCtxMan.cancel()
}

// Find find
func Find(name string) *Ctx {
	return ctxManFind(name)
}

// FindByEnv find Ctx by *Env
func FindByEnv(env *Env) *Ctx {
	return ctxManFind(env.GetName())
}

// Post post msg
func Post(name, msg, msgex, lparam, wparam string, ptr unsafe.Pointer) bool {
	ctx := ctxManFind(name)

	if nil == ctx {
		return false
	}

	ctx.Post(msg, msgex, lparam, wparam, ptr)
	return true
}

// Close close envOne
func Close(name string) bool {
	ctx := ctxManFind(name)

	if nil == ctx {
		return false
	}

	ctx.Destroy()
	return true
}
