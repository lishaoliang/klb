///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kgcrypto_e.go
/// @author	lishaoliang
/// @brief	kg_time export
///////////////////////////////////////////////////////////////////////////

package kgcrypto

/*
#include "klua/klua.h"
extern int klua_go_open_kg_crypto_cgo(lua_State* L);
*/
import "C"
import (
	"crypto/sha1"
	"encoding/base64"
	"io"
	"time"

	"github.com/lishaoliang/klb/src/klua"
)

// exprot func

//export klua_go_open_kg_crypto
func klua_go_open_kg_crypto(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_crypto_cgo(lua)

	return ret
}

//export klua_go_kg_crypto_sleep
func klua_go_kg_crypto_sleep(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ms := klua.LuaLCheckinteger(l, 1) // @1 sleep ms

	time.Sleep((time.Duration)(time.Millisecond) * (time.Duration)(ms))
	return 0
}

//export klua_go_kg_crypto_sleep_ns
func klua_go_kg_crypto_sleep_ns(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ns := klua.LuaLCheckinteger(l, 1) // @1 sleep ns

	time.Sleep((time.Duration)(time.Nanosecond) * (time.Duration)(ns))
	return 0
}

//export klua_go_kg_crypto_ws_encode
func klua_go_kg_crypto_ws_encode(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	key := klua.LuaLCheckstring(l, 1) // @1 key

	sha := sha1.New()
	io.WriteString(sha, key)
	io.WriteString(sha, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")

	b64 := base64.StdEncoding.EncodeToString(sha.Sum(nil))

	klua.LuaPushstring(l, b64)
	return 1
}
