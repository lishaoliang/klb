package kpool

import "github.com/lishaoliang/klb/src/kutil/kbuf"

var gPool pool

// init
func init() {
	// init
}

// Init Init
func Init(blockSize, blockNum int) {
	gPool.init(blockSize, blockNum)
}

// Quit Quit
func Quit() {
	gPool.Destroy()
}

// Default get default pool
func Default() KPool {
	return &gPool
}

// Malloc Malloc
func Malloc() kbuf.KBuf {
	return gPool.Malloc()
}

// Create Create KPool
func Create(blockSize, blockNum int) KPool {
	var p pool
	p.init(blockSize, blockNum)
	return &p
}
