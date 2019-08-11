///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    ctx_man.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	ctxMan
///////////////////////////////////////////////////////////////////////////
package klua

import (
	"context"
	"sync"
)

// 存储所有*Ctx,并映射string;
// 通过string可查找对应的*Ctx
type ctxMan struct {
	ctx    context.Context
	cancel context.CancelFunc
	wg     sync.WaitGroup

	m     map[string]*Ctx
	mutex sync.Mutex
}

// 模块唯一
var gCtxMan ctxMan

///////////////////////////////////////////////////////////////////////////////

// 初始化
func ctxManInit() {
	gCtxMan.ctx, gCtxMan.cancel = context.WithCancel(context.Background())

	gCtxMan.m = make(map[string]*Ctx)
}

// 存储*Ctx
func ctxManPush(s *Ctx) string {

	var name string
	for true {
		name = comRandStr(8)

		if nil == gCtxMan.m[name] {
			break
		}
	}

	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	gCtxMan.m[name] = s
	return name
}

// 通过名称移除*Ctx
func ctxManRemove(name string) {
	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	delete(gCtxMan.m, name)
}

// 通过名称查找*Ctx
func ctxManFind(name string) *Ctx {
	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	ctx := gCtxMan.m[name]
	return ctx
}
