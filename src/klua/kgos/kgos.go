///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kgos.go
/// @author lishaoliang
/// @brief	kgos
///////////////////////////////////////////////////////////////////////////

package kgos

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_os_shell(lua_State* L);

int klua_go_open_kg_os_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "shell",	klua_go_kg_os_shell },

		{ NULL,		NULL }
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

// OpenKgOs kg_os
func OpenKgOs(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_os_cgo)), name)
}
