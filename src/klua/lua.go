///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/10
//
/// @file    lua.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	Lua
///////////////////////////////////////////////////////////////////////////
package klua

/*
#include "stdlib.h"
#include "klua/klua.h"
*/
import (
	"C"
)

import (
	"unsafe"
)

// LuaCFunction lua c function
type LuaCFunction C.lua_CFunction

// LuaState lua state
type LuaState C.lua_State

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

// luaIntToBool int to bool;
// 0. false;
// not 0. true
func luaIntToBool(v int) bool {
	if 0 != v {
		return true
	}
	return false
}

// luaBoolToInt bool to int;
// false. 0;
// true. 1
func luaBoolToInt(b bool) int {
	if b {
		return 1
	}
	return 0
}

// access functions (stack -> C -> Go) ------------------------------------------------------------

// LuaIsnumber lua_isnumber
func LuaIsnumber(lua *LuaState, idx int) bool {
	b := C.lua_isnumber((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
}

// LuaIsstring lua_isstring
func LuaIsstring(lua *LuaState, idx int) bool {
	b := C.lua_isstring((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
}

// LuaIscfunction lua_iscfunction
func LuaIscfunction(lua *LuaState, idx int) bool {
	b := C.lua_iscfunction((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
}

// LuaIsinteger lua_isinteger
func LuaIsinteger(lua *LuaState, idx int) bool {
	b := C.lua_isinteger((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
}

// LuaIsuserdata lua_isuserdata
func LuaIsuserdata(lua *LuaState, idx int) bool {
	b := C.lua_isuserdata((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
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
	return luaIntToBool(int(b))
}

// LuaTostring lua_tolstring
func LuaTostring(lua *LuaState, idx int) string {
	s := C.lua_tolstring((*C.lua_State)(lua), C.int(idx), nil)
	return C.GoString(s)
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

// LuaLCheckboolean luaL_checkboolean
func LuaLCheckboolean(lua *LuaState, idx int) bool {
	b := C.luaL_checkboolean((*C.lua_State)(lua), C.int(idx))
	return luaIntToBool(int(b))
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

// LuaPushboolean lua_pushboolean
func LuaPushboolean(lua *LuaState, b bool) {
	n := luaBoolToInt(b)
	C.lua_pushboolean((*C.lua_State)(lua), C.int(n))
}

// LuaPushlightuserdata lua_pushlightuserdata
func LuaPushlightuserdata(lua *LuaState, p unsafe.Pointer) {
	C.lua_pushlightuserdata((*C.lua_State)(lua), p)
}

// get functions (Lua -> stack) ------------------------------------------------------------
