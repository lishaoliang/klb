package khttpflv

import (
	"bufio"
	"bytes"
	"errors"
	"net"
	"sync/atomic"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kflv"
	"github.com/lishaoliang/klb/src/knet/khttp"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// Conn Conn
type Conn struct {
	conn kconn.Conn

	mdWrite kbuf.KBuf          // 转换成flv的中间缓存
	parser  kflv.PackMnpParser // 转换解析中间变量
}

/////////////////////////////////////////////////////////////////////////////////

// Hijack Hijack
func (m *Conn) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	panic("Not Support")
	return nil, nil, nil
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
	return false
}

// SendFile SendFile
func (m *Conn) SendFile(sequence, uid uint32, extra, txt, path []byte) bool {
	return false
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

const (
	connStatusHeader = 0 // 接收HTTP的Header阶段
	connStatusBody   = 1 // 接收HTTP的Body阶段
)

// WorkerRead WorkerRead
func (m *Conn) WorkerRead() error {
	defer m.conn.Wg.Done()

	if kconn.SERVE == m.conn.NetType {
		m.conn.PushNewConnect() // 服务端在读线程通知新连接消息
	}

	status := connStatusHeader
	buf := kutil.NewBuf(khttp.HeaderMax)

	var frame [][]byte
	frameSpare := 0
	headLen := 0
	bodyLen := 0

	for {
		select {
		default:
			var err error
			n := 0

			if connStatusHeader == status {
				spare := buf.Spare()
				if len(spare) <= 0 {
					err = errors.New("khttpconn.conn.WorkerRead,read header out of memory")
					m.conn.PushDisconnect(err)
					return err
				}

				n, err = m.conn.Read(spare)
				if nil == err {
					buf.Add(n)

					for {
						d := buf.Data()
						//fmt.Println("httpconn.conn.WorkerRead:", string(d))

						ok, head, body, _ := khttp.Parser(d)
						if ok {
							fullLen := head + body
							if fullLen <= len(d) {
								// 完整的帧数据, 继续解析
								m.conn.PushTxt(0, 0, d[:head], d[head:head+body])
								buf.Memmove(head + body)
							} else {
								// 只解析到了头
								frame0 := buf.Drop()
								frameSpare = fullLen - len(frame0) // 剩余需要读取数据长度

								headLen = head
								bodyLen = body

								frame = append(frame, frame0)

								// 简单处理, 直接申请需要剩余内存长度
								buf = kutil.NewBuf(frameSpare)
								kutil.Assert(0 < frameSpare)
								status = connStatusBody
								break
							}
						} else {
							break // 未解析到头, 继续读取
						}
					}
				}
			} else {
				// 接收Body 阶段
				spare := buf.Spare()
				if len(spare) <= 0 {
					err = errors.New("khttpconn.conn.WorkerRead,read body out of memory")
					m.conn.PushDisconnect(err)
					return err
				}

				n, err = m.conn.Read(spare)
				if nil == err {
					buf.Add(n)
					d := buf.Data()

					frameSpare -= len(d)
					if frameSpare <= 0 {
						// 数据完整了
						frameN := buf.Drop()
						frame = append(frame, frameN)

						data := bytes.Join(frame, nil)
						m.conn.PushTxt(0, 0, data[:headLen], data[headLen:headLen+bodyLen])

						frame = nil
						frameSpare = 0
						headLen = 0
						bodyLen = 0

						buf = kutil.NewBuf(khttp.HeaderMax)
						status = connStatusHeader
					}
				}
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

/////////////////////////////////////////////////////////////////////////////////

// writeData writeData
func (m *Conn) writeData(data *kconn.ConnWrite) error {

	if kconn.DataTxt == data.Packtype {
		head := data.Extra
		body := data.Data

		if err := m.conn.Write(head); nil != err {
			return err
		}

		if err := m.conn.Write(body); nil != err {
			return err
		}

		// 如果回复200, 则补发一个flv头
		var flv kflv.Head
		flv.Video = 0x1
		//flv.Audio = 0x1

		b := make([]byte, kflv.FlvHeadSize+4)
		flv.Pack(b)

		if err := m.conn.Write(b); nil != err {
			return err
		}

	} else if kconn.DataMedia == data.Packtype {
		return m.writeMedia(data)
	}

	// 其他不支持

	return nil
}
