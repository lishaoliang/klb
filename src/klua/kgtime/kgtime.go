///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgtime.go
/// @author	lishaoliang
/// @brief	kg_time
///////////////////////////////////////////////////////////////////////////

package kgtime

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_time_sleep(lua_State* L);
extern int klua_go_kg_time_sleep_ns(lua_State* L);

int klua_go_open_kg_time_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "sleep",		klua_go_kg_time_sleep },
		{ "sleep_ns",	klua_go_kg_time_sleep_ns },

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

// OpenKgTime kg_time
func OpenKgTime(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_time_cgo)), name)
}
