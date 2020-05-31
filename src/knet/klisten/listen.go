///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   listen.go
/// @author lishaoliang
/// @brief	listen
///////////////////////////////////////////////////////////////////////////

package klisten

import (
	"context"
	"fmt"
	"net"
	"sync"
)

// listen
type listen struct {
	ctx    context.Context    // 上下文环境
	cancel context.CancelFunc // 取消
	wg     sync.WaitGroup     // 等待组

	listener net.Listener // 监听对象
	tls      bool         // 是否为TLS监听
	run      bool         // 是否已经start线程了
	callback ListenCb     // 监听到连接的回调函数
	cbPtr    interface{}  // 对象指针
}

func newListen(ln net.Listener, tls bool, cb ListenCb, ptr interface{}) *listen {
	var m listen

	m.ctx, m.cancel = context.WithCancel(context.Background())
	m.listener = ln
	m.tls = tls
	m.run = false
	m.callback = cb
	m.cbPtr = ptr

	return &m
}

func (m *listen) destroy() {
	m.stop()
	m.callback = nil
	m.cbPtr = nil
}

func (m *listen) start() {
	m.wg.Add(1)
	go m.worker()

	m.run = true
}

func (m *listen) stop() {
	if m.run {
		m.listener.Close()

		m.cancel()
		m.wg.Wait()

		m.run = false
	}
}

func (m *listen) worker() error {
	defer m.wg.Done()

	for {
		select {
		default:
			// net.Listener.Accept函数一直柱塞, 直到有连接或异常出现
			// 调用net.Listener.Close函数之后, Accept函数会产生异常
			conn, err := m.listener.Accept()
			if err != nil {
				// handle error
			} else if nil != m.callback {
				if 0 == m.callback(m.cbPtr, conn, m.tls) {
					conn = nil
				}
			}

			if nil != conn {
				fmt.Println("close new connect.", conn)
				conn.Close()
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
