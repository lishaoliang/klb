package knet

import (
	"sync"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/khttp/khttpconn"
	"github.com/lishaoliang/klb/src/knet/khttp/khttpflv"
	"github.com/lishaoliang/klb/src/knet/khttp/khttpmnp"
	"github.com/lishaoliang/klb/src/knet/kmnp/kmnpconn"
	"github.com/lishaoliang/klb/src/knet/krtsp/krtspserve"
	"github.com/lishaoliang/klb/src/knet/kws/kwsconn"
	"github.com/lishaoliang/klb/src/knet/kws/kwsflv"
	"github.com/lishaoliang/klb/src/knet/kws/kwsmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

var gMapCreateCb *kutil.MapIface // 创建函数
var gMapMutex sync.Mutex         // 锁

// init
func init() {
	gMapCreateCb = kutil.NewMapIface("", "", 12)
	gPathMap = make(map[string]string)

	// 注册服务端协议
	Register(kconn.SERVE, MNP, kmnpconn.CreateServe)
	Register(kconn.SERVE, HTTP, khttpconn.CreateServe)
	Register(kconn.SERVE, HTTPS, khttpconn.CreateServe)
	Register(kconn.SERVE, RTSP, krtspserve.CreateServe)
	Register(kconn.SERVE, WS, kwsconn.CreateServe)

	Register(kconn.SERVE, HTTPMNP, khttpmnp.CreateServe)
	Register(kconn.SERVE, HTTPFLV, khttpflv.CreateServe)

	Register(kconn.SERVE, WSMNP, kwsmnp.CreateServe)
	Register(kconn.SERVE, WSFLV, kwsflv.CreateServe)

	// 注册客户端协议
	Register(kconn.CLIENT, MNP, kmnpconn.CreateClient)
	Register(kconn.CLIENT, HTTP, khttpconn.CreateClient)
	Register(kconn.CLIENT, HTTPS, khttpconn.CreateClient)
	Register(kconn.CLIENT, WS, kwsconn.CreateClient)
}

// protocol name
const (
	UNKOWN = "UNKOWN" // unkown
	MNP    = "MNP"    // mnp 协议
	MNPS   = "MNPS"   // mnp 协议: TLS
	RTMP   = "RTMP"   // rtmp 协议
	RTSP   = "RTSP"   // rtsp 协议
	HTTP   = "HTTP"   // http 协议
	HTTPS  = "HTTPS"  // https 协议: TLS
	WS     = "WS"     // websocket 协议
	WSS    = "WSS"    // websocket 协议: TLS

	HTTPMNP = "HTTP-MNP" // http mnp 协议
	HTTPFLV = "HTTP-FLV" // http flv 协议
	WSMNP   = "WS-MNP"   // websocket mnp 协议
	WSFLV   = "WS-FLV"   // websocket flv 协议
)

// Register create KConn callback func
// @brief 注册创建连接回调函数
// @param [in] netType kconn.SERVE."SERVE"; kconn.CLIENT."CLIENT"
// @param [in] protocol 协议名称
// @param [in] cb 回调函数
// @return 无
// @note 已存在key, 则替换
func Register(netType, protocol string, cb kconn.CreateCb) {
	gMapMutex.Lock()
	defer gMapMutex.Unlock()

	key := netType + "@" + protocol

	gMapCreateCb.Remove(key)
	gMapCreateCb.PushByName(key, cb)
}

// Unregister create KConn callback func
// @brief 注销创建连接回调函数
// @param [in] netType kconn.SERVE."SERVE"; kconn.CLIENT."CLIENT"
// @param [in] protocol 协议名称
// @return 无
func Unregister(netType, protocol string) {
	gMapMutex.Lock()
	defer gMapMutex.Unlock()

	key := netType + "@" + protocol

	gMapCreateCb.Remove(key)
}

// GetCreateCb get GetCreateCb func
// @brief 获取创建连接回调函数
// @param [in] netType kconn.SERVE."SERVE"; kconn.CLIENT."CLIENT"
// @param [in] protocol 协议名称
// @return kconn.CreateCb 创建函数
func GetCreateCb(netType, protocol string) kconn.CreateCb {
	gMapMutex.Lock()
	defer gMapMutex.Unlock()

	key := netType + "@" + protocol

	item := gMapCreateCb.Find(key)
	if nil != item {
		if cb, ok := item.(kconn.CreateCb); ok {
			return cb
		}
	}

	return nil
}
