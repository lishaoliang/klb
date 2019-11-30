///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	pool.go
/// @author	lishaoliang
/// @brief	pool
///////////////////////////////////////////////////////////////////////////
package kmpool

import (
	"sync/atomic"
)

// kmPool fix mem pool
type kmPool struct {
	Pool

	idxs []KmBuf
	mem  []byte

	blockSize int
	blockNum  int
}

func kmPoolInit(pool *kmPool, blockSize, blockNum int) {
	size64 := int64(blockSize)
	num64 := int64(blockNum)

	pool.blockSize = blockSize
	pool.blockNum = blockNum

	pool.mem = make([]byte, size64*num64, size64*num64)
	pool.idxs = make([]KmBuf, pool.blockNum, pool.blockNum)

	for i := 0; i < pool.blockNum; i++ {
		pool.idxs[i].id = i
		pool.idxs[i].pool = pool
		pool.idxs[i].buf = pool.mem[size64*int64(i) : size64*int64(i+1)]
		pool.idxs[i].bufLen = pool.blockSize
	}
}

func kmPoolQuit(pool *kmPool) {
	pool.blockNum = 0
	pool.blockSize = 0
	pool.mem = nil
	pool.idxs = nil
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
// 销毁
func (m *kmPool) Destroy() {
	kmPoolQuit(m)
}

// Malloc malloc
// 申请内存
func (m *kmPool) Malloc() *KmBuf {

	for i := 0; i < m.blockNum; i++ {
		buf := &m.idxs[i]
		if 0 == atomic.LoadInt32(&buf.atomicUse) {
			buf.use()
			buf.Ref()
			return buf
		}
	}

	return nil
}

// BlockSize block size
// 块大小
func (m *kmPool) BlockSize() int {
	return m.blockSize
}

// BlockNum block num
// 总块数目
func (m *kmPool) BlockNum() int {
	return m.blockNum
}
