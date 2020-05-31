// live stream media framework
// 实时流媒体框架: 服务端

package klsmfw

import (
	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

var gLsmfw *lsmfw

// init
func init() {

}

// Init Init
func Init(cfg string) error {
	gLsmfw = lsmfwCreate()

	return gLsmfw.Init(cfg)
}

// Quit quit
func Quit() {
	gLsmfw.Destroy()
	gLsmfw = nil
}

// Start start
func Start(name, entry, preload, loader string) error {
	return gLsmfw.Start(name, entry, preload, loader)
}

// Stop stop
func Stop() {
	gLsmfw.Stop()
}

// Listen Listen
func Listen(name, network, address string) error {
	return gLsmfw.Listen(name, network, address)
}

// ListenTLS ListenTLS
func ListenTLS(name, network, address, certFile, keyFile string) error {
	return gLsmfw.ListenTLS(name, network, address, certFile, keyFile)
}

// CloseListen CloseListen
func CloseListen(name string) error {
	return gLsmfw.CloseListen(name)
}

// SetNewConn SetNewConn
// 设置新连接回调函数
func SetNewConn(l *klua.LuaState, idx int) {
	gLsmfw.GetLuaEnv().RegNewConn(l, idx)
}

// SetReceiver SetReceiver
// 设置文本接收回调函数
func SetReceiver(l *klua.LuaState, idx int) {
	gLsmfw.GetLuaEnv().RegReceiver(l, idx)
}

// SetDisconnect SetDisconnect
// 设置连接断开回调函数
func SetDisconnect(l *klua.LuaState, idx int) {
	gLsmfw.GetLuaEnv().RegDisconnect(l, idx)
}

// SwitchProtocol SwitchProtocol
// 更换协议类型
func SwitchProtocol(name, dstProtocol string, head, body []byte) bool {
	return gLsmfw.SwitchProtocol(name, dstProtocol, head, body)
}

// CloseConn CloseConn
// 断开连接
func CloseConn(name, protocol string) {
	gLsmfw.CloseConn(name, protocol)
}

// SendTxt SendTxt
// 发送文本
func SendTxt(name, protocol string, sequence, uid uint32, extra, txt []byte) bool {
	return gLsmfw.SendTxt(name, protocol, sequence, uid, extra, txt)
}

// SendFile SendFile
// 发送文件
func SendFile(name, protocol string, sequence, uid uint32, extra, txt, path []byte) bool {
	return gLsmfw.SendFile(name, protocol, sequence, uid, extra, txt, path)
}

// OpenStream OpenStream
// 打开流
func OpenStream(name, protocol string, chnn, sidx uint32) bool {
	return gLsmfw.OpenStream(name, protocol, chnn, sidx)
}

// CloseStream CloseStream
// 关闭流
func CloseStream(name, protocol string, chnn, sidx uint32) bool {
	return gLsmfw.CloseStream(name, protocol, chnn, sidx)
}

// PushMedia PushMedia
// 压入媒体数据
func PushMedia(chnn, sidx uint32, buf kbuf.KBuf) bool {
	return gLsmfw.DispatchMedia(chnn, sidx, buf)
}
