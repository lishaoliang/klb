// stream media client framework
// 流媒体客户端

package ksmcfw

import (
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/knet/kmnp"
)

var gSmcfw *smcfw

// init
func init() {

}

// Init Init
func Init(cfg string) error {
	gSmcfw = smcfwCreate()

	return gSmcfw.Init(cfg)
}

// Quit quit
func Quit() {
	gSmcfw.Destroy()
	gSmcfw = nil
}

// Start start
func Start(name, entry, preload, loader string) error {
	return gSmcfw.Start(name, entry, preload, loader)
}

// Stop stop
func Stop() {
	gSmcfw.Stop()
}

// SetNewConn SetNewConn
func SetNewConn(l *klua.LuaState, idx int) {
	gSmcfw.GetLuaEnv().RegNewConn(l, idx)
}

// SetReceiver SetReceiver
func SetReceiver(l *klua.LuaState, idx int) {
	gSmcfw.GetLuaEnv().RegReceiver(l, idx)
}

// SetDisconnect SetDisconnect
func SetDisconnect(l *klua.LuaState, idx int) {
	gSmcfw.GetLuaEnv().RegDisconnect(l, idx)
}

// Dial Dial
func Dial(name, protocol, network, address string) {
	gSmcfw.Dial(name, protocol, network, address)
}

// SendTxt SendTxt
func SendTxt(name, protocol string, sequence, uid uint32, extra, txt []byte) bool {
	return gSmcfw.SendTxt(name, protocol, sequence, uid, extra, txt)
}

// Bind Bind
func Bind(name string, chnn, sidx uint32) {
	gSmcfw.Bind(name, chnn, sidx)
}

// Unbind Unbind
func Unbind(name string) {
	gSmcfw.Unbind(name)
}

// StreamCb Stream callback
func StreamCb(chnn, sidx uint32, mnps kmnp.MnpStream) {
	gSmcfw.StreamCb(chnn, sidx, mnps)
}
