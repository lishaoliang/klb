///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   ser_man.go
/// @author lishaoliang
/// @brief	ser man
///////////////////////////////////////////////////////////////////////////

package kamdc

import (
	"context"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/kbuf/kmpool"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

// serMan server
type serMan struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	rList chan kmnp.ConnDataR // 从连接接收得到的数据

	poolV1 kmpool.Pool // V1内存池
	poolV2 kmpool.Pool // V2内存池

	connMap   *kutil.MapIface // 存储serConn
	connMutex sync.Mutex      // connMap锁

	streamMap   *kutil.MapIface // 开启了流的连接,存储serConn
	streamMutex sync.Mutex      // streamMap锁
}

// serManCreate server man
func serManCreate() *serMan {
	var m serMan
	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.rList = make(chan kmnp.ConnDataR, 10000)

	m.poolV1 = kmpool.PoolCreate(1024*4, 1024)
	m.poolV2 = kmpool.PoolCreate(1024*4, 1024)

	// 预计支持100+连接量
	m.connMap = kutil.MapIfaceCreate("mdc", ":", 8)
	m.streamMap = kutil.MapIfaceCreate("mdc", ":", 8)

	return &m
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (m *serMan) Destroy() {

	// 通知读写线程关闭
	m.cancel()
	m.wg.Wait()

	// 关闭所有连接
	maps := m.connMap.Map()
	for _, v := range maps {
		sc := m.toSerConn(v)
		if nil != sc {
			go func(conn *serConn) {
				conn.Destroy()
			}(sc)
		}
	}

	m.connMap.Destroy()
	m.streamMap.Destroy()

	close(m.rList)

	m.poolV2.Destroy()
	m.poolV1.Destroy()
}

// Start start worker
func (m *serMan) Start(name, entry, preload, loader string) {

	m.wg.Add(1)
	go m.workerCmd(name, entry, preload, loader)
}

// Push push
func (m *serMan) Push(protocol string, conn net.Conn) {
	mnp := kmnp.SerLimitCreate(conn, &m.rList)
	sc := serConnCreate(mnp)
	sc.SetProtocol(protocol)

	m.connMutex.Lock()
	defer m.connMutex.Unlock()

	name := m.connMap.Push(sc)
	sc.SetName(name)
	sc.Start()
}

// SendTxt send text
func (m *serMan) SendTxt(protocol, name string, sequence, uid int64, extra []byte, data []byte) bool {

	m.connMutex.Lock()
	defer m.connMutex.Unlock()

	sc := m.findConn(name)

	if nil != sc {
		return sc.SendTxt(sequence, uid, extra, data)
	}

	return false
}

// OpenStream
func (m *serMan) OpenStream(protocol, name string, chnn, sidx int) bool {

	m.connMutex.Lock()
	defer m.connMutex.Unlock()

	m.streamMutex.Lock()
	defer m.streamMutex.Unlock()

	sc := m.findConn(name)

	if nil != sc {
		sc.OpenStream(chnn, sidx)

		if sc.HasStream() {
			m.streamMap.PushByName(name, sc)
		}

		return true
	}

	return false
}

// CloseStream
func (m *serMan) CloseStream(protocol, name string, chnn, sidx int) bool {

	m.connMutex.Lock()
	defer m.connMutex.Unlock()

	m.streamMutex.Lock()
	defer m.streamMutex.Unlock()

	sc := m.findConn(name)

	if nil != sc {
		sc.CloseStream(chnn, sidx)

		if !sc.HasStream() {
			m.streamMap.Remove(name)
		}

		return true
	}

	return false
}

// PushStream
func (m *serMan) PushStream(chnn, sidx int, kmnps *kmnp.MnpStream) {
	var buf *kmpool.KmBuf

	if kmnp.MNPsidxV1 == sidx {
		buf, _ = kmnps.PackKmBuf(nil, m.poolV1, chnn, sidx)
	} else if kmnp.MNPsidxV2 == sidx {
		buf, _ = kmnps.PackKmBuf(nil, m.poolV2, chnn, sidx)
	}

	if nil == buf {
		return
	}

	m.streamMutex.Lock()
	for _, v := range m.streamMap.Map() {
		sc := m.toSerConn(v)
		if nil != sc {
			sc.SendMedia(chnn, sidx, buf)
		}
	}
	m.streamMutex.Unlock()

	buf.UnrefAll()
}

///////////////////////////////////////////////////////////////////////////////

// 转换为 *serConn
func (m *serMan) toSerConn(item interface{}) *serConn {
	if nil != item {
		sc, ok := item.(*serConn)

		if ok {
			return sc
		}
	}

	return nil
}

// 查找连接
func (m *serMan) findConn(name string) *serConn {
	return m.toSerConn(m.connMap.Find(name))
}
