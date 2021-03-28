package kgsmcfw

/*
#include "klb_type.h"
#include "klua/klua.h"
#include "klbbase/klb_mnp.h"

extern int klua_go_open_kg_smcfw_cgo(lua_State* L);
*/
import "C"
import (
	"unsafe"

	"github.com/lishaoliang/klb/src/kframework/ksmcfw"
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/knet/kmnp"
)

// exprot func

//export klua_go_open_kg_smcfw
func klua_go_open_kg_smcfw(lua *C.lua_State) C.int {
	// export for c load

	ret := C.klua_go_open_kg_smcfw_cgo(lua)

	return ret
}

//export klua_go_kg_smcfw_stream_cb
func klua_go_kg_smcfw_stream_cb(chnn, sidx C.uint32_t, stream *C.klb_mnp_stream_t) C.int {

	/// @brief 码流回调函数定义
	/// @param [in] chnn        通道
	/// @param [in] sidx        流序号: klb_mnp_sidx_e
	/// @param [in] *p_stream   码流数据
	/// @return int 0.成功; 非0.失败
	/// typedef int(*klb_mnp_stream_cb)(uint32_t chnn, uint32_t sidx, klb_mnp_stream_t* p_stream);

	// 转换成对应的go结构
	var mnps kmnp.MnpStream
	mnps.Vtype = uint8(stream.vtype)
	mnps.Dtype = uint32(stream.dtype)
	mnps.Time = int64(stream.time)

	for i := 0; i < int(stream.pack_num); i++ {
		var pack kmnp.MnpPack
		pack.Phy = uintptr(stream.packs[i].phy)
		pack.Data = unsafe.Pointer(stream.packs[i].p_data)
		pack.DataLen = int(stream.packs[i].data_len)

		mnps.Size += pack.DataLen

		mnps.Packs = append(mnps.Packs, pack)
	}

	ksmcfw.StreamCb(uint32(chnn), uint32(sidx), mnps)

	//if 0 == chnn {
	//	fmt.Println("kgsmcfw_e:", chnn, sidx, mnps.Time, mnps.Size)
	//}

	return 0
}

//export klua_go_kg_smcfw_init
func klua_go_kg_smcfw_init(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	cfg := klua.LuaLCheckstring(l, 1) // @1 初始配置

	err := ksmcfw.Init(cfg)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_smcfw_quit
func klua_go_kg_smcfw_quit(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	ksmcfw.Quit()

	return 0
}

//export klua_go_kg_smcfw_start
func klua_go_kg_smcfw_start(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)    // @1 线程名称,thread name
	entry := klua.LuaLCheckstring(l, 2)   // @2 入口脚本,entry
	preload := klua.LuaLCheckstring(l, 3) // @3 预加载函数名
	loader := klua.LuaLCheckstring(l, 4)  // @4 自定义加载函数名

	err := ksmcfw.Start(name, entry, preload, loader)

	if nil != err {
		klua.LuaPushboolean(l, false)
		klua.LuaPushstring(l, err.Error())
	} else {
		klua.LuaPushboolean(l, true)
		klua.LuaPushstring(l, "")
	}

	return 2
}

//export klua_go_kg_smcfw_stop
func klua_go_kg_smcfw_stop(lua *C.lua_State) C.int {
	//l := (*klua.LuaState)(lua)

	ksmcfw.Stop()

	return 0
}

//export klua_go_kg_smcfw_set_newconn
func klua_go_kg_smcfw_set_newconn(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ksmcfw.SetNewConn(l, 1) // @1 新连接

	return 0
}

//export klua_go_kg_smcfw_set_receiver
func klua_go_kg_smcfw_set_receiver(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ksmcfw.SetReceiver(l, 1) // @1 数据接收函数

	return 0
}

//export klua_go_kg_smcfw_set_disconnect
func klua_go_kg_smcfw_set_disconnect(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	ksmcfw.SetDisconnect(l, 1) // @1 连接断开函数

	return 0
}

//export klua_go_kg_smcfw_dial
func klua_go_kg_smcfw_dial(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)     // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2) // @2 协议名称
	network := klua.LuaLCheckstring(l, 3)  // @3 网络
	address := klua.LuaLCheckstring(l, 4)  // @4 地址

	ksmcfw.Dial(name, protocol, network, address)

	return 0
}

//export klua_go_kg_smcfw_send_txt
func klua_go_kg_smcfw_send_txt(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)      // @1 连接名称
	protocol := klua.LuaLCheckstring(l, 2)  // @2 协议名称
	sequence := klua.LuaLCheckinteger(l, 3) // @3 序列号
	uid := klua.LuaLCheckinteger(l, 4)      // @4 UID
	extra := klua.LuaLCheckstringB(l, 5)    // @5 额外数据(或头数据)
	txt := klua.LuaLCheckstringB(l, 6)      // @6 文本数据

	b := ksmcfw.SendTxt(name, protocol, uint32(sequence), uint32(uid), extra, txt)

	klua.LuaPushboolean(l, b)
	return 1
}

//export klua_go_kg_smcfw_bind
func klua_go_kg_smcfw_bind(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1)  // @1 连接名称
	chnn := klua.LuaLCheckinteger(l, 2) // @2 通道
	sidx := klua.LuaLCheckinteger(l, 3) // @3 流序号

	ksmcfw.Bind(name, uint32(chnn), uint32(sidx))

	return 0
}

//export klua_go_kg_smcfw_unbind
func klua_go_kg_smcfw_unbind(lua *C.lua_State) C.int {
	l := (*klua.LuaState)(lua)

	name := klua.LuaLCheckstring(l, 1) // @1 连接名称

	ksmcfw.Unbind(name)

	return 0
}
