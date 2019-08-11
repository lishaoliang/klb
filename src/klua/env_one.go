///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/11
//
/// @file    env_one.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	envOne
///////////////////////////////////////////////////////////////////////////
package klua

import (
	"errors"
	"unsafe"
)

// envMsg message
type envMsg struct {
	msg    string
	lparam string
	wparam string
	ptr    unsafe.Pointer
}

// 单个*Env环境
type envOne struct {
	env  *Env   // CGO Env(Lua对接C环境)
	name string // 名称

	msgs   chan envMsg // messages, post from other envOne
	closes chan int    // 关闭信息
}

// 创建单环境
// name=""表示不记名
func envOneCreate(name string, ctxName string, preLoad LuaCFunction) (*envOne, error) {
	if "" != name && nil != envManFind(name) {
		return nil, errors.New("klua.envOne.name,error!repeat")
	}

	var e envOne
	e.name = name
	e.closes = make(chan int)
	e.msgs = make(chan envMsg)

	e.env = EnvCreate(ctxName, preLoad)

	if "" != name && !envManPush(name, &e) {
		e.env.Destroy()
		return nil, errors.New("klua.envOne.name,error!repeat")
	}

	return &e, nil
}

///////////////////////////////////////////////////////////////////////////////

// Destroy 销毁
func (e *envOne) Destroy() {
	//println("envOne.Destroy", e)

	if "" != e.name {
		envManRemove(e.name)
	}

	e.env.Destroy()

	close(e.msgs)
	close(e.closes)
}

// Close close
func (e *envOne) Close() {
	// 先移除, 保证关闭后, 不会再被使用
	if "" != e.name {
		envManRemove(e.name)
	}

	// 再通知关闭
	e.closes <- 1
}

// GetEnv 获取*Env环境指针
func (e *envOne) GetEnv() *Env {
	return e.env
}

// Post post消息
func (e *envOne) Post(msg, lparam, wparam string, ptr unsafe.Pointer) {
	var em envMsg
	em.msg = msg
	em.lparam = lparam
	em.wparam = wparam
	em.ptr = ptr

	e.msgs <- em
}
