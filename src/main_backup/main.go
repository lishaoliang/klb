///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    main.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   main (cgo encode gb2312)
///////////////////////////////////////////////////////////////////////////
package main

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lrt -lm
#include "klua/klua.h"
*/
import "C"
import "fmt"


// cgo: encode gb2312
func main() {
    var env *C.klua_env_t = C.klua_env_create()

    C.klua_env_dolibrary(env, C.CString("aaa"))

    C.klua_env_destroy(env)
}


//export klua_go_openlibs
func klua_go_openlibs(L *C.lua_State) C.int {
    fmt.Println("*C.lua_State:", L)

    OpenTest(L)

    return 0
}

//export klua_go_main
func klua_go_main(argc C.int, argv **C.char) C.int {
    //fmt.Println(argc, argv)

    //var ret = C.klua_main(argc, argv, nil)

    var env *C.klua_env_t = C.klua_env_create()

    C.klua_env_dolibrary(env, C.CString("aaa"))

    C.klua_env_destroy(env)

    return 0
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

/*
#cgo linux CFLAGS: -std=c99 -D__LINUX__ -I ../../inc
#cgo linux LDFLAGS: -L ../../klib/release/lib_arm_linux -lklb_c -lpthread -lrt -lm
#cgo arm CFLAGS: -std=c99 -D__LINUX__ -I ../../inc
#cgo arm LDFLAGS: -L ../../klib/release/lib_arm_linux -lklb_c -lpthread -lrt -lm
#include <stdio.h>
#include <stdlib.h>
*/
