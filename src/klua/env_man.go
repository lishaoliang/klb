///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    env_man.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	envMan
///////////////////////////////////////////////////////////////////////////
package klua

import (
	"sync"
)

// 存储所有记名*envOne,并映射string;
// 通过string可查找对应的*envOne
type envMan struct {
	m     map[string]*envOne
	mutex sync.Mutex
}

// 模块唯一
var gEnvMan envMan

///////////////////////////////////////////////////////////////////////////////

// 初始化
func envManInit() {
	gEnvMan.m = make(map[string]*envOne)
}

func envManPush(name string, e *envOne) bool {
	gEnvMan.mutex.Lock()
	defer gEnvMan.mutex.Unlock()

	b := false

	if nil == gEnvMan.m[name] {
		gEnvMan.m[name] = e
		b = true
	}

	return b
}

func envManRemove(name string) {
	gEnvMan.mutex.Lock()
	defer gEnvMan.mutex.Unlock()

	delete(gEnvMan.m, name)
}

func envManFind(name string) *envOne {
	gEnvMan.mutex.Lock()
	defer gEnvMan.mutex.Unlock()

	eo := gEnvMan.m[name]
	return eo
}
