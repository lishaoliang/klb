///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgthread_e.go
/// @author	lishaoliang
/// @brief	kg_thread export
///////////////////////////////////////////////////////////////////////////

package kgthread

/*
#include "klua/klua.h"
extern int klua_go_open_kg_thread_cgo(lua_State* L);
*/
import "C"
import (
	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_thread
func klua_go_open_kg_thread(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_thread_cgo(lua)

	return ret
}

//export klua_go_kg_thread_create
func klua_go_kg_thread_create(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1)    // @1 线程名称,thread name
	entry := klua.LuaLCheckstring(ls, 2)   // @2 入口脚本,entry
	preload := klua.LuaLCheckstring(ls, 3) // @3 预加载函数名
	loader := klua.LuaLCheckstring(ls, 4)  // @4 自定义加载函数名

	ctx, _ := klua.Open(name, entry, preload, loader, false)

	if nil != ctx {
		klua.LuaPushboolean(ls, true) // #1 true
	} else {
		klua.LuaPushboolean(ls, false) // #1 false
	}

	return 1
}

//export klua_go_kg_thread_destroy
func klua_go_kg_thread_destroy(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1) // @1 thread name

	ret := klua.Close(name)

	klua.LuaPushboolean(ls, ret) // #1 true,false
	return 1
}

//export klua_go_kg_thread_post
func klua_go_kg_thread_post(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1)    // @1 thread name
	msg := klua.LuaLCheckstringB(ls, 2)    // @2 message
	msgex := klua.LuaLCheckstringB(ls, 3)  // @3 message ex
	lparam := klua.LuaLCheckstringB(ls, 4) // @4 l param
	wparam := klua.LuaLCheckstringB(ls, 5) // @5 w param

	ret := klua.Post(name, msg, msgex, lparam, wparam)

	klua.LuaPushboolean(ls, ret) // #1 true,false
	return 1
}
