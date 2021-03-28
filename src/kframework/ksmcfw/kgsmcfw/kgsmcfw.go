package kgsmcfw

/*
#cgo CFLAGS: -std=c99 -I ${SRCDIR}/../../../../inc
#cgo LDFLAGS: -L ${SRCDIR}/../../../../lib -lklb_c -lpthread -lm

#include "klb_type.h"
#include "klua/klua.h"
#include "klbbase/klb_mnp.h"

extern int klua_go_kg_smcfw_init(lua_State* L);
extern int klua_go_kg_smcfw_quit(lua_State* L);
extern int klua_go_kg_smcfw_start(lua_State* L);
extern int klua_go_kg_smcfw_stop(lua_State* L);

extern int klua_go_kg_smcfw_set_newconn(lua_State* L);
extern int klua_go_kg_smcfw_set_receiver(lua_State* L);
extern int klua_go_kg_smcfw_set_disconnect(lua_State* L);

extern int klua_go_kg_smcfw_dial(lua_State* L);
extern int klua_go_kg_smcfw_send_txt(lua_State* L);

extern int klua_go_kg_smcfw_bind(lua_State* L);
extern int klua_go_kg_smcfw_unbind(lua_State* L);

extern int klua_go_kg_smcfw_stream_cb(uint32_t chnn, uint32_t sidx, klb_mnp_stream_t* stream);
static int klua_go_kg_smcfw_get_stream_cb(lua_State* L)
{
	lua_pushlightuserdata(L, klua_go_kg_smcfw_stream_cb);
	return 1;
}

int klua_go_open_kg_smcfw_cgo(lua_State* L)
{
	static luaL_Reg lib[] =
	{
		{ "init",			klua_go_kg_smcfw_init },
		{ "quit",			klua_go_kg_smcfw_quit },
		{ "start",			klua_go_kg_smcfw_start },
		{ "stop",			klua_go_kg_smcfw_stop },

		{ "set_newconn",	klua_go_kg_smcfw_set_newconn },
		{ "set_receiver",	klua_go_kg_smcfw_set_receiver },
		{ "set_disconnect",	klua_go_kg_smcfw_set_disconnect },

		{ "dial",			klua_go_kg_smcfw_dial },
		{ "send_txt",		klua_go_kg_smcfw_send_txt },

		{ "bind",			klua_go_kg_smcfw_bind },
		{ "unbind",			klua_go_kg_smcfw_unbind },

		{ "get_stream_cb",	klua_go_kg_smcfw_get_stream_cb },

		{ NULL,				NULL }
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

// OpenKgSmcfw kg_smcfw
func OpenKgSmcfw(lua *klua.LuaState, name string) {
	klua.LoadLib(lua, (klua.LuaCFunction)(unsafe.Pointer(C.klua_go_open_kg_smcfw_cgo)), name)
}
