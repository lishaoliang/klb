///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    ctx.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	Ctx
///////////////////////////////////////////////////////////////////////////
package klua

import (
	"context"
	"sync"
	"unsafe"
)

type ctxMsg struct {
	msg    string
	msgex  string
	lparam string
	wparam string
	ptr    unsafe.Pointer
}

type ctxMsgB struct {
	msg    []byte
	msgex  []byte
	lparam []byte
	wparam []byte
	ptr    unsafe.Pointer
}

// Ctx ctx
type Ctx struct {
	env     *Env         // CGO Env(Lua对接C环境)
	preLoad LuaCFunction // 预加载函数

	name string       // 本Ctx名称
	msgs chan ctxMsg  // 传递过来的消息
	msgB chan ctxMsgB // 传递过来的[]byte
}

// CtxCreate create Ctx
func CtxCreate(preLoad LuaCFunction) *Ctx {
	var s Ctx

	// push ctxMan
	s.name = ctxManPush(&s)

	s.preLoad = preLoad

	s.msgs = make(chan ctxMsg)
	s.msgB = make(chan ctxMsgB)
	s.env = EnvCreate(s.name, preLoad)

	return &s
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (s *Ctx) Destroy() {
	//fmt.Println("Ctx.Destroy", s.name)

	ctxManRemove(s.name)

	s.env.Destroy()

	close(s.msgs)
	close(s.msgB)
}

// GetName get ctx name
func (s *Ctx) GetName() string {
	return s.name
}

// GetPreLoad get preload func
func (s *Ctx) GetPreLoad() LuaCFunction {
	return s.preLoad
}

// DoFile worker do file
func (s *Ctx) DoFile(loader string) (bool, error) {
	gCtxMan.wg.Add(1)
	go s.worker(gCtxMan.ctx, &gCtxMan.wg, loader, true)

	return true, nil
}

// DoLibrary worker do library
func (s *Ctx) DoLibrary(loader string) (bool, error) {

	gCtxMan.wg.Add(1)
	go s.worker(gCtxMan.ctx, &gCtxMan.wg, loader, false)

	return true, nil
}

// Post post
func (s *Ctx) Post(msg, msgex, lparam, wparam string, ptr unsafe.Pointer) {
	var cm ctxMsg
	cm.msg = msg
	cm.msgex = msgex
	cm.lparam = lparam
	cm.wparam = wparam
	cm.ptr = ptr

	s.msgs <- cm
}

// PostB post []byte
func (s *Ctx) PostB(msg, msgex, lparam, wparam []byte, ptr unsafe.Pointer) {
	var cm ctxMsgB
	cm.msg = msg
	cm.msgex = msgex
	cm.lparam = lparam
	cm.wparam = wparam
	cm.ptr = ptr

	s.msgB <- cm
}

///////////////////////////////////////////////////////////////////////////////

func (s *Ctx) worker(ctx context.Context, wg *sync.WaitGroup, loader string, bDoFile bool) error {
	defer wg.Done()

	env := s.env

	if bDoFile {
		env.DoFile(loader)
	} else {
		env.DoLibrary(loader)
	}

	if !env.HasKgo() {
		// lua 如果没有"kgo"函数, 则可以直接退出
		return ctx.Err()
	}

	for {
		select {
		case msg := <-s.msgs:
			env.CallKgo(msg.msg, msg.msgex, msg.lparam, msg.wparam, msg.ptr) // post消息
		case msgB := <-s.msgB:
			env.CallKgoB(msgB.msg, msgB.msgex, msgB.lparam, msgB.wparam, msgB.ptr) // post []byte
		case <-ctx.Done():
			return ctx.Err() // 超时或强制退出
		}
	}
}
