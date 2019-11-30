///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	lua.go
/// @author	lishaoliang
/// @brief	lua
///////////////////////////////////////////////////////////////////////////

package klua

/*
#include "stdlib.h"
#include "string.h"
#include "klua/klua.h"
*/
import (
	"C"
)

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/kutil"
)

// LuaCFunction lua c function
type LuaCFunction C.lua_CFunction

// LuaState lua state
type LuaState C.lua_State

// LuaInteger lua_Integer
type LuaInteger C.lua_Integer

// LUA basic types
const (
	LUATNONE          = int(C.LUA_TNONE)          //LUA_TNONE
	LUATNIL           = int(C.LUA_TNIL)           //LUA_TNIL
	LUATBOOLEAN       = int(C.LUA_TBOOLEAN)       //LUA_TBOOLEAN
	LUATLIGHTUSERDATA = int(C.LUA_TLIGHTUSERDATA) //LUA_TLIGHTUSERDATA
	LUATNUMBER        = int(C.LUA_TNUMBER)        //LUA_TNUMBER
	LUATSTRING        = int(C.LUA_TSTRING)        //LUA_TSTRING
	LUATTABLE         = int(C.LUA_TTABLE)         //LUA_TTABLE
	LUATFUNCTION      = int(C.LUA_TFUNCTION)      //LUA_TFUNCTION
	LUATUSERDATA      = int(C.LUA_TUSERDATA)      //LUA_TUSERDATA
	LUATTHREAD        = int(C.LUA_TTHREAD)        //LUA_TTHREAD
	LUANUMTAGS        = int(C.LUA_NUMTAGS)        //LUA_NUMTAGS
)

// LUA
const (
	LUAREGISTRYINDEX = int(C.LUA_REGISTRYINDEX) // LUA_REGISTRYINDEX
)

// LuaIntToBool int to bool;
// 0. false;
// not 0. true
func LuaIntToBool(v int) bool {
	if 0 != v {
		return true
	}
	return false
}

// LuaBoolToInt bool to int;
// false. 0;
// true. 1
func LuaBoolToInt(b bool) int {
	if b {
		return 1
	}
	return 0
}

// basic stack manipulation -----------------------------------------------------------------------

// LuaSettop lua_settop
func LuaSettop(lua *LuaState, idx int) {
	C.lua_settop((*C.lua_State)(lua), C.int(idx))
}

// access functions (stack -> C -> Go) ------------------------------------------------------------

// LuaIsnumber lua_isnumber
func LuaIsnumber(lua *LuaState, idx int) bool {
	b := C.lua_isnumber((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaIsstring lua_isstring
func LuaIsstring(lua *LuaState, idx int) bool {
	b := C.lua_isstring((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaIscfunction lua_iscfunction
func LuaIscfunction(lua *LuaState, idx int) bool {
	b := C.lua_iscfunction((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaIsinteger lua_isinteger
func LuaIsinteger(lua *LuaState, idx int) bool {
	b := C.lua_isinteger((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaIsuserdata lua_isuserdata
func LuaIsuserdata(lua *LuaState, idx int) bool {
	b := C.lua_isuserdata((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaType lua_type, lua type at idx
func LuaType(lua *LuaState, idx int) int {
	t := C.lua_type((*C.lua_State)(lua), C.int(idx))
	return int(t)
}

// LuaTypename lua_typename
func LuaTypename(lua *LuaState, idx int) string {
	name := C.lua_typename((*C.lua_State)(lua), C.int(idx))
	return C.GoString(name)
}

// LuaTonumber lua_tonumberx
func LuaTonumber(lua *LuaState, idx int) float64 {
	num := C.lua_tonumberx((*C.lua_State)(lua), C.int(idx), nil)
	return float64(num)
}

// LuaTointeger lua_tointegerx
func LuaTointeger(lua *LuaState, idx int) int64 {
	i := C.lua_tointegerx((*C.lua_State)(lua), C.int(idx), nil)
	return int64(i)
}

// LuaToboolean lua_toboolean
func LuaToboolean(lua *LuaState, idx int) bool {
	b := C.lua_toboolean((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaTostring lua_tolstring
func LuaTostring(lua *LuaState, idx int) string {
	s := C.lua_tolstring((*C.lua_State)(lua), C.int(idx), nil)
	return C.GoString(s)
}

// LuaTostringPtr lua_tolstring
func LuaTostringPtr(lua *LuaState, idx int) (unsafe.Pointer, int) {
	l := C.size_t(0)
	s := C.lua_tolstring((*C.lua_State)(lua), C.int(idx), &l)

	return unsafe.Pointer(s), int(l)
}

// LuaTocfunction lua_tocfunction
func LuaTocfunction(lua *LuaState, idx int) LuaCFunction {
	f := C.lua_tocfunction((*C.lua_State)(lua), C.int(idx))
	return (LuaCFunction)(unsafe.Pointer(f))
}

// LuaTouserdata lua_touserdata
func LuaTouserdata(lua *LuaState, idx int) unsafe.Pointer {
	ptr := C.lua_touserdata((*C.lua_State)(lua), C.int(idx))
	return unsafe.Pointer(ptr)
}

// LuaTothread lua_tothread
func LuaTothread(lua *LuaState, idx int) *LuaState {
	ptr := C.lua_tothread((*C.lua_State)(lua), C.int(idx))
	return (*LuaState)(unsafe.Pointer(ptr))
}

// LuaTopointer lua_topointer
func LuaTopointer(lua *LuaState, idx int) unsafe.Pointer {
	ptr := C.lua_topointer((*C.lua_State)(lua), C.int(idx))
	return unsafe.Pointer(ptr)
}

// check functions (stack -> C -> Go) ------------------------------------------------------------

// LuaLChecknumber luaL_checknumber
func LuaLChecknumber(lua *LuaState, idx int) float64 {
	n := C.luaL_checknumber((*C.lua_State)(lua), C.int(idx))
	return float64(n)
}

// LuaLCheckinteger luaL_checkinteger
func LuaLCheckinteger(lua *LuaState, idx int) int64 {
	n := C.luaL_checkinteger((*C.lua_State)(lua), C.int(idx))
	return int64(n)
}

// LuaLCheckstring luaL_checklstring
func LuaLCheckstring(lua *LuaState, idx int) string {
	s := C.luaL_checklstring((*C.lua_State)(lua), C.int(idx), nil)
	return C.GoString(s)
}

// LuaLCheckstringB luaL_checklstring
func LuaLCheckstringB(lua *LuaState, idx int) []byte {
	l := C.size_t(0)
	s := C.luaL_checklstring((*C.lua_State)(lua), C.int(idx), &l)

	b := make([]byte, l)
	kutil.Memcpy(b, unsafe.Pointer(s), int(l))

	return b
}

// LuaLCheckstringPtr luaL_checklstring
func LuaLCheckstringPtr(lua *LuaState, idx int) (unsafe.Pointer, int) {
	l := C.size_t(0)
	s := C.luaL_checklstring((*C.lua_State)(lua), C.int(idx), &l)

	return unsafe.Pointer(s), int(l)
}

// LuaLCheckboolean luaL_checkboolean
func LuaLCheckboolean(lua *LuaState, idx int) bool {
	b := C.luaL_checkboolean((*C.lua_State)(lua), C.int(idx))
	return LuaIntToBool(int(b))
}

// LuaLChecklightuserdata luaL_checklightuserdata
func LuaLChecklightuserdata(lua *LuaState, idx int) unsafe.Pointer {
	p := C.luaL_checklightuserdata((*C.lua_State)(lua), C.int(idx))
	return unsafe.Pointer(p)
}

// push functions (Go -> C -> stack) ------------------------------------------------------------

// LuaPushnil lua_pushnil
func LuaPushnil(lua *LuaState) {
	C.lua_pushnil((*C.lua_State)(lua))
}

// LuaPushnumber lua_pushnumber
func LuaPushnumber(lua *LuaState, n float64) {
	C.lua_pushnumber((*C.lua_State)(lua), C.double(n))
}

// LuaPushinteger lua_pushinteger
func LuaPushinteger(lua *LuaState, n int64) {
	C.lua_pushinteger((*C.lua_State)(lua), C.longlong(n))
}

// LuaPushstring lua_pushstring
func LuaPushstring(lua *LuaState, s string) {
	cStr := C.CString(s)
	defer C.free(unsafe.Pointer(cStr))

	C.lua_pushstring((*C.lua_State)(lua), cStr)
}

// LuaPushstringB lua_pushlstring
func LuaPushstringB(lua *LuaState, b []byte) {
	l := len(b)

	if 0 < l {
		C.lua_pushlstring((*C.lua_State)(lua), (*C.char)(unsafe.Pointer(&b[0])), C.size_t(l))
	} else {
		C.lua_pushlstring((*C.lua_State)(lua), nil, C.size_t(0))
	}
}

// LuaPushboolean lua_pushboolean
func LuaPushboolean(lua *LuaState, b bool) {
	n := LuaBoolToInt(b)
	C.lua_pushboolean((*C.lua_State)(lua), C.int(n))
}

// LuaPushlightuserdata lua_pushlightuserdata
func LuaPushlightuserdata(lua *LuaState, p unsafe.Pointer) {
	C.lua_pushlightuserdata((*C.lua_State)(lua), p)
}

// get functions (Lua -> stack) ------------------------------------------------------------

// LuaRawgeti lua_rawgeti
func LuaRawgeti(lua *LuaState, idx int, n LuaInteger) {
	C.lua_rawgeti((*C.lua_State)(lua), C.int(idx), C.lua_Integer(n))
}

// 'load' and 'call' functions (load and run Lua code) -------------------------------------

// LuaPcall lua_pcall
func LuaPcall(lua *LuaState, nargs, nresults, errfunc int) int {
	ret := C.lua_pcallk((*C.lua_State)(lua), C.int(nargs), C.int(nresults), C.int(errfunc), 0, nil)
	return int(ret)
}

// some useful macros ----------------------------------------------------------------------

// LuaPop lua_pop
func LuaPop(lua *LuaState, idx int) {
	// lua_pop(L,n)		lua_settop(L, -(n)-1)
	LuaSettop(lua, -(idx)-1)
}
