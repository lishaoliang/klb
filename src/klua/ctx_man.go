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
	"math/rand"
	"sync"
	"time"
)

// 存储所有*Ctx,并映射string;
// 通过string可查找对应的*Ctx
type ctxMan struct {
	ctx    context.Context    // 上下文环境
	cancel context.CancelFunc // 取消
	wg     sync.WaitGroup     // 等待组

	mapCtx map[string]*Ctx // 集合*Ctx
	mutex  sync.Mutex      // 锁

	r        *rand.Rand // 随机值
	bytes    []byte     // 字符集[A-Za-z0-9]
	bytesLen int        // bytes长度
}

// 模块唯一
var gCtxMan ctxMan

///////////////////////////////////////////////////////////////////////////////

// 初始化
func ctxManInit() {
	gCtxMan.ctx, gCtxMan.cancel = context.WithCancel(context.Background())

	gCtxMan.r = rand.New(rand.NewSource(time.Now().UnixNano()))

	chs := "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
	gCtxMan.bytes = []byte(chs)
	gCtxMan.bytesLen = len(gCtxMan.bytes)

	gCtxMan.mapCtx = make(map[string]*Ctx)
}

func randStr(sl int) string {
	ret := []byte{}
	for i := 0; i < sl; i++ {
		ret = append(ret, gCtxMan.bytes[gCtxMan.r.Intn(gCtxMan.bytesLen)])
	}

	return string(ret)
}

// 存储*Ctx
func ctxManPush(s *Ctx) string {

	var name string
	for true {
		name = randStr(8)

		if nil == gCtxMan.mapCtx[name] {
			break
		}
	}

	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	gCtxMan.mapCtx[name] = s
	return name
}

// 通过名称移除*Ctx
func ctxManRemove(name string) {
	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	delete(gCtxMan.mapCtx, name)
}

// 通过名称查找*Ctx
func ctxManFind(name string) *Ctx {
	gCtxMan.mutex.Lock()
	defer gCtxMan.mutex.Unlock()

	ctx := gCtxMan.mapCtx[name]
	return ctx
}
