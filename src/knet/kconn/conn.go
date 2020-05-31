package kconn

import (
	"bufio"
	"container/list"
	"context"
	"errors"
	"fmt"
	"net"
	"sync"
	"sync/atomic"

	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
	"github.com/lishaoliang/klb/src/kutil/kpool"
)

// Conn conn
type Conn struct {
	AtomicWriteRun int32              // 写线程是否运行; 0.停止; 非0.运行
	Ctx            context.Context    // 同步ctx
	Cancel         context.CancelFunc // 同步cancel
	Wg             sync.WaitGroup     // 同步wg

	NetConn    net.Conn // 连接
	Preread    []byte   // 预读数据
	PrereadPos int      // 已经取走的预读数据位置

	ReadOut   *chan ConnRead // 接收到网络数据后, 输出文本/二进制数据管道
	ReadMdOut *chan ConnRead // 接收到网络数据后, 输出媒体数据管道
	Pool      kpool.KPool    // 媒体数据内存池

	WriteCond   *sync.Cond // 写线程唤醒信号
	WriteList   *list.List // 非媒体写列表
	WriteMdList *list.List // 媒体写列表

	NetType  string // 网络类型
	Protocol string // 协议
	Name     string // 名称
}

// InitConn InitConn
func InitConn(m *Conn, netType string, conn net.Conn, data []byte, pool kpool.KPool, rOut *chan ConnRead, rMdOut *chan ConnRead) {
	m.Ctx, m.Cancel = context.WithCancel(context.Background())

	m.NetConn = conn
	m.Preread = data
	m.PrereadPos = 0

	m.ReadOut = rOut
	m.ReadMdOut = rMdOut
	m.Pool = pool

	m.WriteCond = sync.NewCond(new(sync.Mutex))
	m.WriteList = list.New()
	m.WriteMdList = list.New()

	m.NetType = netType // SERVE, CLIENT
}

// Hijack Hijack
func (m *Conn) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	// http.Hijack原本是: 让调用者接管连接, 返回连接和关联到该连接的一个缓冲读写器
	// 这里简化处理: 只保留连接句柄, 其他资源(内存等)一律随原对象释放
	// 并且必须等待到连接完全关闭(线程退出,资源释放)之后, 才可以退出函数; 否则原线程会依然读写网络,造成干扰
	//
	// 但 net.Conn.Read, net.Conn.Write 函数属于柱塞函数, 使用Read,Write函数时, 却不能调用Hijack, 否则出现"假死"
	// 标准http模块使用 net.Conn.SetDeadline 来处理Hijack的这个问题
	// 这里换一种思路处理: 由需要转换协议的对象(khttp)自己识别到需要使用Hijack,主动停止读写,等待被调用Hijack,从而切换协议
	//
	// 所以: 对需要切换协议的模块, 需要做检测是否需要Hijack,主动停止读写动作
	conn := m.NetConn

	// 关闭线程
	// 注意: 因为没有关闭socket,网络Read,Write会阻塞; 如果不解决阻塞, m.Stop()会卡死
	// 所以之前必须解决Read/Write阻塞问题
	m.Stop()

	// 释放掉残留数据
	for {
		data := m.GetConnWrite()
		if nil != data {
			data.Extra = nil
			data.Data = nil
			if nil != data.Buf {
				data.Buf.UnrefNext()
			}
		} else {
			break
		}
	}

	kutil.Assert(0 == m.WriteList.Len())
	kutil.Assert(0 == m.WriteMdList.Len())

	///////////////////////////////////////
	// 解除引用
	m.NetConn = nil
	m.Preread = nil
	m.ReadOut = nil
	m.ReadMdOut = nil
	m.Pool = nil
	m.WriteCond = nil
	m.WriteList = nil
	m.WriteMdList = nil

	return conn, nil, nil
}

// Destroy Destroy
func (m *Conn) Destroy() {
	//fmt.Println("kflvserve.conn.Destroy", m.protocol, m.name)

	// Bug. 必须先关闭连接, 再关闭线程
	// m.netConn.Read(), m.netConn.Write() 函数为阻塞方式
	// 只有调用m.netConn.Close()之后, 以上函数才会返回

	// 关闭连接
	m.NetConn.Close()

	// 关闭线程
	// 注意: 上一步m.NetConn.Close()之后,下面m.Stop() 才不会卡死
	// 因为关闭socket之后,网络的Read,Write会退出阻塞
	m.Stop()

	// 释放掉残留数据
	for {
		data := m.GetConnWrite()
		if nil != data {
			data.Extra = nil
			data.Data = nil
			if nil != data.Buf {
				data.Buf.UnrefNext()
			}
		} else {
			break
		}
	}

	kutil.Assert(0 == m.WriteList.Len())
	kutil.Assert(0 == m.WriteMdList.Len())

	///////////////////////////////////////
	// 解除引用
	m.NetConn = nil
	m.Preread = nil
	m.ReadOut = nil
	m.ReadMdOut = nil
	m.Pool = nil
	m.WriteCond = nil
	m.WriteList = nil
	m.WriteMdList = nil
}

// Stop Stop
func (m *Conn) Stop() {
	atomic.StoreInt32(&m.AtomicWriteRun, 0) // 停止

	// 通知
	m.WriteCond.L.Lock()
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	m.Cancel()
	m.Wg.Wait()
}

// SetProtocol SetProtocol
func (m *Conn) SetProtocol(protocol string) {
	m.Protocol = protocol
}

// GetProtocol GetProtocol
func (m *Conn) GetProtocol() string {
	return m.Protocol
}

// SetName SetName
func (m *Conn) SetName(name string) {
	m.Name = name
}

// GetName GetName
func (m *Conn) GetName() string {
	return m.Name
}

// SendHeart SendHeart
func (m *Conn) SendHeart() bool {
	var data ConnWrite
	data.Packtype = DataHeart

	m.WriteCond.L.Lock()
	m.WriteList.PushBack(&data)
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	return true
}

// SendTxt SendTxt
func (m *Conn) SendTxt(sequence, uid uint32, extra, txt []byte) bool {
	var data ConnWrite
	data.Packtype = DataTxt
	data.Sequence = sequence
	data.UID = uid
	data.Extra = extra
	data.Data = txt

	m.WriteCond.L.Lock()
	m.WriteList.PushBack(&data)
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	return true
}

// SendBin SendBin
func (m *Conn) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	var data ConnWrite
	data.Packtype = DataBin
	data.Sequence = sequence
	data.UID = uid
	data.Extra = extra
	data.Data = bin

	m.WriteCond.L.Lock()
	m.WriteList.PushBack(&data)
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	return true
}

// SendFile SendFile
func (m *Conn) SendFile(sequence, uid uint32, extra, txt, path []byte) bool {
	var data ConnWrite
	data.Packtype = DataFile
	data.Sequence = sequence
	data.UID = uid
	data.Extra = extra
	data.Data = txt
	data.Path = path

	m.WriteCond.L.Lock()
	m.WriteList.PushBack(&data)
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	return true
}

// SendMedia SendMedia
func (m *Conn) SendMedia(buf kbuf.KBuf) bool {
	var data ConnWrite
	data.Packtype = DataMedia
	data.Buf = buf

	buf.RefNext()

	m.WriteCond.L.Lock()
	m.WriteMdList.PushBack(&data)
	m.WriteCond.Signal()
	m.WriteCond.L.Unlock()

	return true
}

// PushNewConnect PushNewConnect
func (m *Conn) PushNewConnect() {
	var data ConnRead
	data.Packtype = NewConnect
	data.Name = m.Name
	data.Protocol = m.Protocol

	*m.ReadOut <- data
}

// PushDisconnect PushDisconnect
func (m *Conn) PushDisconnect(err error) {
	var data ConnRead
	data.Packtype = Disconnect
	data.Name = m.Name
	data.Protocol = m.Protocol
	data.Err = errors.New(err.Error())

	*m.ReadOut <- data
}

// PushTxt PushTxt
func (m *Conn) PushTxt(sequence, uid uint32, extra, txt []byte) {
	var data ConnRead
	data.Packtype = DataTxt
	data.Name = m.Name
	data.Protocol = m.Protocol

	data.Sequence = sequence
	data.UID = uid
	data.Extra = extra
	data.Data = txt

	*m.ReadOut <- data
}

// PushBin PushBin
func (m *Conn) PushBin(sequence, uid uint32, extra, bin []byte) {
	var data ConnRead
	data.Packtype = DataBin
	data.Name = m.Name
	data.Protocol = m.Protocol

	data.Sequence = sequence
	data.UID = uid
	data.Extra = extra
	data.Data = bin

	*m.ReadOut <- data
}

// PushMedia PushMedia
func (m *Conn) PushMedia(buf kbuf.KBuf) {
	var data ConnRead
	data.Packtype = DataMedia
	data.Name = m.Name
	data.Protocol = m.Protocol

	data.Buf = buf

	*m.ReadMdOut <- data
}

// Write Write
func (m *Conn) Write(b []byte) error {
	blen := len(b)
	if blen <= 0 {
		return nil
	}

	pos := 0
	for {
		n, err := m.NetConn.Write(b[pos:])

		if nil != err {
			return err
		}

		pos += n
		if blen <= pos {
			kutil.Assert(blen == pos)
			break // 此块数据写入完毕
		}
	}

	return nil
}

// Read Read
func (m *Conn) Read(b []byte) (n int, err error) {
	if nil != m.Preread {
		spare := len(m.Preread) - m.PrereadPos // 剩余数据
		if 0 < spare {
			// 从预读内存中获取
			r := kutil.Min(len(b), spare)
			copy(b, m.Preread[m.PrereadPos:m.PrereadPos+r])
			m.PrereadPos += r

			return r, nil
		}

		// 预读数据取完了
		m.Preread = nil
		m.PrereadPos = 0
	}

	return m.NetConn.Read(b) // 从socket中取数据
}

// Malloc Malloc
func (m *Conn) Malloc() kbuf.KBuf {
	return m.Pool.Malloc()
}

// GetConnWrite GetConnWrite
func (m *Conn) GetConnWrite() *ConnWrite {
	item := m.WriteList.Front()
	if nil != item {
		v := item.Value
		m.WriteList.Remove(item)

		if cw, ok := v.(*ConnWrite); ok {
			return cw
		}

		kutil.Assert(false)
	}

	item = m.WriteMdList.Front()
	if nil != item {
		v := item.Value
		m.WriteMdList.Remove(item)

		if cw, ok := v.(*ConnWrite); ok {
			return cw
		}

		kutil.Assert(false)
	}

	return nil
}

///////////////////////////////////////////////////////////////////////////////
//  以下函数需要被重写

// Start Start
func (m *Conn) Start() {
	atomic.StoreInt32(&m.AtomicWriteRun, 1) // 可运行

	m.Wg.Add(1)
	go m.WorkerWrite()

	m.Wg.Add(1)
	go m.WorkerRead()
}

// WriteData WriteData
func (m *Conn) WriteData(data *ConnWrite) error {
	// demo 直接释放数据

	data.Extra = nil
	data.Data = nil

	if nil != data.Buf {
		data.Buf.UnrefNext()
	}

	fmt.Println("kconn.conn.WriteData,not write", m.Name, m.Protocol)
	return nil
}

// WorkerWrite WorkerWrite
func (m *Conn) WorkerWrite() error {
	defer m.Wg.Done()

	// 写线程不使用管道:是因为慢速的连接会拖慢管道,从而影响Send*系列接口
	// 写线程一直持续到调用者关闭, 是为了将放入写链表的数据尽快消费掉

	if CLIENT == m.NetType {
		m.PushNewConnect() // 客户端在写线程通知新连接消息
	}

	// 是否出现错误
	var err error

	// 初始不进入等待
	wait := false

	for 0 != atomic.LoadInt32(&m.AtomicWriteRun) {
		//    c.L.Lock()
		//    for !condition() {
		//        c.Wait()
		//    }
		//    ... make use of condition ...
		//    c.L.Unlock()
		m.WriteCond.L.Lock()
		if wait {
			m.WriteCond.Wait()
		}

		data := m.GetConnWrite() // 获取需要发送的数据
		m.WriteCond.L.Unlock()

		if nil != data {
			wait = false // 有数据发送, 则不进等待; 一直到全部数据发送完毕

			if nil == err {
				err = m.WriteData(data)
				if nil != err {
					m.PushDisconnect(err)
				}
			} else {
				// 出现错误, 并不立即退出线程, 而是需要等待到调用者主动关闭线程
			}

			// 释放数据
			data.Extra = nil
			data.Data = nil
			if nil != data.Buf {
				data.Buf.UnrefNext()
			}
		} else {
			wait = true // 无数据可写, 则进入等待
		}
	}

	return err
}

// WorkerRead WorkerRead
func (m *Conn) WorkerRead() error {
	defer m.Wg.Done()

	// 读线程当出现错误时, 直接退出

	if SERVE == m.NetType {
		m.PushNewConnect() // 服务端在读线程通知新连接消息
	}

	// 临时缓存
	b := make([]byte, 512)

	for {
		select {
		default:
			_, err := m.Read(b)
			if nil != err {
				m.PushDisconnect(err)
				return err // 读线程, 读到错误则退出
			}
		case <-m.Ctx.Done():
			return m.Ctx.Err() // 超时或强制退出
		}
	}
}
