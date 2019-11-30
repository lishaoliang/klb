///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kgstream.go
/// @author lishaoliang
/// @brief	kgstream
///////////////////////////////////////////////////////////////////////////

package kgstream

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"
#include "mnp/klb_mnp.h"
#include "string.h"

extern int klua_go_kg_stream_init(lua_State* L);
extern int klua_go_kg_stream_quit(lua_State* L);
extern int klua_go_kg_stream_start(lua_State* L);
extern int klua_go_kg_stream_stop(lua_State* L);
extern int klua_go_kg_stream_set_stream_cb(lua_State* L);

klb_mnp_stream_cb g_cb_klua_go_kg_stream = NULL;
int cb_klua_go_kg_stream(int chnn, int sidx, klb_mnp_stream_t* p_stream)
{
	if (NULL != g_cb_klua_go_kg_stream)
	{
		return g_cb_klua_go_kg_stream(chnn, sidx, p_stream);
	}
	return 1;
}

int klua_go_open_kg_stream_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "init",	klua_go_kg_stream_init },
		{ "quit",	klua_go_kg_stream_quit },
		{ "start",	klua_go_kg_stream_start },
		{ "stop",	klua_go_kg_stream_stop },

		{ "set_stream_cb", klua_go_kg_stream_set_stream_cb},

		{ NULL,		NULL }
	};

	luaL_newlib(L, lib);
	return 1;
}

*/
import "C"

import (
	"unsafe"

	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/knet/kmnp"
)

// OpenKgStream kg_stream
func OpenKgStream(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_stream_cgo)), name)
}

// cbStreamH264 cb stream h264
func cbStreamH264(t int64, chnn, idx int, frame *fh264Frame) {

	spsLen := len(frame.sps)
	ppsLen := len(frame.pps)
	imgLen := len(frame.img)

	var stream C.klb_mnp_stream_t
	stream.dtype = kmnp.MNPh264
	stream.vtype = kmnp.MNPvtypeP
	stream.time = C.int64_t(t)

	if 0 < spsLen && 0 < ppsLen && 0 < imgLen {
		stream.vtype = kmnp.MNPvtypeI
		stream.pack_num = 3

		stream.packs[0].phy = 0
		stream.packs[0].p_data = (*C.char)(frame.pSps)
		stream.packs[0].data_len = C.int(spsLen)

		stream.packs[1].phy = 0
		stream.packs[1].p_data = (*C.char)(frame.pPps)
		stream.packs[1].data_len = C.int(ppsLen)

		stream.packs[2].phy = 0
		stream.packs[2].p_data = (*C.char)(frame.pImg)
		stream.packs[2].data_len = C.int(imgLen)
	} else if 0 < imgLen {
		stream.vtype = kmnp.MNPvtypeP
		stream.pack_num = 1

		stream.packs[0].phy = 0
		stream.packs[0].p_data = (*C.char)(frame.pImg)
		stream.packs[0].data_len = C.int(imgLen)
	}

	C.cb_klua_go_kg_stream(C.int(chnn), C.int(idx), &stream)
}
