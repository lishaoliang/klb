///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kgstream_e.go
/// @author lishaoliang
/// @brief	kgstream e
///////////////////////////////////////////////////////////////////////////

package kgstream

/*
#include "klua/klua.h"
#include "mnp/klb_mnp.h"
#include "string.h"

extern int klua_go_open_kg_stream_cgo(lua_State* L);
extern klb_mnp_stream_cb g_cb_klua_go_kg_stream;
*/
import "C"
import "github.com/lishaoliang/klb/src/klua"

var gStream *stream

// exprot func

//export klua_go_open_kg_stream
func klua_go_open_kg_stream(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_stream_cgo(lua)

	return ret
}

//export klua_go_kg_stream_init
func klua_go_kg_stream_init(lua *C.lua_State) C.int {

	gStream = streamCreate("./test1.h264", "./test1.h264")

	return 0
}

//export klua_go_kg_stream_quit
func klua_go_kg_stream_quit(lua *C.lua_State) C.int {

	gStream.Destroy()
	gStream = nil

	return 0
}

//export klua_go_kg_stream_start
func klua_go_kg_stream_start(lua *C.lua_State) C.int {

	gStream.Start()

	return 0
}

//export klua_go_kg_stream_stop
func klua_go_kg_stream_stop(lua *C.lua_State) C.int {

	gStream.Stop()

	return 0
}

//export klua_go_kg_stream_set_stream_cb
func klua_go_kg_stream_set_stream_cb(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ptr := klua.LuaLChecklightuserdata(l, 1)
	C.g_cb_klua_go_kg_stream = (C.klb_mnp_stream_cb)(ptr)

	return 0
}
