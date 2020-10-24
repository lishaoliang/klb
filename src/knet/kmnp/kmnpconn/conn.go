package kmnpconn

import (
	"bufio"
	"bytes"
	"errors"
	"fmt"
	"net"
	"sync/atomic"

	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
	"github.com/lishaoliang/klb/src/kutil/kbuf"
)

// Conn Conn
type Conn struct {
	conn kconn.Conn

	readMnp    kmnp.Mnp // mnp 头
	readStatus uint8    // 读取状态
}

/////////////////////////////////////////////////////////////////////////////////

// Hijack Hijack
func (m *Conn) Hijack() (net.Conn, *bufio.ReadWriter, error) {
	return m.conn.NetConn, nil, nil
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

const mnpPackTypeNil = kmnp.MnpPackMax + 1

// WorkerRead WorkerRead
func (m *Conn) WorkerRead() error {
	defer m.conn.Wg.Done()

	if kconn.SERVE == m.conn.NetType {
		m.conn.PushNewConnect() // 服务端在读线程通知新连接消息
	}

	h := kutil.NewBuf(kmnp.MnpHeadSize) // mnp 头接收缓存

	var buf *kutil.Buf            // 文本,二进制缓存
	md := kutil.NewBuf(36 * 1024) // 媒体接收缓存

	var txt [][]byte
	var bin [][]byte

	m.readStatus = mnpPackTypeNil

	for {
		select {
		default:
			var err error
			var ok bool

			switch m.readStatus {
			case mnpPackTypeNil:
				ok, err = m.readHead(h)
				if ok {
					m.readStatus = m.readMnp.Packtype
					h.Reset()

					// 准备接下来的读取缓存
					if kmnp.MnpPackText == m.readMnp.Packtype || kmnp.MnpPackBin == m.readMnp.Packtype {
						buf = kutil.NewBuf(int(m.readMnp.Size - kmnp.MnpHeadSize))
					}

					// test
					if kmnp.MnpPackMedia == m.readMnp.Packtype {
						buf = kutil.NewBuf(int(m.readMnp.Size - kmnp.MnpHeadSize))
					}
				}
			case kmnp.MnpPackHeart:
				if kconn.SERVE == m.conn.NetType {
					m.conn.SendHeart() // 服务端回应心跳
				}

				m.readStatus = mnpPackTypeNil
			case kmnp.MnpPackText:
				ok, err = m.readTxt(buf)
				if ok {
					txt = append(txt, buf.Drop())
					buf = nil

					if kmnp.MnpOptFull == m.readMnp.Opt || kmnp.MnpOptEnd == m.readMnp.Opt {
						data := bytes.Join(txt, nil)

						var tmpText kmnp.MnpText
						tmpText.Unpack(data)

						// 读取到了数据帧
						m.conn.PushTxt(tmpText.Sequence, tmpText.UID, data[kmnp.MnpTextSize:kmnp.MnpTextSize+tmpText.Extra],
							data[kmnp.MnpTextSize+tmpText.Extra:])

						txt = nil
					}

					m.readStatus = mnpPackTypeNil
				}
			case kmnp.MnpPackBin:
				ok, err = m.readBin(buf)
				if ok {
					bin = append(bin, buf.Drop())
					buf = nil

					if kmnp.MnpOptFull == m.readMnp.Opt || kmnp.MnpOptEnd == m.readMnp.Opt {
						data := bytes.Join(bin, nil)

						var mnpBin kmnp.MnpBin
						mnpBin.Unpack(data)

						// 读取到了数据帧
						m.conn.PushBin(mnpBin.Sequence, mnpBin.UID, data[kmnp.MnpTextSize:kmnp.MnpTextSize+mnpBin.Extra],
							data[kmnp.MnpTextSize+mnpBin.Extra:])

						txt = nil
					}

					m.readStatus = mnpPackTypeNil
				}
			case kmnp.MnpPackMedia:
				ok, err = m.readMd(md)
				if ok {
					m.processMd(md) // 处理数据帧

					// 复位, 重置
					md.Reset()
					m.readStatus = mnpPackTypeNil
				}
			default:
				fmt.Println("kmnpconn.conn.WorkerRead,unkown type")
				err = errors.New("kmnpconn.conn.WorkerRead,unkown type")
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
