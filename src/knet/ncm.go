package knet

import (
	"bytes"
	"context"
	"crypto/tls"
	"errors"
	"net"
	"sync"
	"time"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

// Ncm net connect manage
// 网络连接管理模块
type Ncm struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	mapConns *kutil.MapIface // 连接列表
	mutex    sync.Mutex      // 锁

	rList   chan kconn.ConnRead // 读文本/二进制列表
	rMdList chan kconn.ConnRead // 读媒体列表
}

// CreateNcm create Ncm
func CreateNcm() *Ncm {
	var m Ncm

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.mapConns = kutil.NewMapIface("", "", 12)
	m.rList = make(chan kconn.ConnRead)
	m.rMdList = make(chan kconn.ConnRead)

	return &m
}

// Destroy destroy
func (m *Ncm) Destroy() {
	// 接管 m.rList, m.rMdList
	// 将后面产生的数据从管道中取出来, 直到不再有数据产生为止
	// 即必须等待到所有连接被清理完毕
	rd := make(chan struct{}) // wait done
	go func() {
		for {
			select {
			case txt := <-m.rList:
				if nil != txt.Buf {
					txt.Buf.UnrefNext()
				}
			case md := <-m.rMdList:
				if nil != md.Buf {
					md.Buf.UnrefNext()
				}
			case <-rd:
				return
			}
		}
	}()

	// 断开所有连接, 并销毁
	m.mutex.Lock()
	d := m.mapConns.Map()
	for _, v := range d {
		c := m.toConn(v)
		if nil != c {
			c.Destroy()
		}
	}
	//m.mapConns.Destroy()
	m.mutex.Unlock()

	// 等待所有关闭连接线程, 处理完毕
	m.cancel()
	m.wg.Wait()

	// 关闭管道
	close(m.rMdList)
	close(m.rList)
	close(rd)

	m.mapConns = nil
	m.rList = nil
	m.rMdList = nil
}

// Push connect
// 压入(监听得到的)连接
func (m *Ncm) Push(name, protocol string, conn net.Conn, data []byte) bool {
	c := m.serveCreate(protocol, conn, data)
	if nil == c {
		return false // 不支持的协议, conn由调用者关闭
	}

	// 压入列表
	// 成功: 则启动了线程
	// 失败: 这里直接丢弃, conn由调用者关闭
	ret := m.pushByName(name, protocol, c)

	return ret // false时, conn由调用者关闭
}

// Dial dial/connect
// 发起连接
func (m *Ncm) Dial(name, protocol, network, address string) {
	// 非阻塞连接服务端
	// 出现初始连接错误, 由go程处理
	// 新连接加入则由 kconn.Conn.Start() 函数启动的go程中通知
	//  a.如果是客户端, 则由go写线程通知
	//  b.如果是服务端, 则由go读线程通知
	go func() {

		// todo. 进程要求关闭时, 如何快速退出?

		timeout := time.Duration(time.Second) * 6
		conn, err := net.DialTimeout(network, address, timeout)
		if nil == err {
			if MNPS == protocol || HTTPS == protocol || WSS == protocol {
				// tls连接, 客户端不验证
				conn = tls.Client(conn, &tls.Config{InsecureSkipVerify: true})
			}

			c := m.clientCreate(protocol, conn)
			if nil == c {
				conn.Close() // 不支持的协议, 直接关闭
				err = errors.New("knet.ncm.DialByName,not protocol")
				goto errDialByName
			}

			ret := m.pushByName(name, protocol, c)

			if !ret {
				c.Destroy() // 名称重复, 关闭
				err = errors.New("knet.ncm.DialByName,name repeat")
				goto errDialByName
			}

			return // 新连接加入
		}

	errDialByName: // 新连接错误
		var data kconn.ConnRead
		data.Name = name
		data.Protocol = protocol
		data.Err = err
		data.Packtype = kconn.NewConnectError

		m.rList <- data
	}()
}

// SwitchProtocol SwitchProtocol
// 更换协议类型
func (m *Ncm) SwitchProtocol(name, dstProtocol string, head, body []byte) bool {
	m.mutex.Lock()
	r := m.mapConns.Remove(name)
	m.mutex.Unlock()

	//fmt.Println("Ncm.SwitchProtocol", name, dstProtocol)

	if nil != r {
		c := m.toConn(r)
		if nil != c {
			conn, _, _ := c.Hijack()

			data := bytes.Join([][]byte{head, body}, nil)
			push := m.Push(name, dstProtocol, conn, data)
			if !push {
				conn.Close()
			}

			//fmt.Println("Ncm.SwitchProtocol", name, push)
			return push
		}
	}

	//fmt.Println("Ncm.SwitchProtocol", name, false)
	return false
}

// Close connect
// 关闭单个连接
func (m *Ncm) Close(name string) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	r := m.mapConns.Remove(name)
	if nil != r {
		c := m.toConn(r)
		if nil != c {
			// Bug. kconn.Conn.Destroy, kconn.Conn.Stop这两个函数在Ncm中使用时和
			// 操作m.rList, m.rMdList可能处于同一个线程,会造成channel相互等待卡死
			// 所以将所有关闭连接放入专门的线程中处理
			m.closeConn(c)

			return true
		}
	}

	return false
}

// CloseAll close all connect
// 关闭所有连接
func (m *Ncm) CloseAll() {

}

// SendTxt send text
// 发送文本数据
func (m *Ncm) SendTxt(name string, sequence, uid uint32, extra, txt []byte) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	c := m.findConn(name)
	if nil != c {
		return c.SendTxt(sequence, uid, extra, txt)
	}

	return false
}

// SendBin send bin
// 发送二进制数据
func (m *Ncm) SendBin(name string, sequence, uid uint32, extra, bin []byte) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	c := m.findConn(name)
	if nil != c {
		return c.SendBin(sequence, uid, extra, bin)
	}

	return false
}

// SendFile send file
// 发送文件
func (m *Ncm) SendFile(name string, sequence, uid uint32, extra, txt, path []byte) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	c := m.findConn(name)
	if nil != c {
		return c.SendFile(sequence, uid, extra, txt, path)
	}

	return false
}

// SendMedia send media
// 发送媒体数据
func (m *Ncm) SendMedia(name string, buf kbuf.KBuf) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	c := m.findConn(name)
	if nil != c {
		return c.SendMedia(buf)
	}

	return false
}

// Recv recv not media
// 接收非媒体数据
func (m *Ncm) Recv() <-chan kconn.ConnRead {
	return m.rList
}

// RecvMedia recv media
// 接收媒体数据
func (m *Ncm) RecvMedia() <-chan kconn.ConnRead {
	return m.rMdList
}

//////////////////////////////////////////////////////////////////////////

func (m *Ncm) serveCreate(protocol string, conn net.Conn, data []byte) kconn.KConn {
	var c kconn.KConn

	callback := GetCreateCb(kconn.SERVE, protocol)
	if nil != callback {
		c = callback(conn, data, kpool.Default(), &m.rList, &m.rMdList)
	}

	return c
}

func (m *Ncm) clientCreate(protocol string, conn net.Conn) kconn.KConn {
	var c kconn.KConn

	callback := GetCreateCb(kconn.CLIENT, protocol)
	if nil != callback {
		c = callback(conn, nil, kpool.Default(), &m.rList, &m.rMdList)
	}

	return c
}

func (m *Ncm) pushByName(name, protocol string, c kconn.KConn) bool {
	m.mutex.Lock()
	defer m.mutex.Unlock()

	ret := m.mapConns.PushByName(name, c)
	if ret {
		//fmt.Println("Ncm.pushByName", protocol, name)

		c.SetProtocol(protocol)
		c.SetName(name)

		c.Start()
	}

	return ret
}

func (m *Ncm) toConn(conn interface{}) kconn.KConn {
	if c, ok := conn.(kconn.KConn); ok {
		return c
	}

	kutil.Assert(false)
	return nil
}

func (m *Ncm) findConn(name string) kconn.KConn {
	f := m.mapConns.Find(name)
	if nil != f {
		return m.toConn(f)
	}

	return nil
}

// closeConn close kconn.Conn
func (m *Ncm) closeConn(c kconn.KConn) {
	m.wg.Add(1)
	go m.workerCloseConn(c)
}

func (m *Ncm) workerCloseConn(c kconn.KConn) {
	defer m.wg.Done()

	c.Destroy()
}
