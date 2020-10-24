///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2020, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgctx.go
/// @author	lishaoliang
/// @brief	kg_ctx
///////////////////////////////////////////////////////////////////////////

package kgctx

/*
//#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
//#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_ctx_get_preload(lua_State* L);
extern int klua_go_kg_ctx_get_loader(lua_State* L);

int klua_go_open_kg_ctx_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "get_preload",	klua_go_kg_ctx_get_preload },
		{ "get_loader",		klua_go_kg_ctx_get_loader },

		{ NULL,				NULL }
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

// OpenKgCtx kg_ctx
func OpenKgCtx(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_ctx_cgo)), name)
}
