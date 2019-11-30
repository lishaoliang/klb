///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kamdc_e.go
/// @author lishaoliang
/// @brief	kamdc e
///////////////////////////////////////////////////////////////////////////

package kamdc

/*
#include "klua/klua.h"
#include "mnp/klb_mnp.h"

extern int klua_go_open_ka_mdc_cgo(lua_State* L);
*/
import "C"
import (
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

// exprot func

//export klua_go_open_ka_mdc
func klua_go_open_ka_mdc(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_ka_mdc_cgo(lua)

	return ret
}

//export klua_go_ka_mdc_init
func klua_go_ka_mdc_init(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	ret := mdcInit()

	klua.LuaPushinteger(ls, int64(ret))
	return 1
}

//export klua_go_ka_mdc_quit
func klua_go_ka_mdc_quit(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	ret := mdcQuit()

	klua.LuaPushinteger(ls, int64(ret))
	return 1
}

//export klua_go_ka_mdc_start
func klua_go_ka_mdc_start(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1)    // @1 线程名称thread name
	entry := klua.LuaLCheckstring(ls, 2)   // @2 脚本入口
	preload := klua.LuaLCheckstring(ls, 3) // @3 预加载函数名称
	loader := klua.LuaLCheckstring(ls, 4)  // @4 加载脚本函数名称

	ret := mdcStart(name, entry, preload, loader)

	klua.LuaPushinteger(ls, int64(ret))
	return 1
}

//export klua_go_ka_mdc_stop
func klua_go_ka_mdc_stop(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	ret := mdcStop()

	klua.LuaPushinteger(ls, int64(ret))
	return 1
}

//export klua_go_ka_mdc_send
func klua_go_ka_mdc_send(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(ls, 1)              // @1 name
	sequence := klua.LuaLCheckinteger(ls, 2)         // @2 sequence
	uid := klua.LuaLCheckinteger(ls, 3)              // @3 uid
	pExtra, lExtra := klua.LuaLCheckstringPtr(ls, 4) // @4 extra
	pData, lData := klua.LuaLCheckstringPtr(ls, 5)   // @5 data

	extra := make([]byte, lExtra)
	kutil.Memcpy(extra, pExtra, lExtra)

	data := make([]byte, lData)
	kutil.Memcpy(data, pData, lData)

	ret := gSerMan.SendTxt("", name, sequence, uid, extra, data)

	klua.LuaPushboolean(ls, ret) // #1 true,false
	return 1
}

//export klua_go_ka_mdc_open_stream
func klua_go_ka_mdc_open_stream(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)
	name := klua.LuaLCheckstring(ls, 1)  // @1 name
	chnn := klua.LuaLCheckinteger(ls, 2) // @2 chnn
	sidx := klua.LuaLCheckinteger(ls, 3) // @3 sidx

	ret := gSerMan.OpenStream("", name, int(chnn), int(sidx))

	klua.LuaPushboolean(ls, ret) // #1 true,false
	return 1
}

//export klua_go_ka_mdc_close_stream
func klua_go_ka_mdc_close_stream(lua *C.lua_State) C.int {
	ls := (*klua.LuaState)(lua)
	name := klua.LuaLCheckstring(ls, 1)  // @1 name
	chnn := klua.LuaLCheckinteger(ls, 2) // @2 chnn
	sidx := klua.LuaLCheckinteger(ls, 3) // @3 sidx

	ret := gSerMan.CloseStream("", name, int(chnn), int(sidx))

	klua.LuaPushboolean(ls, ret) // #1 true,false
	return 1
}

//export cb_klua_go_ka_mdc_push_stream
func cb_klua_go_ka_mdc_push_stream(chnn, sidx C.int, stream *C.klb_mnp_stream_t) C.int {
	ms := kmnp.MnpStreamCreate((*kmnp.KlbMnpStream)(unsafe.Pointer(stream)))

	gSerMan.PushStream(int(chnn), int(sidx), ms)

	return 0
}
