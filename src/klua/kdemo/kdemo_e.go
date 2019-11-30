///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kdemo.go
/// @author lishaoliang
/// @brief	kdemo export
///////////////////////////////////////////////////////////////////////////

package kdemo

/*
#include "klua/klua.h"
extern int klua_go_open_kd_demo_cgo(lua_State* L);
*/
import "C"
import (
	"fmt"

	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kd_demo
func klua_go_open_kd_demo(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kd_demo_cgo(lua)

	return ret
}

//export klua_go_kd_demo_hello
func klua_go_kd_demo_hello(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	klua.LuaPushstring(l, "hello kdemo")
	klua.LuaPushboolean(l, true)

	return 2
}

//export klua_go_kd_demo_test
func klua_go_kd_demo_test(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	a1 := klua.LuaLCheckstring(l, 1)
	a2 := klua.LuaLCheckstring(l, 2)

	fmt.Println("go:", a1, a2)

	klua.LuaPushstring(l, a1)
	klua.LuaPushstring(l, a2)

	return 2
}
