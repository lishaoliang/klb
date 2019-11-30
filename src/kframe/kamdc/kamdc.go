///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   kamdc.go
/// @author lishaoliang
/// @brief	kamdc
///////////////////////////////////////////////////////////////////////////

package kamdc

/*
#cgo CFLAGS: -std=c11 -I ${SRCDIR}/../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"
#include "mnp/klb_mnp.h"
#include "string.h"

extern int klua_go_ka_mdc_init(lua_State* L);
extern int klua_go_ka_mdc_quit(lua_State* L);
extern int klua_go_ka_mdc_start(lua_State* L);
extern int klua_go_ka_mdc_stop(lua_State* L);

extern int klua_go_ka_mdc_send(lua_State* L);
extern int klua_go_ka_mdc_open_stream(lua_State* L);
extern int klua_go_ka_mdc_close_stream(lua_State* L);
extern int klua_go_ka_mdc_close_stream(lua_State* L);


extern int cb_klua_go_ka_mdc_push_stream(int chnn, int sidx, klb_mnp_stream_t* p_stream);
static int klua_go_ka_mdc_get_cb_push_stream(lua_State* L)
{
	lua_pushlightuserdata(L, (void*)(cb_klua_go_ka_mdc_push_stream));
	return 1;
}

int klua_go_open_ka_mdc_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "init",	klua_go_ka_mdc_init },
		{ "quit",	klua_go_ka_mdc_quit },
		{ "start",	klua_go_ka_mdc_start },
		{ "stop",	klua_go_ka_mdc_stop },

		{ "send",			klua_go_ka_mdc_send },
		{ "open_stream",	klua_go_ka_mdc_open_stream },
		{ "close_stream",	klua_go_ka_mdc_close_stream },

		{ "get_cb_push_stream",	klua_go_ka_mdc_get_cb_push_stream},

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
)

// init
func init() {
	//fmt.Println("main init!")
}

// OpenKaMdc ka_mdc. k arm Multimedia Dispatching Center
func OpenKaMdc(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_ka_mdc_cgo)), name)
}
