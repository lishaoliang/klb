package kconn

import (
	"net"
	"net/http"

	"github.com/lishaoliang/klb/src/kutil/kbuf"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

// net type serve,client
const (
	SERVE  = "SERVE"
	CLIENT = "CLIENT"
)

// Packtype 数据包类型
const (
	DataHeart = uint8(0x00) // 心跳, 同MNPheart
	DataTxt   = uint8(0x01) // 文本, 同MNPtxt
	DataBin   = uint8(0x02) // 二进制, 同MNPbin
	DataMedia = uint8(0x03) // 媒体数据, 同MNPmedia

	DataFile = uint8(0xF0) // 文件数据

	Disconnect      = uint8(0xFD) // 连接断开
	NewConnectError = uint8(0xFE) // 新连接错误
	NewConnect      = uint8(0xFF) // 新连接加入
)

// ConnWrite Conn data write
// 连接写数据中间格式
type ConnWrite struct {
	Packtype uint8     // 类型: DataHeart,DataTxt,DataBin,DataMedia,DataFile
	Sequence uint32    // 序列号
	UID      uint32    // 用户自定id
	Extra    []byte    // (非媒体数据)附加数据or头数据
	Data     []byte    // (非媒体数据)正式数据
	Path     []byte    // Packtype=DataFile时, 表示读取文件路径
	Buf      kbuf.KBuf // 媒体数据
}

// ConnRead Conn data read
// 连接读数据中间格式
type ConnRead struct {
	Packtype uint8  // 类型: DataTxt,DataBin,DataMedia,Disconnect,NewConnect
	Protocol string // 协议
	Name     string // 连接名称
	Err      error  // 错误

	Sequence uint32    // 序列号
	UID      uint32    // 用户自定id
	Extra    []byte    // (非媒体数据)附加数据or头数据
	Data     []byte    // (非媒体数据)正式数据
	Buf      kbuf.KBuf // 媒体数据
}

// CreateCb create KConn callback
// @brief 创建KConn连接回调函数
// @param [in] conn go标准连接
// @param [in] data 连接已经被读出的数据
// @param [in] pool 读取媒体数据时使用的内存池
// @param [in] rOut 读取非媒体数据输出管道: 输出非媒体数据
// @param [in] rMdOut 读取媒体数据输出管道: 输出媒体数据
// @return KConn 创建好的连接
type CreateCb func(conn net.Conn, data []byte, pool kpool.KPool, rOut *chan ConnRead, rMdOut *chan ConnRead) KConn

// KConn connect interface
// 连接通用接口
type KConn interface {
	// http.Hijack原本是: 让调用者接管连接, 返回连接和关联到该连接的一个缓冲读写器
	// 这里简化处理: 只保留连接句柄, 其他资源(内存等)一律随原对象释放
	// 仅用于HTTP协议做转换
	http.Hijacker

	Destroy() // 销毁

	Start() // 开始线程 or 开始读写数据
	//Stop()  // 关闭线程

	SetProtocol(protocol string) // 设置协议名称
	GetProtocol() string         // 获取协议名称

	SetName(name string) // 设置连接名称
	GetName() string     // 获取连接名称

	SendTxt(sequence, uid uint32, extra, txt []byte) bool        // 发送文本数据
	SendBin(sequence, uid uint32, extra, bin []byte) bool        // 发送二进制数据
	SendFile(sequence, uid uint32, extra, txt, path []byte) bool // 发送文件
	SendMedia(buf kbuf.KBuf) bool                                // 发送媒体数据

	//Recv() <-chan ConnRead      // 接收数据文本/二进制数据
	//RecvMedia() <-chan ConnRead // 接收媒体数据
}
