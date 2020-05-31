///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgthread.go
/// @author	lishaoliang
/// @brief	kg_thread export
///////////////////////////////////////////////////////////////////////////

package kgthread

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_thread_create(lua_State* L);
extern int klua_go_kg_thread_destroy(lua_State* L);
extern int klua_go_kg_thread_post(lua_State* L);

int klua_go_open_kg_thread_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "create",		klua_go_kg_thread_create },
		{ "destroy",	klua_go_kg_thread_destroy },

		{ "post",		klua_go_kg_thread_post },

		{ NULL,			NULL }
	};

	luaL_newlib(L, lib);
	return 1;
}
*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
)

// OpenKgThread kg_thread
func OpenKgThread(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_thread_cgo)), name)
}
