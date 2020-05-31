package kglsmfw

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../../lib -lklb_c -lpthread -lm

#include "klua/klua.h"

extern int klua_go_kg_lsmfw_register_protocol(lua_State* L);

extern int klua_go_kg_lsmfw_init(lua_State* L);
extern int klua_go_kg_lsmfw_quit(lua_State* L);
extern int klua_go_kg_lsmfw_start(lua_State* L);
extern int klua_go_kg_lsmfw_stop(lua_State* L);

extern int klua_go_kg_lsmfw_listen(lua_State* L);
extern int klua_go_kg_lsmfw_listen_tls(lua_State* L);
extern int klua_go_kg_lsmfw_close_listen(lua_State* L);

extern int klua_go_kg_lsmfw_set_newconn(lua_State* L);
extern int klua_go_kg_lsmfw_set_receiver(lua_State* L);
extern int klua_go_kg_lsmfw_set_disconnect(lua_State* L);

extern int klua_go_kg_lsmfw_switch_protocol(lua_State* L);
extern int klua_go_kg_lsmfw_close_conn(lua_State* L);
extern int klua_go_kg_lsmfw_send_txt(lua_State* L);
extern int klua_go_kg_lsmfw_send_file(lua_State* L);

extern int klua_go_kg_lsmfw_open_stream(lua_State* L);
extern int klua_go_kg_lsmfw_close_stream(lua_State* L);

int klua_go_open_kg_lsmfw_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "register_protocol",	klua_go_kg_lsmfw_register_protocol },

		{ "init",				klua_go_kg_lsmfw_init },
		{ "quit",				klua_go_kg_lsmfw_quit },
		{ "start",				klua_go_kg_lsmfw_start },
		{ "stop",				klua_go_kg_lsmfw_stop },

		{ "listen",				klua_go_kg_lsmfw_listen },
		{ "listen_tls", 		klua_go_kg_lsmfw_listen_tls },
		{ "close_listen",		klua_go_kg_lsmfw_close_listen },

		{ "set_newconn",		klua_go_kg_lsmfw_set_newconn },
		{ "set_receiver",		klua_go_kg_lsmfw_set_receiver },
		{ "set_disconnect",		klua_go_kg_lsmfw_set_disconnect },

		{ "switch_protocol",	klua_go_kg_lsmfw_switch_protocol },
		{ "close_conn",			klua_go_kg_lsmfw_close_conn },
		{ "send_txt",			klua_go_kg_lsmfw_send_txt },
		{ "send_file",			klua_go_kg_lsmfw_send_file },

		{ "open_stream",		klua_go_kg_lsmfw_open_stream },
		{ "close_stream",		klua_go_kg_lsmfw_close_stream },

		{ NULL,					NULL }
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

// OpenKgLsmfw kg_lsmfw
func OpenKgLsmfw(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_lsmfw_cgo)), name)
}
