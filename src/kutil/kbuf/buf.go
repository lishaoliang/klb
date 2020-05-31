package kbuf

import (
	"sync/atomic"

	"github.com/lishaoliang/klb/src/kutil"
)

// MallocCb malloc callback
// 申请缓存函数
type MallocCb func() KBuf

// MallocCb1 malloc callback with param len
// 申请缓存函数: 使用最少需求长度
type MallocCb1 func(buflen int) KBuf

// MallocByCb1 malloc callback with param []byte
// 申请缓存函数: 直接使已有[]byte
type MallocByCb1 func(buf []byte) KBuf

// KBuf K Buf
// 带引用计数的双向链表缓存
type KBuf interface {
	ID() int    // get id, 获取序号
	Prev() KBuf // get prev, 获取前一个节点
	Next() KBuf // get next, 获取下一个节点

	Ref()       // 计数++
	Unref()     // 计数--
	RefNext()   // 从当前至尾, 计数++
	UnrefNext() // 从当前至尾, 计数--
	RefAll()    // 从头至尾, 计数++
	UnrefAll()  // 从头至尾, 计数--

	Buf() []byte               // get buffer, 所有缓存切片
	BufLen() int               // get buffer len, 缓存大小
	Data() []byte              // get data, 获取有效数据切片
	Pos() (start int, end int) // get pos, 获取数据起始/结束位置
	Spare() []byte             // get spare buffer, 获取剩余缓存
	Add(n int)                 // add end pos, 结束位置加n
	IsFull() bool              // 是否满了

	SetPos(start, end int) // 设置位置
	SetStart(start int)    // 设置起始位置(非线程安全)
	SetEnd(end int)        // 设置结束位置(非线程安全)

	SetPrev(prev KBuf) // 设置前一个节点(非线程安全)
	SetNext(next KBuf) // 设置下一个节点(非线程安全)
}

// Malloc malloc KBuf memory
// 申请KBuf格式内存
type Malloc interface {
	Malloc() KBuf // 申请内存
}

// kbuf K Buf
// 从系统内存直接申请
type kbuf struct {
	atomicCount int32 // 计数器

	prev KBuf // 前一节点
	next KBuf // 下一个节点

	id    int    // 序号
	start int    // 数据起始位置
	end   int    // 数据结束位置
	buf   []byte // 缓存
}

// New K Buf
// 从系统内存直接申请缓存
func New(buflen int) KBuf {
	var m kbuf

	m.id = 0
	m.buf = make([]byte, buflen)

	m.Ref()
	return &m
}

// NewBy K Buf
// 沿用已经有的缓存
func NewBy(buf []byte) KBuf {
	var m kbuf

	m.id = 0
	m.buf = buf

	m.Ref()
	return &m
}

// ID get id
func (m *kbuf) ID() int {
	return m.id
}

// Prev get prev
func (m *kbuf) Prev() KBuf {
	return m.prev
}

// Next get next
func (m *kbuf) Next() KBuf {
	return m.next
}

// Ref ref
func (m *kbuf) Ref() {
	atomic.AddInt32(&m.atomicCount, 1)
}

// Unref un ref
func (m *kbuf) Unref() {
	n := atomic.AddInt32(&m.atomicCount, -1)

	if n <= 0 {
		kutil.Assert(0 == n)

		m.prev = nil
		m.next = nil

		m.id = 0
		m.start = 0
		m.end = 0
		m.buf = nil
	}
}

// RefNext ref next
// 从当前节点到末尾全部做Ref动作
func (m *kbuf) RefNext() {
	cur := KBuf(m)
	for nil != cur {
		next := cur.Next()
		cur.Ref()

		cur = next
	}
}

// UnrefNext unref next
// 从当前节点到末尾全部做Unref动作
func (m *kbuf) UnrefNext() {
	cur := KBuf(m)
	for nil != cur {
		next := cur.Next()
		cur.Unref()

		cur = next
	}
}

// RefAll ref all
// 从根节点到末尾全部做ref动作
func (m *kbuf) RefAll() {
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
func (m *kbuf) UnrefAll() {
	cur := m.Prev()
	for nil != cur {
		prev := cur.Prev()
		cur.Unref()

		cur = prev
	}

	m.UnrefNext()
}

// Buf get buf
func (m *kbuf) Buf() []byte {
	return m.buf
}

// BufLen get buf len
func (m *kbuf) BufLen() int {
	return len(m.buf)
}

// Data get data
func (m *kbuf) Data() []byte {
	return m.buf[m.start:m.end]
}

// Pos get pos
func (m *kbuf) Pos() (start int, end int) {
	return m.start, m.end
}

// Spare get spare buffer
// 获取剩余缓存
func (m *kbuf) Spare() []byte {
	return m.buf[m.end:]
}

// Add add end pos
// 结束位置加n
func (m *kbuf) Add(n int) {
	m.end += n
	m.check()
}

// IsFull is full?
// 是否满了
func (m *kbuf) IsFull() bool {
	return m.end == len(m.buf)
}

// SetPos set pos, start end
func (m *kbuf) SetPos(start, end int) {
	m.start = start
	m.end = end
	m.check()
}

// SetStart set start
func (m *kbuf) SetStart(start int) {
	m.start = start
	m.check()
}

// SetEnd set end
func (m *kbuf) SetEnd(end int) {
	m.end = end
	m.check()
}

// SetPrev set prev
func (m *kbuf) SetPrev(prev KBuf) {
	m.prev = prev
}

// SetNext set next
func (m *kbuf) SetNext(next KBuf) {
	m.next = next
}

///////////////////////////////////////////////////////////

func (m *kbuf) check() {
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
