///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   buf.go
/// @author lishaoliang
/// @brief	buf
///////////////////////////////////////////////////////////////////////////

package kutil

import "errors"

// Buf buf
// buf 常规操作
type Buf struct {
	buf    []byte // 缓存切片
	bufLen int    // 缓存长度
	pos    int    // 当前位置
}

// BufCreate create buf
// 创建缓存
func BufCreate(bufLen int) *Buf {
	var b Buf
	b.bufLen = bufLen
	b.buf = make([]byte, bufLen)
	b.pos = 0

	return &b
}

// Destroy destory
// 销毁
func (m *Buf) Destroy() {
	m.buf = nil
	m.bufLen = 0
	m.pos = 0
}

// Reset reset
// 复位
func (m *Buf) Reset() {
	m.pos = 0
}

// Data data
// 有效数据
func (m *Buf) Data() []byte {
	return m.buf[:m.pos]
}

// LeftBuf buf left
// 剩余缓存
func (m *Buf) LeftBuf() []byte {
	return m.buf[m.pos:]
}

// LeftBufBy buf left by
// 按数量获取剩余缓存
func (m *Buf) LeftBufBy(num int) []byte {
	e := m.pos + num
	if m.bufLen < e {
		e = m.bufLen
	}

	return m.buf[m.pos:e]
}

// Pos pos
// 获取当前位置
func (m *Buf) Pos() int {
	return m.pos
}

// AddPos add pos
// 添加缓存指示位置
func (m *Buf) AddPos(n int) error {
	m.pos += n

	if m.bufLen < m.pos {
		m.pos = m.bufLen
		return errors.New("kutil.buf.AddPos,error!out of range")
	}

	return nil
}

// IsFull is full
// 是否满了
func (m *Buf) IsFull() bool {
	return m.pos == m.bufLen
}
