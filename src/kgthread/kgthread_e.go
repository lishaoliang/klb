///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    kgthread_e.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
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

	name := klua.LuaLCheckstring(ls, 1)   // @1 thread name
	loader := klua.LuaLCheckstring(ls, 2) // @2 main loader in *.lua

	env := klua.EnvGetByLua(ls)
	ctx := klua.CtxFindByEnv(env)

	b, _ := ctx.WorkerDoLibrary(name, loader)

	klua.LuaPushboolean(ls, b) // #1 true,false
	return 1
}

//export klua_go_kg_thread_destroy
func klua_go_kg_thread_destroy(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1) // @1 thread name

	b := klua.CtxClose(name)

	klua.LuaPushboolean(ls, b) // #1 true,false
	return 1
}

//export klua_go_kg_thread_post
func klua_go_kg_thread_post(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1)   // @1 thread name
	msg := klua.LuaLCheckstring(ls, 2)    // @2 message
	lparam := klua.LuaLCheckstring(ls, 3) // @3 l param
	wparam := klua.LuaLCheckstring(ls, 4) // @4 w param
	//var ptr =                          // @5 ptr void*

	b := klua.CtxPost(name, msg, lparam, wparam, nil)

	klua.LuaPushboolean(ls, b) // #1 true,false
	return 1
}
