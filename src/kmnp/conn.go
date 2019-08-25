///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/25
//
/// @file    conn.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	conn
///////////////////////////////////////////////////////////////////////////
package kmnp

import (
	"context"
	"fmt"
	"net"
	"sync"
)

// connData
type connData struct {
	sequence uint32
	uid      uint32
	extra    []byte

	msg   []byte
	bin   []byte
	frame []byte
}

// Conn mnp conn
type Conn struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg
	done   chan bool          // 类完成

	netConn net.Conn      // 连接
	wList   chan connData // 写列表
	rList   chan connData // 读列表

	subProto uint8
}

// ConnCreate create conn
func connCreate(netConn net.Conn) *Conn {
	var m Conn
	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.subProto = MNPsubMedia

	m.done = make(chan bool)
	m.wList = make(chan connData)
	m.rList = make(chan connData)

	m.netConn = netConn

	fmt.Println("new conn:", netConn.RemoteAddr())
	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (m *Conn) Destroy() {
	addr := m.netConn.RemoteAddr()
	//fmt.Println("close conn:1111", m.conn.RemoteAddr())

	// 通知读写线程关闭
	m.cancel()
	m.wg.Wait()

	//fmt.Println("close conn:22222", m.conn.RemoteAddr())

	// 关闭连接
	m.netConn.Close()

	// 关闭管道
	close(m.rList)
	close(m.wList)
	close(m.done)

	m.done = nil
	m.wList = nil
	m.rList = nil
	m.netConn = nil

	fmt.Println("close conn:", addr)
}

// workerRead go worker read
func (m *Conn) workerRead() error {
	defer m.wg.Done()

	for {
		select {
		default:

		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}

// workerWrite go worker write
func (m *Conn) workerWrite() error {
	defer m.wg.Done()

	for {
		select {
		case data := <-m.wList:
			fmt.Println(data)
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}

// Worker worker
func (m *Conn) Worker() {
	m.wg.Add(1)
	go m.workerWrite()

	m.wg.Add(1)
	go m.workerRead()
}

// WaitDone wait for done
func (m *Conn) WaitDone() bool {
	b := <-m.done
	//fmt.Println("kgnet.Mnp.Done", b)
	return b
}

// SendTxt send text
func (m *Conn) SendTxt(sequence, uid uint32, extra, msg []byte) bool {
	return true
}

// SendBin send bin
func (m *Conn) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return true
}

// SendMedia send media
func (m *Conn) SendMedia(frame []byte) bool {
	return true
}

// Recv recv
func (m *Conn) Recv() {

}
