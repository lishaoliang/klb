package main

/*
#include "klua/klua.h"
*/
import "C"
import (
    "fmt"
    "time"
)

func say(s string) {
    for i := 0; i < 5; i++ {
        time.Sleep(100 * time.Millisecond)
        fmt.Println(s)
    }
}

//export klua_gothread_hello
func klua_gothread_hello(L *C.lua_State) C.int {

    go say("Hello go thread!")

    return 0
}

//export klua_gothread_create
func klua_gothread_create(L *C.lua_State) C.int {
    var pName = C.luaL_checklstring(L, C.int(1), nil)
    goWorkerKlua(C.GoString(pName))

    return 0
}

//export klua_gothread_destroy
func klua_gothread_destroy(L *C.lua_State) C.int {

    return 0
}
