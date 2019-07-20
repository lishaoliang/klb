package main

/*
#include "klua/klua.h"
*/
import "C"

//export klua_pre_go_openlibs
func klua_pre_go_openlibs(L *C.lua_State) C.int {

    kluaPreOpenGoThread(L)

    return 0
}
