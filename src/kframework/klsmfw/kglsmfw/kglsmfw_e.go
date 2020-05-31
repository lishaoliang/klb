package kglsmfw

/*
#include "klua/klua.h"
extern int klua_go_open_kg_lsmfw_cgo(lua_State* L);
*/
import "C"
import (
	"github.com/lishaoliang/klb/src/kframework/klsmfw"
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/knet"
)

// exprot func

//export klua_go_open_kg_lsmfw
func klua_go_open_kg_lsmfw(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_lsmfw_cgo(lua)

	return ret
}

//export klua_go_kg_lsmfw_register_protocol
func klua_go_kg_lsmfw_register_protocol(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	protocol := klua.LuaLCheckstring(l, 1) // @1 协议名称
	path := klua.LuaLCheckstring(l, 2)     // @2 路径

	knet.RegisterProtocol(protocol, path)

	return 0
}

//export klua_go_kg_lsmfw_init
func klua_go_kg_lsmfw_init(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	cfg := klua.LuaLCheckstring(l, 1) // @1 初始配置

	err := klsmfw.Init(cfg)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_lsmfw_quit
func klua_go_kg_lsmfw_quit(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	klsmfw.Quit()

	return 0
}

//export klua_go_kg_lsmfw_start
func klua_go_kg_lsmfw_start(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)    // @1 线程名称,thread name
	entry := klua.LuaLCheckstring(l, 2)   // @2 入口脚本,entry
	preload := klua.LuaLCheckstring(l, 3) // @3 预加载函数名
	loader := klua.LuaLCheckstring(l, 4)  // @4 自定义加载函数名

	err := klsmfw.Start(name, entry, preload, loader)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_lsmfw_stop
func klua_go_kg_lsmfw_stop(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	klsmfw.Stop()

	return 0
}

//export klua_go_kg_lsmfw_listen
func klua_go_kg_lsmfw_listen(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)    // @1 监听名称
	network := klua.LuaLCheckstring(l, 2) // @2 网络类型: "tcp"
	address := klua.LuaLCheckstring(l, 3) // @3 网络地址: ":8000"

	err := klsmfw.Listen(name, network, address)

	if nil != err {
		klua.LuaPushboolean(l, false)      // #1 是否成功
		klua.LuaPushstring(l, err.Error()) // #2 错误串
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_lsmfw_listen_tls
func klua_go_kg_lsmfw_listen_tls(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)
	network := klua.LuaLCheckstring(l, 2)
	address := klua.LuaLCheckstring(l, 3)
	certFile := klua.LuaLCheckstring(l, 4)
	keyFile := klua.LuaLCheckstring(l, 5)

	err := klsmfw.ListenTLS(name, network, address, certFile, keyFile)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_lsmfw_close_listen
func klua_go_kg_lsmfw_close_listen(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)
	name := klua.LuaLCheckstring(l, 1) // @1 监听名称

	err := klsmfw.CloseListen(name)

	if nil != err {
		klua.LuaPushboolean(l, false)      // #1 是否成功
		klua.LuaPushstring(l, err.Error()) // #1 错误串
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_lsmfw_set_newconn
func klua_go_kg_lsmfw_set_newconn(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	klsmfw.SetNewConn(l, 1) // @1 数据接收函数

	return 0
}

//export klua_go_kg_lsmfw_set_receiver
func klua_go_kg_lsmfw_set_receiver(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	klsmfw.SetReceiver(l, 1) // @1 数据接收函数

	return 0
}

//export klua_go_kg_lsmfw_set_disconnect
func klua_go_kg_lsmfw_set_disconnect(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	klsmfw.SetDisconnect(l, 1) // @1 连接断开函数

	return 0
}

//export klua_go_kg_lsmfw_switch_protocol
func klua_go_kg_lsmfw_switch_protocol(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)        // @1 连接名称
	dstProtocol := klua.LuaLCheckstring(l, 2) // @2 目标协议名称
	head := klua.LuaLCheckstringB(l, 3)       // @3 头部数据
	body := klua.LuaLCheckstringB(l, 4)       // @4 数据体

	b := klsmfw.SwitchProtocol(name, dstProtocol, head, body)

	klua.LuaPushboolean(l, b)
	return 1
}

//export klua_go_kg_lsmfw_close_conn
func klua_go_kg_lsmfw_close_conn(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)     // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2) // @2 协议名称

	klsmfw.CloseConn(name, protocol)

	return 0
}

//export klua_go_kg_lsmfw_send_txt
func klua_go_kg_lsmfw_send_txt(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)      // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2)  // @2 协议名称
	sequence := klua.LuaLCheckinteger(l, 3) // @3 序列号
	uid := klua.LuaLCheckinteger(l, 4)      // @4 UID
	extra := klua.LuaLCheckstringB(l, 5)    // @5 额外数据(或头数据)
	txt := klua.LuaLCheckstringB(l, 6)      // @6 文本数据

	b := klsmfw.SendTxt(name, protocol, uint32(sequence), uint32(uid), extra, txt)

	klua.LuaPushboolean(l, b)
	return 1
}

//export klua_go_kg_lsmfw_send_file
func klua_go_kg_lsmfw_send_file(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)      // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2)  // @2 协议名称
	sequence := klua.LuaLCheckinteger(l, 3) // @3 序列号
	uid := klua.LuaLCheckinteger(l, 4)      // @4 UID
	extra := klua.LuaLCheckstringB(l, 5)    // @5 额外数据(或头数据)
	txt := klua.LuaLCheckstringB(l, 6)      // @6 文本数据
	path := klua.LuaLCheckstringB(l, 7)     // @7 文件路径

	b := klsmfw.SendFile(name, protocol, uint32(sequence), uint32(uid), extra, txt, path)

	klua.LuaPushboolean(l, b)
	return 1
}

//export klua_go_kg_lsmfw_open_stream
func klua_go_kg_lsmfw_open_stream(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)     // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2) // @2 协议名称
	chnn := klua.LuaLCheckinteger(l, 3)    // @3 通道号
	sidx := klua.LuaLCheckinteger(l, 4)    // @4 流序号

	b := klsmfw.OpenStream(name, protocol, uint32(chnn), uint32(sidx))

	klua.LuaPushboolean(l, b)
	return 1
}

//export klua_go_kg_lsmfw_close_stream
func klua_go_kg_lsmfw_close_stream(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)     // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2) // @2 协议名称
	chnn := klua.LuaLCheckinteger(l, 3)    // @3 通道号
	sidx := klua.LuaLCheckinteger(l, 4)    // @4 流序号

	b := klsmfw.CloseStream(name, protocol, uint32(chnn), uint32(sidx))

	klua.LuaPushboolean(l, b)
	return 1
}
