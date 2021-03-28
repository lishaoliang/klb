///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   main.go
/// @author lishaoliang
/// @brief	main
///////////////////////////////////////////////////////////////////////////

package main

/*
//#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../inc
#include "klua/klua.h"
#include "klbbase/klb_base.h"
extern int klua_go_main_openlibs(lua_State* L);
*/
import "C"
import (
	"fmt"
	"os"
	"path"
	"strings"
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

func usage() {
	fmt.Println("usage")
	fmt.Println("  klb ./xxx.lua")
	fmt.Println("  klb server")
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

	C.klb_base_init(nil)

	kpool.Init(4096, 3072*4)

	c := make(chan os.Signal)

	// 通过编译 windows/linux
	//signal.Notify(c, os.Interrupt, os.Kill, syscall.SIGINT, syscall.SIGTERM, syscall.SIGTSTP)
	signalNotify(c)

	klua.PushPreload("klua_go_main_openlibs", (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_main_openlibs)))
	names := make([]string, 0)

	for i := 1; i < n; i++ {
		strPath := args[i]

		bDoFile := false
		ext := strings.ToLower(path.Ext(strPath))
		if 0 <= strings.LastIndex(ext, "lua") {
			bDoFile = true // 从相对路径加载
		}

		_, err := klua.Open(strPath, strPath, "klua_go_main_openlibs", "", bDoFile)

		if nil != err {
			fmt.Println("main", err)
		} else {
			names = append(names, strPath)
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

	C.klb_base_quit()

	fmt.Println("main over...", s)
}
