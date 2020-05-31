package kgsimulate

/*
#include "klua/klua.h"
extern int klua_go_open_kg_simulate_cgo(lua_State* L);
*/
import "C"
import (
	"github.com/lishaoliang/klb/src/kframework/ksimulate"
	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_simulate
func klua_go_open_kg_simulate(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_simulate_cgo(lua)

	return ret
}

//export klua_go_kg_simulate_init
func klua_go_kg_simulate_init(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	cfg := klua.LuaLCheckstring(l, 1) // @1 初始配置

	//fmt.Println("kgsimulate.klua_go_kg_simulate_init", cfg)
	err := ksimulate.Init(cfg)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_simulate_quit
func klua_go_kg_simulate_quit(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	ksimulate.Quit()

	return 0
}

//export klua_go_kg_simulate_start
func klua_go_kg_simulate_start(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	err := ksimulate.Start()

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_simulate_stop
func klua_go_kg_simulate_stop(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	ksimulate.Stop()

	return 0
}

//export klua_go_kg_simulate_set_stream_receiver
func klua_go_kg_simulate_set_stream_receiver(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ptr := klua.LuaLChecklightuserdata(l, 1)

	//fmt.Println("kgsimulate.klua_go_kg_simulate_set_stream_receiver", ptr)
	ksimulate.SetStreamReceiver(ptr)

	return 0
}

//export klua_go_kg_simulate_command
func klua_go_kg_simulate_command(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	cmd := klua.LuaLCheckstring(l, 1)    // @1 命令名称
	chnn := klua.LuaLCheckinteger(l, 2)  // @2 通道
	sidx := klua.LuaLCheckinteger(l, 3)  // @3 流序号
	lparam := klua.LuaLCheckstring(l, 4) // @4 参数1
	wparam := klua.LuaLCheckstring(l, 5) // @5 参数2

	//fmt.Println("kgsimulate.klua_go_kg_simulate_command", cmd, chnn, sidx, lparam, wparam)
	ret, err := ksimulate.Command(cmd, chnn, sidx, lparam, wparam)

	klua.LuaPushinteger(l, ret) // #1 错误码
	if nil != err {
		klua.LuaPushstring(l, err.Error()) // #2 错误字符串
	} else {
		klua.LuaPushstring(l, "") // #2 错误字符串
	}

	return 2
}
