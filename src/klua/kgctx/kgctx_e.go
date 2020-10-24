///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgctx_e.go
/// @author	lishaoliang
/// @brief	kg_ctx export
///////////////////////////////////////////////////////////////////////////

package kgctx

/*
#include "klua/klua.h"
extern int klua_go_open_kg_ctx_cgo(lua_State* L);
*/
import "C"
import (
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_ctx
func klua_go_open_kg_ctx(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_ctx_cgo(lua)

	return ret
}

//export klua_go_kg_ctx_get_preload
func klua_go_kg_ctx_get_preload(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1) // @1 名称

	var ptr klua.LuaCFunction = klua.GetPreload(name)

	klua.LuaPushlightuserdata(l, unsafe.Pointer(ptr)) // #1 预加载函数的C指针
	return 1
}

//export klua_go_kg_ctx_get_loader
func klua_go_kg_ctx_get_loader(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1) // @1 名称

	var ptr klua.LuaCFunction = klua.GetLoader(name)

	klua.LuaPushlightuserdata(l, unsafe.Pointer(ptr)) // #1  放入自定义加载lua文件函数的C指针
	return 1
}
