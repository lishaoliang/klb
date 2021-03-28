///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	env.go
/// @author	lishaoliang
/// @brief	env
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
func EnvCreate(name string, preLoad LuaCFunction, loader LuaCFunction) *Env {
	var m Env
	m.name = C.CString(name)
	m.env = C.klua_env_create(preLoad)
	m.lua = (*LuaState)(unsafe.Pointer(C.klua_env_get_L(m.env)))

	// set Env ptr at user data
	// so Env must not has Go pointer
	C.klua_env_set_udata(m.env, unsafe.Pointer(&m))

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (m *Env) Destroy() {
	C.klua_env_destroy(m.env)
	C.free(unsafe.Pointer(m.name))

	m.lua = nil
	m.env = nil
	m.name = nil
}

// DoFile call C.klua_env_dofile
func (m *Env) DoFile(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dofile(m.env, cStr)
	m.kgo = (LuaInteger)(C.klua_env_kgo(m.env))
	//fmt.Println("do file", ret, e.kgo)

	return int(ret)
}

// DoLibrary call C.klua_env_dolibrary
func (m *Env) DoLibrary(loader string) int {
	// do file
	cStr := C.CString(loader)
	defer C.free(unsafe.Pointer(cStr))

	ret := C.klua_env_dolibrary(m.env, cStr)
	m.kgo = (LuaInteger)(C.klua_env_kgo(m.env))

	return int(ret)
}

// DoEnd call C.klua_env_doend
func (m *Env) DoEnd() int {
	// do end
	ret := C.klua_env_doend(m.env)

	return int(ret)
}

// GetLuaState GetLuaState
func (m *Env) GetLuaState() *LuaState {
	return m.lua
}

// Report klua_env_report
func (m *Env) Report(status int) {
	C.klua_env_report(m.env, C.int(status))
}

// HasKgo has "kgo" in *.lua file
func (m *Env) HasKgo() bool {
	ret := C.klua_env_has_kgo(m.env)

	if 0 == ret {
		return true
	}

	return false
}

// CallKgo call C.klua_env_call_kgo; "kgo" in *.lua
func (m *Env) CallKgo(msg, msgex, lparam, wparam string, ptr unsafe.Pointer) int {
	cMsg := C.CString(msg)
	defer C.free(unsafe.Pointer(cMsg))

	cMsgex := C.CString(msgex)
	defer C.free(unsafe.Pointer(cMsgex))

	cLparam := C.CString(lparam)
	defer C.free(unsafe.Pointer(cLparam))

	cWparam := C.CString(wparam)
	defer C.free(unsafe.Pointer(cWparam))

	ret := C.klua_env_call_kgo(m.env, cMsg, cMsgex, cLparam, cWparam, ptr)
	return int(ret)
}

// CallKgoB call
func (m *Env) CallKgoB(msg, msgex, lparam, wparam []byte, ptr unsafe.Pointer) int {

	if m.kgo <= 0 {
		return 1
	}

	LuaRawgeti(m.lua, LUAREGISTRYINDEX, m.kgo) // to call 'kgo' in protected mode
	LuaPushstringB(m.lua, msg)                 // 1st argument
	LuaPushstringB(m.lua, msgex)               // 2st argument
	LuaPushstringB(m.lua, lparam)              // 3st argument
	LuaPushstringB(m.lua, wparam)              // 4st argument
	LuaPushlightuserdata(m.lua, ptr)           // 5st argument

	status := LuaPcall(m.lua, 5, 1, 0) // do the call
	result := LuaToboolean(m.lua, -1)  // get result

	if 0 == status {
		LuaPop(m.lua, 1)
	} else {
		C.klua_env_report(m.env, C.int(status))
	}

	if result && 0 == status {
		return 0
	}

	return 1
}

// LoopOnce call C.klua_env_loop_once
func (m *Env) LoopOnce() int {
	// do klua_env_loop_once
	ret := C.klua_env_loop_once(m.env)

	return int(ret)
}

// IsExit call C.klua_env_is_exit
func (m *Env) IsExit() bool {
	// do klua_env_is_exit
	if C.klua_env_is_exit(m.env) {
		return true
	}

	return false
}
