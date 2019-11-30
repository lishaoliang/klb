///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	buf.go
/// @author	lishaoliang
/// @brief	mem pool buf
///////////////////////////////////////////////////////////////////////////
package kmpool

import (
	"sync/atomic"

	"github.com/lishaoliang/klb/src/kutil"
)

// KmBuf buf
// 单个缓存块
type KmBuf struct {
	atomicUse   int32 // 是否被使用; 0.未使用; 1.使用中
	atomicCount int32 // 使用次数

	pool Pool
	id   int

	buf    []byte
	bufLen int

	Start int
	End   int

	Prev *KmBuf
	Next *KmBuf

	X uint32
	Y uint32
	Z uint32
	A uint32
}

///////////////////////////////////////////////////////////////////////////////

// GetID get id
// 缓存块id
func (m *KmBuf) GetID() int {
	return m.id
}

// GetBuf get buf
// 获取缓存切片
func (m *KmBuf) GetBuf() []byte {
	return m.buf
}

// GetBufLen get buf len
// 获取缓存长度
func (m *KmBuf) GetBufLen() int {
	return m.bufLen
}

// Data data
// 获取有效数据切片
func (m *KmBuf) Data() []byte {
	return m.buf[m.Start:m.End]
}

// use
func (m *KmBuf) use() {
	atomic.StoreInt32(&m.atomicUse, 1)
}

// unUse
func (m *KmBuf) unUse() {
	atomic.StoreInt32(&m.atomicUse, 0)
}

// Ref ref
// 计数加一
func (m *KmBuf) Ref() int32 {
	return atomic.AddInt32(&m.atomicCount, 1)
}

// Unref unref
// 计数减一
func (m *KmBuf) Unref() int32 {
	n := atomic.AddInt32(&m.atomicCount, -1)

	if n <= 0 {
		kutil.Assert(0 == n)

		m.Start = 0
		m.End = 0
		m.Prev = nil
		m.Next = nil
		m.X = 0
		m.Y = 0
		m.Z = 0
		m.A = 0

		atomic.StoreInt32(&m.atomicCount, 0)
		m.unUse()
	}

	return n
}

// RefAll ref all
// 从当前节点开始的节点计数加一
func (m *KmBuf) RefAll() {
	cur := m
	for nil != cur {
		next := cur.Next
		cur.Ref()

		cur = next
	}
}

// UnrefAll unref all
// 从当前节点开始的节点计数减一
func (m *KmBuf) UnrefAll() {
	cur := m
	for nil != cur {
		next := cur.Next
		cur.Unref()

		cur = next
	}
}
