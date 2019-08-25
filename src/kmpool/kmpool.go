///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/17
//
/// @file    kmpool.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	kmpool
///////////////////////////////////////////////////////////////////////////
package kmpool

var gKmPool KmPool

// init
func init() {
	// init
}

// PoolInit pool init
func PoolInit(blockSize, blockNum int) {
	kmPoolInit(&gKmPool, blockSize, blockNum)
}

// PoolQuit pool quit
func PoolQuit() {
	kmPoolQuit(&gKmPool)
}

// Malloc malloc
func Malloc() *KmBuf {
	return gKmPool.Malloc()
}

// PoolCreate pool create
func PoolCreate(blockSize, blockNum int) *KmPool {
	var kp KmPool
	kmPoolInit(&kp, blockSize, blockNum)

	return &kp
}
