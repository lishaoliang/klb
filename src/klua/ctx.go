///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	ctx.go
/// @author	lishaoliang
/// @brief	Ctx
///////////////////////////////////////////////////////////////////////////

package klua

import (
	"context"
	"sync"
	"time"
)

// ctxMsg msg []byte
// []byte消息
type ctxMsg struct {
	msg    []byte // message[string]
	msgex  []byte // message ext[string]
	lparam []byte // l param[string]
	wparam []byte // w param[string]
}

// Ctx ctx
type Ctx struct {
	ctx    context.Context    // 上下文环境
	cancel context.CancelFunc // 取消
	wg     sync.WaitGroup     // 等待组

	env  *Env        // CGO Env(Lua对接C环境)
	name string      // 本Ctx线程名称
	msgs chan ctxMsg // 传递过来的消息
}

// ctxCreate create Ctx
func ctxCreate(name, preLoad, loader string) *Ctx {
	var m Ctx

	m.ctx, m.cancel = context.WithCancel(context.Background())

	cbPreLoad := gMapFuncs.FindPreload(preLoad)
	cbLoader := gMapFuncs.FindLoader(loader)

	m.name = name
	m.msgs = make(chan ctxMsg)
	m.env = EnvCreate("", cbPreLoad, cbLoader)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// destroy destroy
func (m *Ctx) destroy() {

	// 等待线程退出
	m.cancel()
	m.wg.Wait()

	// 销毁lua环境
	//m.env.Destroy()

	close(m.msgs)

	m.env = nil
	m.msgs = nil
}

// GetName get ctx name
func (m *Ctx) GetName() string {
	return m.name
}

// GetEnv get env
func (m *Ctx) GetEnv() *Env {
	return m.env
}

// DoFile worker do file
func (m *Ctx) DoFile(loader string) (bool, error) {
	gCtxMan.wg.Add(1)
	m.wg.Add(1)
	go m.worker(gCtxMan.ctx, &gCtxMan.wg, loader, true)

	return true, nil
}

// DoLibrary worker do library
func (m *Ctx) DoLibrary(loader string) (bool, error) {
	gCtxMan.wg.Add(1)
	m.wg.Add(1)
	go m.worker(gCtxMan.ctx, &gCtxMan.wg, loader, false)

	return true, nil
}

// Post post []byte
func (m *Ctx) Post(msg, msgex, lparam, wparam []byte) {
	var cm ctxMsg
	cm.msg = msg
	cm.msgex = msgex
	cm.lparam = lparam
	cm.wparam = wparam

	m.msgs <- cm
}

///////////////////////////////////////////////////////////////////////////////

func (m *Ctx) worker(ctx context.Context, wg *sync.WaitGroup, loader string, bDoFile bool) error {
	defer wg.Done()
	defer m.wg.Done()
	defer m.env.Destroy()

	env := m.env

	if bDoFile {
		env.DoFile(loader)
	} else {
		env.DoLibrary(loader)
	}

	tick := time.NewTicker(time.Millisecond * 10)
	defer tick.Stop()

	for {
		select {
		case msg := <-m.msgs:
			env.CallKgoB(msg.msg, msg.msgex, msg.lparam, msg.wparam, nil) // post []byte
		case <-tick.C:
			env.LoopOnce()

			if env.IsExit() {
				env.DoEnd()
				return ctx.Err() // 超时或强制退出
			}
		case <-ctx.Done():
			env.DoEnd()
			return ctx.Err() // 超时或强制退出
		case <-m.ctx.Done():
			env.DoEnd()
			return ctx.Err() // 超时或强制退出
		}
	}
}
