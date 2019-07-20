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

extern int klua_go_test_hello();
extern int klua_go_test_func(lua_State* L);

static int lua_hello(lua_State* L)
{
    printf("Hello go!\n");
    klua_go_test_hello();

    return 0;
}

int luaopen_test(lua_State* L)
{
    static luaL_Reg lib[] =
    {
        { "hello",    lua_hello },
        { "func",    klua_go_test_func },

        { NULL,     NULL }
    };

    // 创建导出库函数
    luaL_newlib(L, lib);

    return 1;
}
*/
import "C"

import (
    "unsafe"
)

func OpenTest(L *C.lua_State) C.int {
    C.klua_pre_loadlib(L, (C.lua_CFunction)(unsafe.Pointer(C.luaopen_test)), C.CString("test"))
    return 1
}
