///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgcrypto.go
/// @author	lishaoliang
/// @brief	kg_time
///////////////////////////////////////////////////////////////////////////

package kgcrypto

/*
//#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
//#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_crypto_sleep(lua_State* L);
extern int klua_go_kg_crypto_sleep_ns(lua_State* L);

extern int klua_go_kg_crypto_ws_encode(lua_State* L);

int klua_go_open_kg_crypto_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "sleep",		klua_go_kg_crypto_sleep },
		{ "sleep_ns",	klua_go_kg_crypto_sleep_ns },

		{ "ws_encode",	klua_go_kg_crypto_ws_encode },

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

// OpenKgCrypto kg_crypto
func OpenKgCrypto(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_crypto_cgo)), name)
}
