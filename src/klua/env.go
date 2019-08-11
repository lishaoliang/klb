///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/10
//
/// @file    env.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	Env
///////////////////////////////////////////////////////////////////////////
package klua

/*
#include "stdlib.h"
#include "klua/klua.h"
*/
import "C"
import "unsafe"

// Env *C.klua_env_t *C.lua_State, CGO
// Env must not has Go pointer
// or panic: runtime error: cgo argument has Go pointer to Go pointer
type Env struct {
	env *C.klua_env_t
	lua *LuaState // *C.lua_State, C pointer

	ctxName *C.char // Can't get Ctx by Go pointer, use string replace
}

// EnvCreate create lua env
func EnvCreate(ctxName string, preLoad LuaCFunction) *Env {
	var e Env
	e.ctxName = C.CString(ctxName)
	e.env = C.klua_env_create(preLoad)
	e.lua = (*LuaState)(unsafe.Pointer(C.klua_env_get_L(e.env)))

	// set Env ptr at user data
	// so Env must not has Go pointer
	C.klua_env_set_udata(e.env, unsafe.Pointer(&e))

	return &e
}

// EnvGetByLua get env by luastate
func EnvGetByLua(lua *LuaState) *Env {

	// get *C.klua_env_t by lua
	env := C.klua_env_get_by_L((*C.lua_State)(unsafe.Pointer(lua)))

	// get Env ptr by *C.klua_env_t at user data
	e := (*Env)(unsafe.Pointer(C.klua_env_get_udata(env)))

	return e
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (e *Env) Destroy() {
	C.klua_env_destroy(e.env)
	C.free(unsafe.Pointer(e.ctxName))

	e.lua = nil
	e.env = nil
	e.ctxName = nil
}

// GetCtxName get Ctx name
func (e *Env) GetCtxName() string {
	return C.GoString(e.ctxName)
}

// DoFile call C.klua_env_dofile
func (e *Env) DoFile(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dofile(e.env, cStr)

	return int(ret)
}

// DoLibrary call C.klua_env_dolibrary
func (e *Env) DoLibrary(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dolibrary(e.env, cStr)

	return int(ret)
}

// HasKgo has "kgo" in *.lua file
func (e *Env) HasKgo() bool {
	ret := C.klua_env_has_kgo(e.env)

	if 0 == ret {
		return true
	}

	return false
}

// CallKgo call C.klua_env_call_kgo; "kgo" in *.lua
func (e *Env) CallKgo(msg, lparam, wparam string, ptr unsafe.Pointer) int {
	cMsg := C.CString(msg)
	defer C.free(unsafe.Pointer(cMsg))

	cLparam := C.CString(lparam)
	defer C.free(unsafe.Pointer(cLparam))

	cWparam := C.CString(wparam)
	defer C.free(unsafe.Pointer(cWparam))

	ret := C.klua_env_call_kgo(e.env, cMsg, cLparam, cWparam, ptr)
	return int(ret)
}
