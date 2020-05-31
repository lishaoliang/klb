package klsmfw

import (
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kutil"
)

// luaEnv luaEnv
type luaEnv struct {
	name string    // 名称
	ctx  *klua.Ctx // lua环境

	newconn    int // 新连接加入
	receiver   int // Lua接收数据函数
	disconnect int // Lua连接断开调用函数
}

// luaEnvCreate luaEnvCreate
func luaEnvCreate() *luaEnv {
	var m luaEnv

	return &m
}

// Destroy Destroy
func (m *luaEnv) Destroy() {

}

// Open open
func (m *luaEnv) Open(name, entry, preload, loader string) {
	var err error
	m.ctx, err = klua.Open(name, entry, preload, loader, false)

	if nil != err {
		panic(err.Error())
	}

	m.name = name
}

// Close close
func (m *luaEnv) Close() {
	//l := m.ctx.GetEnv().GetLuaState()
	//m.UnregReceiver(l)
	//m.UnregDisconnect(l)

	klua.Close(m.name)
	m.name = ""
	m.ctx = nil
}

// GetCtx get lua ctx
func (m *luaEnv) GetCtx() *klua.Ctx {
	return m.ctx
}

// RegNewConn RegNewConn
func (m *luaEnv) RegNewConn(l *klua.LuaState, idx int) {
	m.UnregNewConn(l)

	klua.LuaLChecktype(l, idx, klua.LUATFUNCTION)
	klua.LuaPushvalue(l, idx)

	m.newconn = klua.LuaLRef(l, klua.LUAREGISTRYINDEX)
	kutil.Assert(0 < m.newconn)
}

// UnregNewConn UnregNewConn
func (m *luaEnv) UnregNewConn(l *klua.LuaState) {
	if 0 < m.newconn {
		klua.LuaLUnref(l, klua.LUAREGISTRYINDEX, m.newconn)
		m.newconn = 0
	}
}

// RegReceiver RegReceiver
func (m *luaEnv) RegReceiver(l *klua.LuaState, idx int) {
	m.UnregReceiver(l)

	klua.LuaLChecktype(l, idx, klua.LUATFUNCTION)
	klua.LuaPushvalue(l, idx)

	m.receiver = klua.LuaLRef(l, klua.LUAREGISTRYINDEX)
	kutil.Assert(0 < m.receiver)
}

// UnregReceiver UnregReceiver
func (m *luaEnv) UnregReceiver(l *klua.LuaState) {
	if 0 < m.receiver {
		klua.LuaLUnref(l, klua.LUAREGISTRYINDEX, m.receiver)
		m.receiver = 0
	}
}

// RegDisconnect RegDisconnect
func (m *luaEnv) RegDisconnect(l *klua.LuaState, idx int) {
	m.UnregDisconnect(l)

	klua.LuaLChecktype(l, idx, klua.LUATFUNCTION)
	klua.LuaPushvalue(l, idx)

	m.disconnect = klua.LuaLRef(l, klua.LUAREGISTRYINDEX)
	kutil.Assert(0 < m.disconnect)
}

// UnregDisconnect UnregDisconnect
func (m *luaEnv) UnregDisconnect(l *klua.LuaState) {
	if 0 < m.disconnect {
		klua.LuaLUnref(l, klua.LUAREGISTRYINDEX, m.disconnect)
		m.disconnect = 0
	}
}

// CallNewConn CallNewConn
func (m *luaEnv) CallNewConn(name, protocol string, ok bool, err string) int {
	if m.newconn <= 0 {
		return 1
	}

	e := m.ctx.GetEnv()
	l := e.GetLuaState()

	klua.LuaRawgeti(l, klua.LUAREGISTRYINDEX, klua.LuaInteger(m.newconn)) // to call 'receiver' in protected mode
	klua.LuaPushstring(l, name)                                           // 1st argument
	klua.LuaPushstring(l, protocol)                                       // 2st argument
	klua.LuaPushboolean(l, ok)                                            // 3st argument
	klua.LuaPushstring(l, err)                                            // 4st argument

	status := klua.LuaPcall(l, 4, 1, 0) // do the call
	result := klua.LuaToboolean(l, -1)  // get result

	if 0 == status {
		klua.LuaPop(l, 1)
	} else {
		e.Report(status)
	}

	if result && 0 == status {
		return 0
	}

	return 1
}

// CallReceiver CallReceiver
func (m *luaEnv) CallReceiver(name, protocol string, sequence, uid uint32, extra, txt []byte) int {
	if m.receiver <= 0 {
		return 1
	}

	e := m.ctx.GetEnv()
	l := e.GetLuaState()

	klua.LuaRawgeti(l, klua.LUAREGISTRYINDEX, klua.LuaInteger(m.receiver)) // to call 'receiver' in protected mode
	klua.LuaPushstring(l, name)                                            // 1st argument
	klua.LuaPushstring(l, protocol)                                        // 2st argument
	klua.LuaPushinteger(l, int64(sequence))                                // 3st argument
	klua.LuaPushinteger(l, int64(uid))                                     // 4st argument
	klua.LuaPushstringB(l, extra)                                          // 5st argument
	klua.LuaPushstringB(l, txt)                                            // 6st argument

	status := klua.LuaPcall(l, 6, 1, 0) // do the call
	result := klua.LuaToboolean(l, -1)  // get result

	if 0 == status {
		klua.LuaPop(l, 1)
	} else {
		e.Report(status)
	}

	if result && 0 == status {
		return 0
	}

	return 1
}

// CallDisconnect CallDisconnect
func (m *luaEnv) CallDisconnect(name, protocol, err string) int {
	if m.disconnect <= 0 {
		return 1
	}

	e := m.ctx.GetEnv()
	l := e.GetLuaState()

	klua.LuaRawgeti(l, klua.LUAREGISTRYINDEX, klua.LuaInteger(m.disconnect)) // to call 'receiver' in protected mode
	klua.LuaPushstring(l, name)                                              // 1st argument
	klua.LuaPushstring(l, protocol)                                          // 2st argument
	klua.LuaPushstring(l, err)                                               // 3st argument

	status := klua.LuaPcall(l, 3, 1, 0) // do the call
	result := klua.LuaToboolean(l, -1)  // get result

	if 0 == status {
		klua.LuaPop(l, 1)
	} else {
		e.Report(status)
	}

	if result && 0 == status {
		return 0
	}

	return 1
}
