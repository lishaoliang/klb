///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	ctx_man.go
/// @author	lishaoliang
/// @brief	ctxMan
///////////////////////////////////////////////////////////////////////////

package klua

import (
	"context"
	"fmt"
	"sync"

	"github.com/lishaoliang/klb/src/kutil"
)

// 存储所有*Ctx,并映射string;
// 通过string可查找对应的*Ctx
type ctxMan struct {
	ctx    context.Context    // 上下文环境
	cancel context.CancelFunc // 取消
	wg     sync.WaitGroup     // 等待组
	rand   *kutil.Rand        // 随机字符串

	mapCtx map[string]*Ctx // 集合*Ctx
	mutex  sync.Mutex      // 锁
}

// 模块唯一
var gCtxMan ctxMan

///////////////////////////////////////////////////////////////////////////////

// 初始化
func ctxManInit() {
	gCtxMan.ctx, gCtxMan.cancel = context.WithCancel(context.Background())

	gCtxMan.rand = kutil.NewRand()

	gCtxMan.mapCtx = make(map[string]*Ctx)
}

// Destroy destroy
func (m *ctxMan) Destroy() {
	//m.rand.Destroy()

	m.rand = nil
	m.mapCtx = nil
}

// Wait wait
func (m *ctxMan) Wait() {
	m.wg.Wait()
}

// Cancel cancel
func (m *ctxMan) Cancel() {
	m.cancel()
}

// Open
func (m *ctxMan) Open(name, entry, preload, loader string, bDoFile bool) (*Ctx, error) {
	ctx := ctxCreate(name, preload, loader)

	m.mutex.Lock()
	defer m.mutex.Unlock()

	if nil != m.mapCtx[name] {
		return nil, fmt.Errorf("klua.ctx_man.Open error!name=\"%s\" repeat", name)
	}

	m.mapCtx[name] = ctx // 放入ctx集合

	if bDoFile {
		ctx.DoFile(entry)
	} else {
		ctx.DoLibrary(entry)
	}

	return ctx, nil
}

// Close close
func (m *ctxMan) Close(name string) bool {

	m.mutex.Lock()
	ctx := m.mapCtx[name]
	if nil != ctx {
		delete(m.mapCtx, name)
	}
	m.mutex.Unlock()

	if nil != ctx {
		// 关闭Ctx耗时
		// 从map中移除之后, 另开线程等待关闭结束
		//go func(c *Ctx) {
		//	c.destroy()
		//}(ctx)
		ctx.destroy()

		return true
	}

	return false
}

// Post post
func (m *ctxMan) Post(name string, msg, msgex, lparam, wparam []byte) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	ctx := m.mapCtx[name]

	if nil != ctx {
		ctx.Post(msg, msgex, lparam, wparam)
		return true
	}

	return false
}
