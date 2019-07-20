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
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
#include "klua/klua.h"

extern int klua_pre_go_openlibs(lua_State* L);

int klua_pre_go_openlibs_cgo(lua_State* L)
{
    klua_pre_go_openlibs(L);
    return 0;
}

*/
import "C"
import (
    "context"
    "fmt"
    "os"
    "sync"
    "time"
    "unsafe"
)

func usage() {
    fmt.Println("usage")
    fmt.Println("  klua ./xxx.lua")
}

func worker(ctx context.Context, wg *sync.WaitGroup, name string) error {
    var pEnv *C.klua_env_t = C.klua_env_create((C.lua_CFunction)(unsafe.Pointer(C.klua_pre_go_openlibs_cgo)), C.CString(name))

    defer C.klua_env_destroy(pEnv)
    defer wg.Done()

    for {
        select {
        default:
            //fmt.Println("hello start")
            C.klua_env_call_kgo(pEnv)
            //fmt.Println("hello end...", status)
            return ctx.Err()
        case <-ctx.Done():
            return ctx.Err()
        }
    }
}

var gCtx context.Context
var gWg sync.WaitGroup

func goWorkerKlua(name string) {
    gWg.Add(1)
    go worker(gCtx, &gWg, name)
}

func main() {

    args := os.Args

    if len(args) < 2 {
        usage()
        return
    }

    name := args[1] // load lua file: ***.lua

    // help cn: https://books.studygolang.com/advanced-go-programming-book/ch1-basic/ch1-06-goroutine.html

    //c := make(chan os.Signal)
    //signal.Notify(c, os.Interrupt, os.Kill)

    var cancel context.CancelFunc
    gCtx, cancel = context.WithTimeout(context.Background(), 6*time.Second)

    //var wg sync.WaitGroup

    //for i := 0; i < 1; i++ {
    //    wg.Add(1)
    //    go worker(ctx, &wg, name)
    //}
    goWorkerKlua(name)

    //time.Sleep(3 * time.Second)

    //s := <-c
    //fmt.Println("signal quit : ", s)

    //cancel()
    gWg.Wait()
    cancel()
}
