package krtspclient

import (
	"context"
	"errors"
	"fmt"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/krtsp"
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

	wList chan kconn.ConnWrite // 写列表
	//rList  chan kconn.ConnRead  // 读列表
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
	//m.rList = make(chan kconn.ConnRead)

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

	//close(m.rList)
	close(m.wList)

	m.preread = nil
	m.wList = nil
	//m.rList = nil
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
func (m *Conn) SendTxt(sequence, uid uint32, extra, txt []byte) bool {
	var data kconn.ConnWrite
	data.Packtype = kconn.DataTxt
	data.Extra = extra

	m.wList <- data

	return true
}

// SendBin SendBin
func (m *Conn) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return false
}

// Send Send
func (m *Conn) Send(buf kbuf.KBuf) bool {
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

func (m *Conn) pushNewConnect() {
	var data kconn.ConnRead
	data.Packtype = kconn.NewConnect
	data.Name = m.name
	data.Protocol = m.protocol

	*m.rOut <- data
}

func (m *Conn) pushDisconnect(err error) {
	var data kconn.ConnRead
	data.Packtype = kconn.Disconnect
	data.Name = m.name
	data.Protocol = m.protocol
	data.Err = errors.New(err.Error())

	*m.rOut <- data
}

func (m *Conn) pushTxt(extra, txt []byte) {
	var data kconn.ConnRead
	data.Packtype = kconn.DataTxt
	data.Name = m.name
	data.Protocol = m.protocol

	data.Extra = extra
	data.Data = txt

	*m.rOut <- data
}

func (m *Conn) pushMedia(buf kbuf.KBuf) {
	var data kconn.ConnRead
	data.Packtype = kconn.DataMedia
	data.Name = m.name
	data.Protocol = m.protocol

	data.Buf = buf

	*m.rMdOut <- data
}

func (m *Conn) workerWrite() error {
	defer m.wg.Done()

	// 客户端连接在write线程中, 通知新连接加入
	m.pushNewConnect()

	for {
		select {
		case data := <-m.wList:
			kutil.Assert(kconn.DataTxt == data.Packtype)
			//fmt.Println("krtspclient.workerWrite", string(data.Extra), string(data.Data))

			b := data.Extra
			pos := 0

			for {
				n, err := m.netConn.Write(b[pos:])

				if nil != err {
					fmt.Println("krtspclient.conn.workerWrite", err)
					m.pushDisconnect(err)
					return err
				}

				pos += n
				if len(b) <= pos {
					break //
				}
			}
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

	buf := kutil.NewBuf(64 * 1024)
	defer buf.Destroy()

	for {
		select {
		default:
			spare := buf.Spare()
			if len(spare) <= 0 {
				e := errors.New("krtspclient.workerRead,out of memory")
				m.pushDisconnect(e)
				return e
			}

			n, err := m.read(buf.Spare())
			if nil != err {
				m.pushDisconnect(err)
				return err
			}

			if 0 < n {
				buf.Add(n)
				d := buf.Data()
				pos := 0

				for {
					ret, head, body := krtsp.Parser(d[pos:])
					if ret {
						m.pushTxt(d[pos:pos+head], d[pos+head:pos+head+body])
						pos += head + body
					} else {
						break
					}
				}

				if 0 < pos {
					buf.Memmove(pos)
				}
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
