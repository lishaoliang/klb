///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    kgtime_e.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kg_time export
///////////////////////////////////////////////////////////////////////////
package kgtime

/*
#include "klua/klua.h"
extern int klua_go_open_kg_time_cgo(lua_State* L);
*/
import "C"
import (
	"time"

	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_time
func klua_go_open_kg_time(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_time_cgo(lua)

	return ret
}

//export klua_go_kg_time_sleep
func klua_go_kg_time_sleep(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ms := klua.LuaLCheckinteger(l, 1) // @1 sleep ms

	time.Sleep((time.Duration)(time.Millisecond) * (time.Duration)(ms))
	return 0
}

//export klua_go_kg_time_sleep_ns
func klua_go_kg_time_sleep_ns(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ns := klua.LuaLCheckinteger(l, 1) // @1 sleep ns

	time.Sleep((time.Duration)(time.Nanosecond) * (time.Duration)(ns))
	return 0
}
