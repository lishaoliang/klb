package krtmpserve

import (
	"bytes"
	"context"
	"fmt"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// Conn conn
type Conn struct {
	kconn.Conn

	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg

	netConn    net.Conn // 连接
	preread    []byte   // 预读数据
	prereadPos int      // 已经取走的预读数据位置

	wList  chan kconn.ConnWrite // 写列表
	rList  chan kconn.ConnRead  // 读列表
	rOut   *chan kconn.ConnRead // 接收到网络数据后, 输出文本/二进制数据管道
	rMdOut *chan kconn.ConnRead // 接收到网络数据后, 输出媒体数据管道

	protocol string // 协议
	name     string // 名称
}

// ConnCreate ConnCreate
func ConnCreate(conn net.Conn, data []byte, rOut *chan kconn.ConnRead, rMdOut *chan kconn.ConnRead) *Conn {
	var m Conn

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.netConn = conn
	m.preread = data

	m.wList = make(chan kconn.ConnWrite)
	m.rList = make(chan kconn.ConnRead)
	m.rOut = rOut
	m.rMdOut = rMdOut

	return &m
}

// Destroy Destroy
func (m *Conn) Destroy() {
	//fmt.Println("kflvserve.conn.Destroy", m.protocol, m.name)

	// Bug. 必须先关闭连接, 再关闭线程
	// m.netConn.Read(), m.netConn.Write() 函数为阻塞方式
	// 只有调用m.netConn.Close()之后, 以上函数才会返回

	// 关闭连接
	m.netConn.Close()

	// 关闭线程
	m.Stop()

	close(m.rList)
	close(m.wList)

	m.preread = nil
	m.wList = nil
	m.rList = nil
	m.rOut = nil
	m.rMdOut = nil
}

// Start Start
func (m *Conn) Start() {
	m.wg.Add(1)
	go m.workerWrite()

	m.wg.Add(1)
	go m.workerRead()
}

// Stop Stop
func (m *Conn) Stop() {
	m.cancel()
	m.wg.Wait()
}

// SetProtocol SetProtocol
func (m *Conn) SetProtocol(protocol string) {
	m.protocol = protocol
}

// GetProtocol GetProtocol
func (m *Conn) GetProtocol() string {
	return m.protocol
}

// SetName SetName
func (m *Conn) SetName(name string) {
	m.name = name
}

// GetName GetName
func (m *Conn) GetName() string {
	return m.name
}

// SendTxt SendTxt
func (m *Conn) SendTxt(sequence, uid uint32, extra, msg []byte) bool {
	return false
}

// SendBin SendBin
func (m *Conn) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return false
}

// SendMedia SendMedia
func (m *Conn) SendMedia(buf kbuf.KBuf) bool {
	return false
}

// Recv Recv
func (m *Conn) Recv() <-chan kconn.ConnRead {
	return nil
}

// RecvMedia RecvMedia
func (m *Conn) RecvMedia() <-chan kconn.ConnRead {
	return nil
}

///////////////////////////////////////////////////////////////////////////////////////////

func (m *Conn) pushPackErr(err error) {
	kutil.Assert(nil != err)

	var data kconn.ConnRead

	data.Protocol = m.protocol
	data.Name = m.name
	data.Err = err

	if nil != m.rOut {
		*m.rOut <- data
	} else {
		m.rList <- data
	}
}

func (m *Conn) pushPack(buf kbuf.KBuf) {
	var data kconn.ConnRead

	data.Protocol = m.protocol
	data.Name = m.name
	data.Err = nil
	data.Buf = buf

	if nil != m.rOut {
		*m.rOut <- data
	} else {
		m.rList <- data
	}
}

func (m *Conn) pushPackMd(buf kbuf.KBuf) {

}

func (m *Conn) workerWrite() error {
	defer m.wg.Done()

	for {
		select {
		case data := <-m.wList:
			d := data.Buf.Data()
			fmt.Println("kflvserve write", d)

			data.Buf.Unref()
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}

func (m *Conn) read(b []byte) (n int, err error) {
	if nil != m.preread {
		spare := len(m.preread) - m.prereadPos // 剩余数据
		if 0 < spare {
			// 从预读内存中获取
			r := kutil.Min(len(b), spare)
			copy(b, m.preread[m.prereadPos:m.prereadPos+r])
			m.prereadPos += r

			return r, nil
		}

		// 预读数据取完了
		m.preread = nil
		m.prereadPos = 0
	}

	return m.netConn.Read(b) // 从socket中取数据
}

var myCRLF = []byte{0x0D, 0x0A}                 // "\r\n"
var myCRLFCRLF = []byte{0x0D, 0x0A, 0x0D, 0x0A} // "\r\n\r\n"

func (m *Conn) workerRead() error {
	defer m.wg.Done()

	// todo.
	buf := kbuf.New(64 * 1024)
	defer buf.Unref()

	for {
		select {
		default:
			n, err := m.read(buf.Spare())
			if nil != err {
				m.pushPackErr(err)
				return err
			}

			buf.Add(n)

			d := buf.Data()
			pos := bytes.Index(d, myCRLFCRLF)
			if 0 < pos {
				b := d[0 : pos+4]
				tmp := kbuf.New(len(b))
				copy(tmp.Buf(), b)
				tmp.Add(len(b))
				m.pushPack(tmp)

				buf.SetPos(0, 0)
			}

		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
