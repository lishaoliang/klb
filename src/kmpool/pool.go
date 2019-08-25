///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/17
//
/// @file    pool.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	pool
///////////////////////////////////////////////////////////////////////////
package kmpool

import "sync/atomic"

// KmPool fix mem pool
type KmPool struct {
	idxs []KmBuf
	mem  []byte

	blockSize int
	blockNum  int
}

func kmPoolInit(pool *KmPool, blockSize, blockNum int) {
	size64 := int64(blockSize)
	num64 := int64(blockNum)

	pool.blockSize = blockSize
	pool.blockNum = blockNum

	pool.mem = make([]byte, size64*num64)
	pool.idxs = make([]KmBuf, pool.blockNum)

	for i := 0; i < pool.blockNum; i++ {
		pool.idxs[i].id = i
		pool.idxs[i].pool = pool
		pool.idxs[i].buf = pool.mem[size64*int64(i) : size64*int64(i+1)]
		pool.idxs[i].bufLen = pool.blockSize
	}
}

func kmPoolQuit(pool *KmPool) {
	pool.blockNum = 0
	pool.blockSize = 0
	pool.mem = nil
	pool.idxs = nil
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (kp *KmPool) Destroy() {
	kmPoolQuit(kp)
}

// Malloc malloc
func (kp *KmPool) Malloc() *KmBuf {

	for i := 0; i < kp.blockNum; i++ {
		buf := &kp.idxs[i]
		if 0 == atomic.LoadInt32(&buf.atomic) {
			buf.Ref()
			return buf
		}
	}

	return nil
}
