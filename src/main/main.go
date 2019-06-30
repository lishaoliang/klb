///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//
/// @file    main.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief   main cgo encode gb2312
///////////////////////////////////////////////////////////////////////////
package main

//#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../inc
//#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lrt -lm
//#include "klua/klua.h"
import "C"

// cgo: encode gb2312
func main() {

}

//export klua_go_main
func klua_go_main(argc C.int, argv **C.char) C.int {
	//fmt.Println(argc, argv)

	var ret = C.lua_main(argc, argv)

	return ret
}

/*
#cgo linux CFLAGS: -std=c99 -D__LINUX__ -I ../../inc
#cgo linux LDFLAGS: -L ../../klib/release/lib_arm_linux -lklb_c -lpthread -lrt -lm
#cgo arm CFLAGS: -std=c99 -D__LINUX__ -I ../../inc
#cgo arm LDFLAGS: -L ../../klib/release/lib_arm_linux -lklb_c -lpthread -lrt -lm
#include <stdio.h>
#include <stdlib.h>
#include "hash/klb_hash_dx.h"
*/
