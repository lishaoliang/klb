package kpool

import (
	"sync/atomic"

	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// KPool memory pool
// 固定内存大小的内存池
type KPool interface {
	Destroy()          // 销毁
	Malloc() kbuf.KBuf // 申请内存
}

// pool pool
type pool struct {
	idxs []pbuf
	mem  []byte

	blockSize int
	blockNum  int
}

// Destroy Destroy
func (m *pool) Destroy() {

	// 退出时, 检查所有节点状态
	for i := 0; i < m.blockNum; i++ {
		buf := &m.idxs[i]

		kutil.Assert(0 == atomic.LoadInt32(&buf.atomicUse))
		kutil.Assert(0 == atomic.LoadInt32(&buf.atomicCount))
	}

	m.idxs = nil
	m.mem = nil
	m.blockSize = 0
	m.blockNum = 0
}

// Malloc Malloc
func (m *pool) Malloc() kbuf.KBuf {
	// 按数组方式, 仅适用于 blockNum 数据量较少时使用
	for i := 0; i < m.blockNum; i++ {
		buf := &m.idxs[i]
		if atomic.CompareAndSwapInt32(&buf.atomicUse, 0, 1) {
			buf.Ref()
			return buf
		}
	}

	return nil
}

/////////////////////////////////////////////////////////////////////////////////////

func (m *pool) init(blockSize, blockNum int) {
	size64 := int64(blockSize)
	num64 := int64(blockNum)

	m.blockSize = blockSize
	m.blockNum = blockNum

	m.mem = make([]byte, size64*num64, size64*num64)
	m.idxs = make([]pbuf, m.blockNum, m.blockNum)

	for i := 0; i < m.blockNum; i++ {
		m.idxs[i].id = i
		m.idxs[i].buf = m.mem[size64*int64(i) : size64*int64(i+1)]
	}
}
