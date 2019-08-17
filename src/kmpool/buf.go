package kmpool

import "sync/atomic"

// KmBuf buf
type KmBuf struct {
	atomic int32

	pool *KmPool
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
func (kb *KmBuf) GetID() int {
	return kb.id
}

// GetBuf get buf
func (kb *KmBuf) GetBuf() []byte {
	return kb.buf
}

// GetBufLen get buf len
func (kb *KmBuf) GetBufLen() int {
	return kb.bufLen
}

// Ref ref
func (kb *KmBuf) Ref() int32 {
	return atomic.AddInt32(&kb.atomic, 1)
}

// Unref unref
func (kb *KmBuf) Unref() int32 {
	n := atomic.AddInt32(&kb.atomic, -1)

	if n <= 0 {
		kb.Start = 0
		kb.End = 0
		kb.Prev = nil
		kb.Next = nil
		kb.X = 0
		kb.Y = 0
		kb.Z = 0
		kb.A = 0

		atomic.StoreInt32(&kb.atomic, 0)
	}

	return n
}
