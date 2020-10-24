package kwsflv

import (
	"bufio"
	"net"
	"sync/atomic"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kflv"
	"github.com/lishaoliang/klb/src/knet/kws"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// switchStatus
const (
	switchStatusHTTP      = 0x0 // HTTP: 读写http
	switchStatusSwitching = 0x1 // HTTP切换到Websocket中间状态: 读ws,写http
	switchStatusWebsocket = 0x2 // Websocket: 读写ws
)

// httpStatus
const (
	httpStatusHeader = 0
	httpStatusBody   = 1
)

// wsStatus
const (
	wsStatusHeader = 0
	wsStatusBody   = 1
)

// Conn Conn
type Conn struct {
	conn         kconn.Conn // 连接
	switchStatus int        // 切换状态: switchStatusHTTP,switchStatusWebsocket

	// http时,使用的变量
	httpStatus     int        // http读取状态: httpStatusHeader, httpStatusBody
	httpBuf        *kutil.Buf //
	httpFrame      [][]byte   //
	httpFrameSpare int        //
	httpHeadLen    int
	httpBodyLen    int

	// ws时,使用的变量
	wsStatus     int        //
	wsBuf        *kutil.Buf //
	wsNextBuf    *kutil.Buf //
	wsHeader     kws.Ws
	wsFrame      [][]byte //
	wsFrameSpare int      //

	mdWrite kbuf.KBuf          // 转换成flv的中间缓存
	parser  kflv.PackMnpParser // 转换解析中间变量
}

/////////////////////////////////////////////////////////////////////////////////

// Hijack Hijack
func (m *Conn) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	// Hijack让调用者接管连接, 返回连接和关联到该连接的一个缓冲读写器
	return m.conn.Hijack()
}

// Destroy Destroy
func (m *Conn) Destroy() {
	m.conn.Destroy()
}

// SetProtocol SetProtocol
func (m *Conn) SetProtocol(protocol string) { m.conn.SetProtocol(protocol) }

// GetProtocol GetProtocol
func (m *Conn) GetProtocol() string { return m.conn.GetProtocol() }

// SetName SetName
func (m *Conn) SetName(name string) { m.conn.SetName(name) }

// GetName GetName
func (m *Conn) GetName() string { return m.conn.GetName() }

// SendTxt SendTxt
func (m *Conn) SendTxt(sequence, uid uint32, extra, txt []byte) bool {
	return m.conn.SendTxt(sequence, uid, extra, txt)
}

// SendBin SendBin
func (m *Conn) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return m.conn.SendBin(sequence, uid, extra, bin)
}

// SendFile SendFile
func (m *Conn) SendFile(sequence, uid uint32, extra, txt, path []byte) bool {
	return m.conn.SendFile(sequence, uid, extra, txt, path)
}

// SendMedia SendMedia
func (m *Conn) SendMedia(buf kbuf.KBuf) bool {
	return m.conn.SendMedia(buf)
}

/////////////////////////////////////////////////////////////////////////////////

// Start Start
func (m *Conn) Start() {
	atomic.StoreInt32(&m.conn.AtomicWriteRun, 1) // 可运行

	m.conn.Wg.Add(1)
	go m.WorkerWrite()

	m.conn.Wg.Add(1)
	go m.WorkerRead()
}

// WorkerWrite WorkerWrite
func (m *Conn) WorkerWrite() error {
	defer m.conn.Wg.Done()

	// 写线程不使用管道:是因为慢速的连接会拖慢管道,从而影响Send*系列接口
	// 写线程一直持续到调用者关闭, 是为了将放入写链表的数据尽快消费掉

	if kconn.CLIENT == m.conn.NetType {
		m.conn.PushNewConnect() // 客户端在写线程通知新连接消息
	}

	// 是否出现错误
	var err error

	// 初始不进入等待
	wait := false

	for 0 != atomic.LoadInt32(&m.conn.AtomicWriteRun) {
		//    c.L.Lock()
		//    for !condition() {
		//        c.Wait()
		//    }
		//    ... make use of condition ...
		//    c.L.Unlock()
		m.conn.WriteCond.L.Lock()
		if wait {
			m.conn.WriteCond.Wait()
		}

		m.conn.DropWriteMedia()       // 丢数据流程
		data := m.conn.GetConnWrite() // 获取需要发送的数据
		m.conn.WriteCond.L.Unlock()

		if nil != data {
			wait = false // 有数据发送, 则不进等待; 一直到全部数据发送完毕

			if nil == err {
				err = m.writeData(data)
				if nil != err {
					m.conn.PushDisconnect(err)
				}
			} else {
				// 出现错误, 并不立即退出线程, 而是需要等待到调用者主动关闭线程
			}

			// 释放数据
			m.conn.FreeConnWrite(data)
		} else {
			wait = true // 无数据可写, 则进入等待
		}
	}

	return err
}

// WorkerRead WorkerRead
func (m *Conn) WorkerRead() error {
	defer m.conn.Wg.Done()

	if kconn.SERVE == m.conn.NetType {
		m.conn.PushNewConnect() // 服务端在读线程通知新连接消息
	}

	for {
		select {
		default:
			var err error
			if switchStatusHTTP == m.switchStatus {
				err = m.doReadHTTP()
			} else {
				// switchStatusSwitching, switchStatusWebsocket
				err = m.doReadWebsocket()
			}

			if nil != err {
				m.conn.PushDisconnect(err)
				return err
			}
		case <-m.conn.Ctx.Done():
			return m.conn.Ctx.Err() // 超时或强制退出
		}
	}
}
