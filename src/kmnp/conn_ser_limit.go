///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
//  Created: 2019/08/25
//
/// @file    conn_ser_limit.go
/// @author  lishaoliang
///  \n https://github.com/lishaoliang/klb/blob/master/LICENSE
///  \n https://github.com/lishaoliang/klb
/// @brief	conn_ser_limit
///////////////////////////////////////////////////////////////////////////
package kmnp

import (
	"context"
	"errors"
	"fmt"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/kmpool"
)

// connSerLimitData
type cslDataWrite struct {
	b   []byte
	buf *kmpool.KmBuf
}

// CslDataRead ConnSerLimit Data Read
type CslDataRead struct {
	Name string
	Err  error

	Packtype uint8
	Sequence uint32
	UID      uint32

	Extra []byte
	Data  []byte
}

// ConnSerLimit server mnp conn limit
// 资源受限平台的服务端连接: 适用于嵌入式服务端
// 可发送: 心跳/文本/二进制/媒体包
// 可接收: 心跳/文本/二进制
// 丢弃: 媒体包
type ConnSerLimit struct {
	ctx    context.Context    // 同步ctx
	cancel context.CancelFunc // 同步cancel
	wg     sync.WaitGroup     // 同步wg
	done   chan bool          // 类完成

	netConn net.Conn          // 连接
	wList   chan cslDataWrite // 写列表
	rList   chan CslDataRead  // 读列表

	name     string
	rOut     *chan CslDataRead
	subProto uint8
}

// connSerLimitCreate create conn
func connSerLimitCreate(netConn net.Conn, r *chan CslDataRead) *ConnSerLimit {
	var csl ConnSerLimit
	csl.ctx, csl.cancel = context.WithCancel(context.Background())

	csl.subProto = MNPsubMedia

	csl.done = make(chan bool)
	csl.wList = make(chan cslDataWrite)
	csl.rList = make(chan CslDataRead)

	csl.rOut = r

	csl.netConn = netConn

	fmt.Println("new conn:", netConn.RemoteAddr())
	return &csl
}

///////////////////////////////////////////////////////////////////////////////

// Destroy destroy
func (csl *ConnSerLimit) Destroy() {
	addr := csl.netConn.RemoteAddr()
	//fmt.Println("close conn:1111", m.conn.RemoteAddr())

	// 通知读写线程关闭
	csl.cancel()
	csl.wg.Wait()

	//fmt.Println("close conn:22222", m.conn.RemoteAddr())

	// 关闭连接
	csl.netConn.Close()

	// 关闭管道
	close(csl.rList)
	close(csl.wList)
	close(csl.done)

	csl.done = nil
	csl.wList = nil
	csl.rList = nil
	csl.netConn = nil
	csl.rOut = nil

	fmt.Println("close conn:", addr)
}

// SetName set name
func (csl *ConnSerLimit) SetName(name string) {
	csl.name = name
}

func (csl *ConnSerLimit) readData(r *CslDataRead) {
	if nil != csl.rOut {
		*csl.rOut <- *r
	} else {
		csl.rList <- *r
	}
}

func (csl *ConnSerLimit) readHead(bh *mnpBuf) (*MnpT, error) {
	n, err := csl.netConn.Read(bh.LeftBuf())

	if nil == err {
		err = bh.AddPos(n)

		if nil == err {
			if bh.IsFull() {
				var mnpt MnpT
				mnpt.Unpack(bh.Data())
				//fmt.Println("mnpt", mnpt)

				if !mnpt.IsOk() {
					err = errors.New("kmnp.ConnSerLimit.readHead,error!MnpT")
				} else {
					return &mnpt, nil
				}
			}
		}
	}

	return nil, err
}

func (csl *ConnSerLimit) readHeart(bh *mnpBuf) error {
	return nil
}

func (csl *ConnSerLimit) readTxt(mnpt *MnpT, br *mnpBuf) (*CslDataRead, error) {
	var n int
	var err error

	left := int(mnpt.Size) - MNPTsize - br.pos
	if 0 < left {
		n, err = csl.netConn.Read(br.LeftBufBy(left))

		if nil == err {
			br.AddPos(n)
		}
	} else {
		b := br.Data()
		var texth MnpTxtH
		texth.Unpack(b[:MNPTxtHsize])

		var data CslDataRead
		data.Name = csl.name
		data.Packtype = MNPtxt
		data.Sequence = texth.Sequence
		data.UID = texth.UID

		data.Extra = []byte(b[MNPTxtHsize : MNPTxtHsize+texth.Extra])
		data.Data = []byte(b[MNPTxtHsize+texth.Extra:])

		return &data, nil
	}

	return nil, err
}

func (csl *ConnSerLimit) readBin(mnpt *MnpT, bh *mnpBuf) error {
	return nil
}

func (csl *ConnSerLimit) readMedia(mnpt *MnpT, bh *mnpBuf) error {
	return nil
}

// workerRead go worker read
func (csl *ConnSerLimit) workerRead() error {
	defer csl.wg.Done()

	var mnpt *MnpT
	status := uint8(0xFF)

	bufH := mnpBufCreate(MNPTsize)
	bufR := mnpBufCreate(1024 * 64)

	var err error

	for {
		select {
		default:
			if nil != err {
				continue //
			}

			var r *CslDataRead

			switch {
			case uint8(0xFF) == status:
				mnpt, err = csl.readHead(bufH)
				if nil != mnpt {
					status = mnpt.Packtype
				}
			case MNPheart == status:
			case MNPtxt == status:
				r, err = csl.readTxt(mnpt, bufR)

				if nil != r {
					//csl.SendTxt(r.Sequence, r.UID, r.Extra, r.Data)
					//fmt.Println("read", string(r.Extra), string(r.Data))
					csl.readData(r)

					bufH.Reset()
					bufR.Reset()
					mnpt = nil
					status = uint8(0xFF)
				}
			case MNPbin == status:
			case MNPmedia == status:
			default:
			}

			if nil != err {
				var data CslDataRead
				data.Name = csl.name
				data.Err = err

				csl.readData(&data)

				//csl.done <- true
				//return csl.ctx.Err() // 出现错误
			}
		case <-csl.ctx.Done():
			return csl.ctx.Err() // 超时或强制退出
		}
	}
}

// workerWrite go worker write
func (csl *ConnSerLimit) workerWrite() error {
	defer csl.wg.Done()

	var err error

	for {
		select {
		case data := <-csl.wList:
			if nil != err {
				continue
			}

			if nil != data.b {
				_, err = csl.netConn.Write(data.b)
				//fmt.Println("kmnp.conn_ser_limit.workerWrite", n, err)
			}

			if nil != err {
				var data CslDataRead
				data.Name = csl.name
				data.Err = err

				csl.readData(&data)

				//csl.done <- true
				//return csl.ctx.Err() // 出现错误
			}
		case <-csl.ctx.Done():
			return csl.ctx.Err() // 超时或强制退出
		}
	}
}

// Worker worker
func (csl *ConnSerLimit) Worker() {
	csl.wg.Add(1)
	go csl.workerWrite()

	csl.wg.Add(1)
	go csl.workerRead()
}

// WaitDone wait for done
func (csl *ConnSerLimit) WaitDone() bool {
	b := <-csl.done
	return b
}

// SendTxt send text
func (csl *ConnSerLimit) SendTxt(sequence, uid uint32, extra, msg []byte) bool {

	var data cslDataWrite
	data.b = packTxt(csl.subProto, sequence, uid, extra, msg)

	csl.wList <- data

	return true
}

// SendBin send bin
func (csl *ConnSerLimit) SendBin(sequence, uid uint32, extra, bin []byte) bool {
	return true
}

// SendMedia send media
func (csl *ConnSerLimit) SendMedia(buf *kmpool.KmBuf) bool {
	return true
}

// Recv recv
// return @1 uint8	数据包类型(packtype): 0,MNPtxt,MNPbin
//	@2 uint32	序列号(sequence)
//	@3 uint32	用户自定义id(uid)
//	@4 []byte	附加数据(extra)
//	@5 []byte	文本或二进制(msg/bin)
//  @6 error	错误信息
func (csl *ConnSerLimit) Recv() (uint8, uint32, uint32, []byte, []byte, error) {

	r := <-csl.rList
	return r.Packtype, r.Sequence, r.UID, r.Extra, r.Data, r.Err
	//return MNPtxt, 0, 0, nil, nil
}
