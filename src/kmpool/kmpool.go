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
