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
	"fmt"
	"unsafe"
)

// Ctx ctx
type Ctx struct {
	name    string
	preLoad LuaCFunction
}

// CtxCreate create Ctx
func CtxCreate(preLoad LuaCFunction) *Ctx {
	var s Ctx
	s.preLoad = preLoad

	// push ctxMan
	s.name = ctxManPush(&s)

	return &s
}

// CtxFind find Ctx by name
func CtxFind(ctxName string) *Ctx {
	return ctxManFind(ctxName)
}

// CtxFindByEnv find Ctx by *Env
func CtxFindByEnv(env *Env) *Ctx {
	return ctxManFind(env.GetCtxName())
}

// CtxWait wait
func CtxWait() {
	gCtxMan.wg.Wait()
}

// CtxCancel cancel
func CtxCancel() {
	gCtxMan.cancel()
}

// CtxPost post msg
func CtxPost(envOneName, msg, lparam, wparam string, ptr unsafe.Pointer) bool {
	eo := envManFind(envOneName)

	if nil == eo {
		return false
	}

	eo.Post(msg, lparam, wparam, ptr)
	return true
}

// CtxClose close envOne
func CtxClose(envOneName string) bool {
	eo := envManFind(envOneName)

	if nil == eo {
		return false
	}

	eo.Close()
	return true
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (s *Ctx) Destroy() {
	//fmt.Println("Ctx.Destroy", s.name)

	ctxManRemove(s.name)
}

// WorkerDoFile worker do file
func (s *Ctx) WorkerDoFile(name string, loader string) (bool, error) {
	e, err := envOneCreate(name, s.name, s.preLoad)

	if nil != err {
		fmt.Println(err)
		return false, err
	}

	gCtxMan.wg.Add(1)
	go worker(gCtxMan.ctx, &gCtxMan.wg, s, e, loader, true)

	return true, nil
}

// WorkerDoLibrary worker do library
func (s *Ctx) WorkerDoLibrary(name string, loader string) (bool, error) {
	e, err := envOneCreate(name, s.name, s.preLoad)

	if nil != err {
		fmt.Println(err)
		return false, err
	}

	gCtxMan.wg.Add(1)
	go worker(gCtxMan.ctx, &gCtxMan.wg, s, e, loader, false)

	return true, nil
}
