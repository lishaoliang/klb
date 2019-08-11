package klua

import (
	"math/rand"
	"time"
)

// klua模块内部基础公共函数
type com struct {
	r *rand.Rand
}

var gCom com

// 初始化
func comInit() {
	gCom.r = rand.New(rand.NewSource(time.Now().UnixNano()))
}

// 随机长度字符串
func comRandStr(l int) string {
	chs := "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	bytes := []byte(chs)

	ret := []byte{}
	for i := 0; i < l; i++ {
		ret = append(ret, bytes[gCom.r.Intn(len(bytes))])
	}

	return string(ret)
}
