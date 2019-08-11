///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/06/30
//
/// @file    main.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   main (cgo must encode ASCII or build error!)
///////////////////////////////////////////////////////////////////////////
package main

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
#include "klua/klua.h"
extern int klua_go_main_openlibs(lua_State* L);
*/
import "C"
import (
	"fmt"
	"os"
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
)

func usage() {
	fmt.Println("usage")
	fmt.Println("  klua ./xxx.lua")
}

// main
func main() {

	args := os.Args

	n := len(args)
	if n < 2 {
		usage()
		return
	}

	for i := 1; i < n; i++ {
		path := args[i]

		ctx := klua.CtxCreate((klua.LuaCFunction)(unsafe.Pointer(C.klua_go_main_openlibs)))
		defer ctx.Destroy()

		ctx.WorkerDoFile("", path)
	}

	klua.CtxWait()
}

// init
func init() {
	//fmt.Println("main init!")
}
