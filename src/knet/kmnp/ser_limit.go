///////////////////////////////////////////////////////////////////////////
//	Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file	conn_ser_limit.go
/// @author	lishaoliang
/// @brief	conn_ser_limit
///////////////////////////////////////////////////////////////////////////
package kmnp

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/kbuf/kmpool"
	"github.com/lishaoliang/klb/src/kutil"
)

// serLimit
// 受限服务端连接, 适用于嵌入式服务端
type serLimit struct {
	ConnInterface

	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg
	done   chan bool          // 类完成

	netConn net.Conn        // 连接
	wList   chan ConnDataW  // 写列表
	rList   chan ConnDataR  // 读列表
	rOut    *chan ConnDataR // 接收到网络数据后, 输出的数据管道

	protocol string // 协议名称
	name     string // 连接名称
	subProto uint8  // 子协议类型
}

// SerLimitCreate create
// 创建受限服务端连接, 适用于嵌入式服务端
// 特点: 内存紧张, 只发送媒体数据, 不接收媒体数据
func SerLimitCreate(netConn net.Conn, r *chan ConnDataR) ConnInterface {
	var m serLimit
	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.subProto = MNPsubMedia

	m.done = make(chan bool)
	m.wList = make(chan ConnDataW, 30*6)
	m.rList = make(chan ConnDataR, 30*6)

	m.rOut = r
	m.netConn = netConn

	//fmt.Println("kmnp.new ser limit conn:", netConn.RemoteAddr())
	return &m
}

///////////////////////////////////////////////////////////////////////////////
func (m *serLimit) Destroy() {
	//addr := m.netConn.RemoteAddr()

	// 通知读写线程关闭
	m.cancel()
	m.wg.Wait()

	// 关闭连接
	m.netConn.Close()

	close(m.done)
	close(m.wList)
	close(m.rList)

	// 置空
	m.done = nil
	m.wList = nil
	m.rList = nil
	m.rOut = nil
	m.netConn = nil

	//fmt.Println("kmnp.close ser limit conn:", addr)
}

func (m *serLimit) Start() {
	m.wg.Add(1)
	go m.workerWrite()

	m.wg.Add(1)
	go m.workerRead()
}

func (m *serLimit) Stop() {
	m.cancel()
	m.wg.Wait()
}

func (m *serLimit) SetProtocol(protocol string) {
	m.protocol = protocol
}

func (m *serLimit) GetProtocol() string {
	return m.protocol
}

func (m *serLimit) SetName(name string) {
	m.name = name
}

func (m *serLimit) GetName() string {
	return m.name
}

func (m *serLimit) SendTxt(sequence, uid uint32, extra, msg []byte) bool {
	var data ConnDataW
	data.Data = packTxt(m.subProto, sequence, uid, extra, msg)
	m.wList <- data

	return true
}

func (m *serLimit) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return false
}

func (m *serLimit) SendMediaKmBuf(buf *kmpool.KmBuf) bool {
	buf.RefAll()

	var data ConnDataW
	data.Buf = buf
	m.wList <- data

	return true
}

func (m *serLimit) Recv() (uint8, uint32, uint32, []byte, []byte, error) {
	r := <-m.rList
	return r.Packtype, r.Sequence, r.UID, r.Extra, r.Data, r.Err
}

func (m *serLimit) RecvMediaKmBuf() (*kmpool.KmBuf, error) {
	return nil, errors.New("kmnp.ser_limit.RecvMedia,not support")
}

func (m *serLimit) RecvMediaByte() ([]byte, error) {
	return nil, errors.New("kmnp.ser_limit.RecvMediaFull,not support")
}

///////////////////////////////////////////////////////////////////////////////

func (m *serLimit) readHead(hb *kutil.Buf) (*MnpT, error) {
	n, err := m.netConn.Read(hb.LeftBuf())

	if nil == err {
		err = hb.AddPos(n)

		if nil == err {
			if hb.IsFull() {
				var mnpt MnpT
				mnpt.Unpack(hb.Data())

				if !mnpt.IsOk() {
					err = errors.New("kmnp.ser_limit.readHead,error!MnpT")
				} else {
					return &mnpt, nil
				}
			}
		}
	}

	return nil, err
}

func (m *serLimit) readTxt(txt *kutil.Buf) (bool, error) {
	n, err := m.netConn.Read(txt.LeftBuf())

	if nil == err {
		err = txt.AddPos(n)

		if nil == err {
			if txt.IsFull() {
				return true, nil // 这个分包读完了
			}
		}
	}

	return false, err
}

func (m *serLimit) readData(r *ConnDataR) {
	if nil != m.rOut {
		*m.rOut <- *r
	} else {
		m.rList <- *r
	}
}

func (m *serLimit) writeHeart() {
	var mnpt MnpT
	mnpt.InitHeart()

	var data ConnDataW
	data.Data = mnpt.Pack(nil)

	m.wList <- data
}

func (m *serLimit) workerWrite() error {
	defer m.wg.Done()
	var err error

	for {
		select {
		case data := <-m.wList:
			if nil != err {
				// 出错后, 仍需要将内存池的数据释放掉, 否则会出现内存池节点未被释放
				if nil != data.Buf {
					data.Buf.UnrefAll()
				}

				continue // 出现错误, 则不再向网络发送数据
			}

			if nil != data.Data {
				b := data.Data
				n := 0
				pos := 0

				for {
					n, err = m.netConn.Write(b[pos:])

					if nil != err {
						break
					}

					pos += n
					if len(b) <= pos {
						break
					}
				}
			}

			cur := data.Buf
			for nil != cur {
				next := cur.Next
				n := 0
				pos := 0

				b := cur.Data()
				for {
					n, err = m.netConn.Write(b[pos:])

					if nil != err {
						break
					}

					pos += n
					if len(b) <= pos {
						break
					}
				}

				if nil != err {
					cur.UnrefAll()
					break
				} else {
					cur.Unref()
				}

				cur = next
			}

			if nil != err {
				var r ConnDataR
				r.Protocol = m.protocol
				r.Name = m.name
				r.Err = err

				m.readData(&r)
			}
		case <-m.ctx.Done():
			// 此线程退出时, 将写列表托管起来
			// 将其中的数据都消费掉, 否则会出现内存池节点未被释放
			go func(wList chan ConnDataW) {
				for d := range wList {
					if nil != d.Buf {
						d.Buf.UnrefAll()
					}
				}
			}(m.wList)

			return m.ctx.Err() // 超时或强制退出
		}
	}
}

func (m *serLimit) workerRead() error {
	defer m.wg.Done()

	var err error  // 是否出错
	var mnpt *MnpT // 当前分包头

	const statusNil = -1 // 初始无效状态
	status := statusNil  // 接收状态

	hb := kutil.BufCreate(MNPTsize) // 接收分包头缓存
	defer hb.Destroy()

	var txt *kutil.Buf // 单次接收的数据包
	var txts [][]byte  // 累计接收的文本包

	for {
		select {
		default:
			if nil != err {
				continue //
			}

			switch {
			case statusNil == status:
				mnpt, err = m.readHead(hb)
				if nil != mnpt {
					status = int(mnpt.Packtype)
				}
			case int(MNPheart) == status:
				m.writeHeart() // 回复心跳

				hb.Reset()
				mnpt = nil
				status = statusNil
			case int(MNPtxt) == status:
				// 每一个包单独接收放入txt中
				// 单个包接收完毕后放入txts缓存, 最终将txts中的数据合并
				if nil == txt {
					txt = kutil.BufCreate(int(mnpt.Size - MNPTsize))
				}

				end := false
				end, err = m.readTxt(txt)
				if end {
					txts = append(txts, txt.Data())

					if MNPfull == mnpt.Opt || MNPend == mnpt.Opt {
						b := bytes.Join(txts, nil)

						var mnpTxtH MnpTxtH
						mnpTxtH.Unpack(b[0:MNPTxtHsize])

						var r ConnDataR
						r.Protocol = m.protocol
						r.Name = m.name
						r.Packtype = MNPtxt
						r.Sequence = mnpTxtH.Sequence
						r.UID = mnpTxtH.UID

						r.Extra = b[MNPTxtHsize : MNPTxtHsize+mnpTxtH.Extra]
						r.Data = b[MNPTxtHsize+mnpTxtH.Extra:]

						m.readData(&r)

						txts = nil
					}

					hb.Reset()
					mnpt = nil
					txt = nil
					status = statusNil
				}
			default:
				fmt.Println("kmnp.ser_limit.workerRead,status error")
				err = errors.New("kmnp.ser_limit.workerRead,status error")
			}

			if nil != err {
				var r ConnDataR
				r.Protocol = m.protocol
				r.Name = m.name
				r.Err = err

				m.readData(&r)
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
