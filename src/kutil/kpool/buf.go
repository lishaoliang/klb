package kpool

import (
	"sync/atomic"

	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// pbuf pool buf, kbuf.KBuf
// 从系统内存直接申请
type pbuf struct {
	atomicUse   int32 // 是否被使用; 0.未使用; 1.使用中
	atomicCount int32 // 计数器

	prev kbuf.KBuf // 前一节点
	next kbuf.KBuf // 下一个节点

	id    int    // 序号
	start int    // 数据起始位置
	end   int    // 数据结束位置
	buf   []byte // 缓存
}

// ID get id
func (m *pbuf) ID() int {
	return m.id
}

// Prev get prev
func (m *pbuf) Prev() kbuf.KBuf {
	return m.prev
}

// Next get next
func (m *pbuf) Next() kbuf.KBuf {
	return m.next
}

// Ref ref
func (m *pbuf) Ref() {
	atomic.AddInt32(&m.atomicCount, 1)
}

// Unref un ref
func (m *pbuf) Unref() {
	n := atomic.AddInt32(&m.atomicCount, -1)

	if n <= 0 {
		kutil.Assert(0 == n)

		m.prev = nil
		m.next = nil

		m.start = 0
		m.end = 0

		atomic.StoreInt32(&m.atomicCount, 0) // 计数归零
		atomic.StoreInt32(&m.atomicUse, 0)   // 归还内存池
	}
}

// RefNext ref next
// 从当前节点到末尾全部做Ref动作
func (m *pbuf) RefNext() {
	cur := kbuf.KBuf(m)
	for nil != cur {
		next := cur.Next()
		cur.Ref()

		cur = next
	}
}

// UnrefNext unref next
// 从当前节点到末尾全部做Unref动作
func (m *pbuf) UnrefNext() {
	cur := kbuf.KBuf(m)
	for nil != cur {
		next := cur.Next()
		cur.Unref()

		cur = next
	}
}

// RefAll ref all
// 从根节点到末尾全部做ref动作
func (m *pbuf) RefAll() {
	cur := m.Prev()
	for nil != cur {
		prev := cur.Prev()
		cur.Ref()

		cur = prev
	}

	m.RefNext()
}

// UnrefAll unref all
// 从根节点到末尾全部做Unref动作
func (m *pbuf) UnrefAll() {
	cur := m.Prev()
	for nil != cur {
		prev := cur.Prev()
		cur.Unref()

		cur = prev
	}

	m.UnrefNext()
}

// Buf get buf
func (m *pbuf) Buf() []byte {
	return m.buf
}

// BufLen get buf len
func (m *pbuf) BufLen() int {
	return len(m.buf)
}

// Data get data
func (m *pbuf) Data() []byte {
	return m.buf[m.start:m.end]
}

// Pos get pos
func (m *pbuf) Pos() (start int, end int) {
	return m.start, m.end
}

// Spare get spare buffer
// 获取剩余缓存
func (m *pbuf) Spare() []byte {
	return m.buf[m.end:]
}

// Add add end pos
// 结束位置加n
func (m *pbuf) Add(n int) {
	m.end += n
	m.check()
}

// IsFull is full?
// 是否满了
func (m *pbuf) IsFull() bool {
	return m.end == len(m.buf)
}

// SetPos set pos, start end
func (m *pbuf) SetPos(start, end int) {
	m.start = start
	m.end = end
	m.check()
}

// SetStart set start
func (m *pbuf) SetStart(start int) {
	m.start = start
	m.check()
}

// SetEnd set end
func (m *pbuf) SetEnd(end int) {
	m.end = end
	m.check()
}

// SetPrev set prev
func (m *pbuf) SetPrev(prev kbuf.KBuf) {
	m.prev = prev
}

// SetNext set next
func (m *pbuf) SetNext(next kbuf.KBuf) {
	m.next = next
}

///////////////////////////////////////////////////////////

func (m *pbuf) check() {
	l := len(m.buf)

	if l < m.start {
		m.start = l
		kutil.Assert(false)
	}

	if l < m.end {
		m.end = l
		kutil.Assert(false)
	}
}
