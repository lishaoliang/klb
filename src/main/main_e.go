package main

/*
#include "klua/klua.h"
*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/kanet"
	"github.com/lishaoliang/klb/src/kdemo"
	"github.com/lishaoliang/klb/src/kgos"
	"github.com/lishaoliang/klb/src/kgrand"
	"github.com/lishaoliang/klb/src/kgthread"
	"github.com/lishaoliang/klb/src/kgtime"
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kxnet"
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

	kanet.OpenKaNet(l, "ka_net")
	kxnet.OpenKxNet(l, "kx_net")

	kdemo.OpenKdDemo(l, "kd_demo")

	return 0
}
