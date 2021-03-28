///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   main_e.go
/// @author lishaoliang
/// @brief	main export
///////////////////////////////////////////////////////////////////////////

package main

/*
#include "klua/klua.h"
*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/kframework/klsmfw/kglsmfw"
	"github.com/lishaoliang/klb/src/kframework/ksimulate/kgsimulate"
	"github.com/lishaoliang/klb/src/kframework/ksmcfw/kgsmcfw"
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/klua/kdemo"
	"github.com/lishaoliang/klb/src/klua/kgcrypto"
	"github.com/lishaoliang/klb/src/klua/kgctx"
	"github.com/lishaoliang/klb/src/klua/kgos"
	"github.com/lishaoliang/klb/src/klua/kgrand"
	"github.com/lishaoliang/klb/src/klua/kgthread"
	"github.com/lishaoliang/klb/src/klua/kgtime"
)

//export klua_go_main_openlibs
func klua_go_main_openlibs(lua *C.lua_State) C.int {

	l := (*klua.LuaState)(unsafe.Pointer(lua))

	klua.LoadLibAll(l)

	kgctx.OpenKgCtx(l, "kg_ctx")
	kgos.OpenKgOs(l, "kg_os")
	kgtime.OpenKgTime(l, "kg_time")
	kgrand.OpenKgRand(l, "kg_rand")
	kgcrypto.OpenKgCrypto(l, "kg_crypto")
	kgthread.OpenKgThread(l, "kg_thread")

	kglsmfw.OpenKgLsmfw(l, "kg_lsmfw")
	kgsimulate.OpenKgSimulate(l, "kg_simulate")
	kgsmcfw.OpenKgSmcfw(l, "kg_smcfw")

	kdemo.OpenKdDemo(l, "kd_demo")

	return 0
}
