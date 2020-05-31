package knet

import (
	"fmt"
	"regexp"
	"sync"
)

var gPathMap map[string]string // HTTP路径映射协议
var gPathMutex sync.Mutex      // 锁

// RegisterProtocol register protocol path
// @brief 注册创建连接回调函数
// @param [in] protocol 协议名称
// @param [in] path 路径
// @return 无
func RegisterProtocol(protocol, path string) {
	gPathMutex.Lock()
	defer gPathMutex.Unlock()

	gPathMap[path] = protocol
}

// PathIsProtocol path is protocol
// @brief 从路径获取协议
// @param [in] path 路径
// @return 无
func PathIsProtocol(path string) string {
	gPathMutex.Lock()
	defer gPathMutex.Unlock()

	for k, v := range gPathMap {
		re := regexp.MustCompile(fmt.Sprintf(`%s`, k))
		s := re.FindString(path)

		if 0 < len(s) {
			return v
		}
	}

	return ""
}
