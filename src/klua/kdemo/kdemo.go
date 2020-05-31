///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kdemo.go
/// @author lishaoliang
/// @brief	kdemo
///////////////////////////////////////////////////////////////////////////

package kdemo

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kd_demo_hello(lua_State* L);
extern int klua_go_kd_demo_test(lua_State* L);

int klua_go_open_kd_demo_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "hello",	klua_go_kd_demo_hello },
		{ "test",	klua_go_kd_demo_test },

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

// OpenKdDemo kd_demo
func OpenKdDemo(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kd_demo_cgo)), name)
}
