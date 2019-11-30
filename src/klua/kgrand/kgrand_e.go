///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgrand_e.go
/// @author	lishaoliang
/// @brief	kgrand export
///////////////////////////////////////////////////////////////////////////

package kgrand

/*
#include "klua/klua.h"
extern int klua_go_open_kg_rand_cgo(lua_State* L);
*/
import "C"
import (
	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_rand
func klua_go_open_kg_rand(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_rand_cgo(lua)

	return ret
}

//export klua_go_kg_rand_str
func klua_go_kg_rand_str(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	n := int64(8)
	if klua.LuaIsinteger(ls, 1) {
		n = klua.LuaTointeger(ls, 1) // @1 字符串长度
	}

	s := gKgRand.Str64B(n)

	klua.LuaPushstringB(ls, s)
	return 1
}

//export klua_go_kg_rand_str_num
func klua_go_kg_rand_str_num(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	n := int64(8)
	if klua.LuaIsinteger(ls, 1) {
		n = klua.LuaTointeger(ls, 1) // @1 字符串长度
	}

	s := gKgRand.StrNum64B(n)

	klua.LuaPushstringB(ls, s)
	return 1
}
