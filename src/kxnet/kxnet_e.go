///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    kxnet_e.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kxnet export
///////////////////////////////////////////////////////////////////////////
package kxnet

/*
#include "klua/klua.h"
extern int klua_go_open_kx_net_cgo(lua_State* L);
*/
import "C"
import "fmt"

// exprot func

//export klua_go_open_kx_net
func klua_go_open_kx_net(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kx_net_cgo(lua)

	return ret
}

//export klua_go_kx_net_hello
func klua_go_kx_net_hello(lua *C.lua_State) C.int {

	fmt.Println("hello kx_net!")

	return 0
}
