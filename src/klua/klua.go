///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	klua.go
/// @author	lishaoliang
/// @brief	klua
///////////////////////////////////////////////////////////////////////////

package klua

/*
//#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../inc
//#cgo windows LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -lm
//#cgo linux LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
//#cgo darwin LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -ldl -lm
#include "stdlib.h"
#include "klua/klua.h"
*/
import "C"
import (
	"unsafe"
)

// init
func init() {
	// init map func, in map_funcs.go
	mapFuncsInit()

	// init ctxMan, in ctx_man.go
	ctxManInit()
}

// PushPreload push preload
// 放入命名预加载函数[多线程安全]
// 在lua环境中,预加载库
func PushPreload(name string, preload LuaCFunction) bool {
	return gMapFuncs.PushPreload(name, preload)
}

// GetPreload get preload
// 获取命名预加载函数[多线程安全]
// 在lua环境中,预加载库
func GetPreload(name string) LuaCFunction {
	return gMapFuncs.FindPreload(name)
}

// PushLoader push loader
// 放入自定义加载lua文件函数[多线程安全]
// 更换lua从文件中加载脚本文件方式, 修改为使用自定义的函数
func PushLoader(name string, loader LuaCFunction) bool {
	return gMapFuncs.PushLoader(name, loader)
}

// GetLoader push loader
// 获取自定义加载lua文件函数[多线程安全]
// 更换lua从文件中加载脚本文件方式, 修改为使用自定义的函数
func GetLoader(name string) LuaCFunction {
	return gMapFuncs.FindLoader(name)
}

// Ctx func

// Wait wait for done
// 等待所有Lua线程[多线程安全]
func Wait() {
	gCtxMan.Wait()
}

// Cancel cancel
// 取消执行[多线程安全]
func Cancel() {
	gCtxMan.Cancel()
}

// Open open
// 打开一个新的Lua线程[多线程安全]
// @name	线程名称
// @entry	入口脚本; 例如, Dofile方式: "./demo/demo.lua", DoLibrary方式: "demo/demo"
// @preload	预先加载库函数名称
// @loader	加载器函数名称; ""表示使用从文件加载
// @bDoFile true.DoFile方式; false.DoLibrary方式;
// #*Ctx	Lua线程环境
// #error	错误
func Open(name, entry, preload, loader string, bDoFile bool) (*Ctx, error) {
	return gCtxMan.Open(name, entry, preload, loader, bDoFile)
}

// Close close
// 关闭某个Lua线程[多线程安全]
func Close(name string) bool {
	return gCtxMan.Close(name)
}

// Post post msg
// 向某个Lua线程post消息[多线程安全]
// 返回值true.仅仅表示将消息投递到对方Channel中
func Post(name string, msg, msgex, lparam, wparam []byte) bool {
	return gCtxMan.Post(name, msg, msgex, lparam, wparam)
}

// LoadLib load lib
// 在Lua环境中加载库[Lua线程]
func LoadLib(lua *LuaState, f LuaCFunction, name string) {
	cStr := C.CString(name)
	defer C.free(unsafe.Pointer(cStr))

	C.klua_loadlib((*C.lua_State)(unsafe.Pointer(lua)), f, cStr)
}

// Open C libs
// 从C部分导出的第三方开源库

// OpenCjson cjson
// 在Lua环境中加载"cjson"库[Lua线程]
func OpenCjson(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_cjson)), name)
}

// OpenCjsonSafe cjson safe
// 在Lua环境中加载"cjson.safe"库[Lua线程]
func OpenCjsonSafe(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_cjson_safe)), name)
}

// OpenLpeg lpeg
// 在Lua环境中加载"lpeg"库[Lua线程]
func OpenLpeg(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_lpeg)), name)
}

// OpenLfs lfs
// 在Lua环境中加载"lfs"库[Lua线程]
func OpenLfs(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_lfs)), name)
}

// eg. "k*"
// 从C部分导出的库, 命名记为 "k*"

// OpenKos kos
// 在Lua环境中加载"kos"库[Lua线程]
func OpenKos(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kos)), name)
}

// OpenKtime ktime
// 在Lua环境中加载"ktime"库[Lua线程]
func OpenKtime(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_ktime)), name)
}

// OpenKthread kthread
// 在Lua环境中加载"kthread"库[Lua线程]
func OpenKthread(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kthread)), name)
}

// OpenKmnpDev kmnp_dev
// 在Lua环境中加载"kmnp_dev"库[Lua线程]
func OpenKmnpDev(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kmnp_dev)), name)
}

// OpenKgui kgui
// 在Lua环境中加载"kgui"库
func OpenKgui(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kgui)), name)
}

// OpenKwnd kwnd
// 在Lua环境中加载"kwnd"库
func OpenKwnd(lua *LuaState, name string) {
	LoadLib(lua, (LuaCFunction)(unsafe.Pointer(C.klua_open_kwnd)), name)
}

// Open Go/C libs
// eg. "k*", "kg_*"
// "k*" 表示为c语言编写的库
// "kg_*" 表示为go语言编写的库
