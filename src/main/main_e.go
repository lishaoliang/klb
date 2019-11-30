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

	"github.com/lishaoliang/klb/src/kframe/kamdc"
	"github.com/lishaoliang/klb/src/kgstream"
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/klua/kdemo"
	"github.com/lishaoliang/klb/src/klua/kgos"
	"github.com/lishaoliang/klb/src/klua/kgrand"
	"github.com/lishaoliang/klb/src/klua/kgthread"
	"github.com/lishaoliang/klb/src/klua/kgtime"
)

//export klua_go_main_openlibs
func klua_go_main_openlibs(lua *C.lua_State) C.int {

	l := (*klua.LuaState)(unsafe.Pointer(lua))

	klua.OpenKos(l, "kos")
	klua.OpenKtime(l, "ktime")
	klua.OpenKthread(l, "kthread")

	kgos.OpenKgOs(l, "kg_os")
	kgtime.OpenKgTime(l, "kg_time")
	kgrand.OpenKgRand(l, "kg_rand")
	kgthread.OpenKgThread(l, "kg_thread")

	kdemo.OpenKdDemo(l, "kd_demo")

	kamdc.OpenKaMdc(l, "ka_mdc")

	kgstream.OpenKgStream(l, "kg_stream")

	return 0
}
