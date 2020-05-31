///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   main.go
/// @author lishaoliang
/// @brief	main
///////////////////////////////////////////////////////////////////////////

package main

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../inc
#include "klua/klua.h"
extern int klua_go_main_openlibs(lua_State* L);
*/
import "C"
import (
	"fmt"
	"os"
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

func usage() {
	fmt.Println("usage")
	fmt.Println("  klua ./xxx.lua")
}

// init
func init() {
	//fmt.Println("main init!")
}

// main
func main() {

	args := os.Args

	n := len(args)
	if n < 2 {
		usage()
		return
	}

	kpool.Init(4096, 3072)

	c := make(chan os.Signal)

	// 通过编译 windows/linux
	//signal.Notify(c, os.Interrupt, os.Kill, syscall.SIGINT, syscall.SIGTERM, syscall.SIGTSTP)
	signalNotify(c)

	klua.PushPreload("klua_go_main_openlibs", (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_main_openlibs)))
	names := make([]string, 0)

	for i := 1; i < n; i++ {
		path := args[i]

		_, err := klua.Open(path, path, "klua_go_main_openlibs", "", true)
		if nil != err {
			fmt.Println("main", err)
		} else {
			names = append(names, path)
		}
	}

	s := <-c
	//fmt.Println("main over.", s)

	for i := len(names) - 1; i >= 0; i-- {
		//fmt.Println("main close:", names[i], i)
		klua.Close(names[i])
	}

	//klua.Cancel()
	klua.Wait()

	kpool.Quit()

	fmt.Println("main over...", s)
}
