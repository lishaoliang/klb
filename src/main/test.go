///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    thread.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   test (cgo encode gb2312)
///////////////////////////////////////////////////////////////////////////
package main

/*
#include "klua/klua.h"
*/
import "C"

import (
    "fmt"
    "time"
)

func Say(s string) {
    for i := 0; i < 5; i++ {
        time.Sleep(100 * time.Millisecond)
        fmt.Println(s)
    }
}

//export klua_go_test_hello
func klua_go_test_hello() C.int {

    go Say("world")
    Say("hello")

    return 0
}

//export klua_go_test_func
func klua_go_test_func(L *C.lua_State) C.int {

    go Say("func,world")
    Say("func,hello")

    return 0
}