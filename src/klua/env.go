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
import (
	"unsafe"
)

// Env *C.klua_env_t *C.lua_State, CGO
// Env must not has Go pointer
// or panic: runtime error: cgo argument has Go pointer to Go pointer
type Env struct {
	env *C.klua_env_t
	lua *LuaState // *C.lua_State, C pointer

	kgo LuaInteger // "kgo"

	name *C.char // Can't get by Go pointer, use string replace
}

// EnvCreate create lua env
func EnvCreate(name string, preLoad LuaCFunction) *Env {
	var e Env
	e.name = C.CString(name)
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
	C.free(unsafe.Pointer(e.name))

	e.lua = nil
	e.env = nil
	e.name = nil
}

// GetName get name
func (e *Env) GetName() string {
	return C.GoString(e.name)
}

// DoFile call C.klua_env_dofile
func (e *Env) DoFile(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dofile(e.env, cStr)
	e.kgo = (LuaInteger)(C.klua_env_kgo(e.env))
	//fmt.Println("do file", ret, e.kgo)

	return int(ret)
}

// DoLibrary call C.klua_env_dolibrary
func (e *Env) DoLibrary(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dolibrary(e.env, cStr)
	e.kgo = (LuaInteger)(C.klua_env_kgo(e.env))

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
func (e *Env) CallKgo(msg, msgex, lparam, wparam string, ptr unsafe.Pointer) int {
	cMsg := C.CString(msg)
	defer C.free(unsafe.Pointer(cMsg))

	cMsgex := C.CString(msgex)
	defer C.free(unsafe.Pointer(cMsgex))

	cLparam := C.CString(lparam)
	defer C.free(unsafe.Pointer(cLparam))

	cWparam := C.CString(wparam)
	defer C.free(unsafe.Pointer(cWparam))

	ret := C.klua_env_call_kgo(e.env, cMsg, cMsgex, cLparam, cWparam, ptr)
	return int(ret)
}

// CallKgoB call
func (e *Env) CallKgoB(msg, msgex, lparam, wparam []byte, ptr unsafe.Pointer) int {

	if e.kgo <= 0 {
		return 1
	}

	LuaRawgeti(e.lua, LUAREGISTRYINDEX, e.kgo) // to call 'kgo' in protected mode
	LuaPushstringB(e.lua, msg)                 // 1st argument
	LuaPushstringB(e.lua, msgex)               // 2st argument
	LuaPushstringB(e.lua, lparam)              // 3st argument
	LuaPushstringB(e.lua, wparam)              // 4st argument
	LuaPushlightuserdata(e.lua, ptr)           // 5st argument

	status := LuaPcall(e.lua, 5, 1, 0) // do the call
	result := LuaToboolean(e.lua, -1)  // get result

	if result {
		LuaPop(e.lua, 1)
	} else {
		C.klua_env_report(e.env, C.int(1))
	}

	if result && 0 == status {
		return 0
	}

	return 1
}
