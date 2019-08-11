///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    kanet.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kanet export
///////////////////////////////////////////////////////////////////////////
package kanet

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
#include "klua/klua.h"

extern int klua_go_ka_net_hello(lua_State* L);

int klua_go_open_ka_net_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "hello",	klua_go_ka_net_hello },
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

// OpenKaNet ka_net
func OpenKaNet(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_ka_net_cgo)), name)
}
