///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgrand.go
/// @author	lishaoliang
/// @brief	kgrand
///////////////////////////////////////////////////////////////////////////

package kgrand

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_rand_str(lua_State* L);
extern int klua_go_kg_rand_str_num(lua_State* L);

int klua_go_open_kg_rand_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "str",		klua_go_kg_rand_str },
		{ "str_num",	klua_go_kg_rand_str_num },

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
	"github.com/lishaoliang/klb/src/kutil"
)

var gKgRand *kutil.Rand

// init
func init() {
	kgRandInit()
}

func kgRandInit() {
	gKgRand = kutil.NewRand()
}

// OpenKgRand kg_rand
func OpenKgRand(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_rand_cgo)), name)
}
