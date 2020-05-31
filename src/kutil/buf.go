///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   buf.go
/// @author lishaoliang
/// @brief	buf
///////////////////////////////////////////////////////////////////////////

package kutil

/*
#include "string.h"
#include "stdlib.h"
*/
import "C"

import (
	"unsafe"
)

// Buf buf
// buf 常规操作
type Buf struct {
	buf []byte // 缓存切片
	pos int    // 当前位置
}

// NewBuf new Buf
// 创建缓存
func NewBuf(bufLen int) *Buf {
	var m Buf

	m.buf = make([]byte, bufLen)
	m.pos = 0

	return &m
}

// Drop Drop
// 放弃数据
func (m *Buf) Drop() []byte {
	d := m.Data()

	m.buf = nil
	m.pos = 0

	return d
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

// Spare spare buffer
// 剩余缓存
func (m *Buf) Spare() []byte {
	return m.buf[m.pos:]
}

// SpareBy spare buffer by num
// 按数量获取剩余缓存
func (m *Buf) SpareBy(num int) []byte {
	bufLen := len(m.buf)
	e := m.pos + num
	if bufLen < e {
		e = bufLen
	}

	return m.buf[m.pos:e]
}

// Pos pos
// 获取当前位置
func (m *Buf) Pos() int {
	return m.pos
}

// Add add pos
// 添加缓存指示位置
func (m *Buf) Add(n int) {
	m.pos += n

	bufLen := len(m.buf)
	if bufLen < m.pos {
		m.pos = bufLen
	}
}

// IsFull is full
// 是否满了
func (m *Buf) IsFull() bool {
	return m.pos == len(m.buf)
}

// Memmove memmove
func (m *Buf) Memmove(pos int) {
	if m.pos <= pos {
		m.pos = 0
	} else if pos <= 0 {
		// do not
	} else {
		l := m.pos - pos
		C.memmove(unsafe.Pointer(&m.buf[0]), unsafe.Pointer(&m.buf[pos]), C.size_t(l))
		m.pos = l
	}
}
