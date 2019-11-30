///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	kmpool.go
/// @author	lishaoliang
/// @brief	k mem pool
///////////////////////////////////////////////////////////////////////////
package kmpool

var gKmPool kmPool

// init
func init() {
	// init
}

// Pool Pool
// 内存池
type Pool interface {
	Destroy()       // 销毁
	Malloc() *KmBuf // 申请内存
	BlockSize() int // 块大小
	BlockNum() int  // 总块数目
}

// PoolInit pool init
// 初始化默认池
func PoolInit(blockSize, blockNum int) {
	kmPoolInit(&gKmPool, blockSize, blockNum)
}

// PoolQuit pool quit
// 默认池退出
func PoolQuit() {
	kmPoolQuit(&gKmPool)
}

// Malloc malloc
// 从默认池中申请缓存
func Malloc() *KmBuf {
	return gKmPool.Malloc()
}

// PoolCreate pool create
// 创建内存池
func PoolCreate(blockSize, blockNum int) Pool {
	var kp kmPool
	kmPoolInit(&kp, blockSize, blockNum)

	return &kp
}
