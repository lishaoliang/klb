///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   ser_conn.go
/// @author lishaoliang
/// @brief	ser conn
///////////////////////////////////////////////////////////////////////////

package kamdc

import (
	"fmt"

	"github.com/lishaoliang/klb/src/kbuf/kmpool"
	"github.com/lishaoliang/klb/src/knet/kmnp"
)

// serConn
// 创建服务端连接环境
type serConn struct {
	conn kmnp.ConnInterface

	stream map[string]bool // 打开的码流
}

// serConnCreate server connect
// 创建服务端连接环境
func serConnCreate(conn kmnp.ConnInterface) *serConn {
	var m serConn
	m.conn = conn
	m.stream = make(map[string]bool)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *serConn) Destroy() {
	m.conn.Stop()
	m.conn.Destroy()

	m.stream = nil
}

// Start start
func (m *serConn) Start() {
	m.conn.Start()
}

// SetProtocol set protol
// 设置协议
func (m *serConn) SetProtocol(protocol string) {
	m.conn.SetProtocol(protocol)
}

// SetName set name
// 设置名称
func (m *serConn) SetName(name string) {
	m.conn.SetName(name)
}

// OpenStream open stream
// 打开流
func (m *serConn) OpenStream(chnn, sidx int) {
	// 过滤 chnn,sidx; 防止map插入过多数据

	key := m.getStreamKey(chnn, sidx)
	m.stream[key] = true
}

// CloseStream close stream
// 关闭流
func (m *serConn) CloseStream(chnn, sidx int) {
	key := m.getStreamKey(chnn, sidx)
	delete(m.stream, key)
}

// HasStream
// 是否需要发送流
func (m *serConn) HasStream() bool {
	if len(m.stream) <= 0 {
		return false
	}

	return true
}

// SendTxt send text
// 发送文本数据
func (m *serConn) SendTxt(sequence, uid int64, extra, msg []byte) bool {
	return m.conn.SendTxt((uint32)(sequence), (uint32)(uid), extra, msg)
}

// SendMedia send media
// 向连接发送媒体数据
func (m *serConn) SendMedia(chnn, sidx int, buf *kmpool.KmBuf) bool {
	key := m.getStreamKey(chnn, sidx)
	if m.stream[key] {
		return m.conn.SendMediaKmBuf(buf)
	}

	return false
}

///////////////////////////////////////////////////////////////////////////////

// getStreamKey get stream key
// 从chnn,sidx转换为key
func (m *serConn) getStreamKey(chnn, sidx int) string {
	return fmt.Sprintf("s:%d:%d", chnn, sidx)
}
