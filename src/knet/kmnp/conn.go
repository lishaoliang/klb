///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	conn.go
/// @author	lishaoliang
/// @brief	conn
///////////////////////////////////////////////////////////////////////////

package kmnp

import "github.com/lishaoliang/klb/src/kbuf/kmpool"

// ConnDataW conn data write
// 连接写数据中间格式
type ConnDataW struct {
	Data []byte        // 整块数据
	Buf  *kmpool.KmBuf // 媒体数据
}

// ConnDataR conn data read
// 连接读数据中间格式
type ConnDataR struct {
	Protocol string // 协议
	Name     string // 连接名称
	Err      error  // 错误

	Packtype uint8  // 类型 MNPtxt,MNPbin,MNPmedia
	Sequence uint32 // 序列号
	UID      uint32 // 用户自定id

	Extra []byte // 附加数据
	Data  []byte // 数据
}

// ConnInterface connect
// 媒体连接通用接口
type ConnInterface interface {
	Destroy() // 销毁

	Start() // 开始线程
	Stop()  // 关闭线程

	SetProtocol(protocol string) // 设置协议名称
	GetProtocol() string         // 获取协议名称

	SetName(name string) // 设置连接名称
	GetName() string     // 获取连接名称

	SendTxt(sequence, uid uint32, extra, msg []byte) bool // 发送文本数据
	SendBin(sequence, uid uint32, extra, bin []byte) bool // 发送二进制数据
	SendMediaKmBuf(buf *kmpool.KmBuf) bool                // 发送媒体数据(内存池方式)
	SendMediaByte(msg []byte) bool                        // 发送媒体数据([]byte方式)

	Recv() (packtype uint8, sequence, uid uint32, extra, msg []byte, err error) // 接收文本,二进制数据
	RecvMediaKmBuf() (buf *kmpool.KmBuf, err error)                             // 接收媒体数据(内存池方式)
	RecvMediaByte() (frame []byte, err error)                                   // 接收媒体数据([]byte方式)
}
