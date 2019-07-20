package main

/*
#include "klua/klua.h"

extern int klua_gothread_hello(lua_State* L);
extern int klua_gothread_create(lua_State* L);
extern int klua_gothread_destroy(lua_State* L);

int klua_pre_open_gothread_cgo(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "hello",        klua_gothread_hello },

        { "create",        klua_gothread_create },
        { "destroy",    klua_gothread_destroy },

        { NULL,            NULL }
    };

    luaL_newlib(L, lib);
    return 1;
}

*/
import "C"
import "unsafe"

func kluaPreOpenGoThread(L *C.lua_State) C.int {
    C.klua_pre_loadlib(L, (C.lua_CFunction)(unsafe.Pointer(C.klua_pre_open_gothread_cgo)), C.CString("gothread"))
    return 1
}
