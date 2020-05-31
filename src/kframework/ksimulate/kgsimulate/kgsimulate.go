package kgsimulate

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_simulate_init(lua_State* L);
extern int klua_go_kg_simulate_quit(lua_State* L);
extern int klua_go_kg_simulate_start(lua_State* L);
extern int klua_go_kg_simulate_stop(lua_State* L);

extern int klua_go_kg_simulate_set_stream_receiver(lua_State* L);
extern int klua_go_kg_simulate_command(lua_State* L);


int klua_go_open_kg_simulate_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "init",		klua_go_kg_simulate_init },
		{ "quit",		klua_go_kg_simulate_quit },
		{ "start",		klua_go_kg_simulate_start },
		{ "stop",		klua_go_kg_simulate_stop },

		{ "set_stream_receiver",	klua_go_kg_simulate_set_stream_receiver },
		{ "command",				klua_go_kg_simulate_command },

		{ NULL,			NULL }
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

// OpenKgSimulate kg_simulate
func OpenKgSimulate(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_simulate_cgo)), name)
}
