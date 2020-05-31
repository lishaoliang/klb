package knet

import (
	"bytes"
	"context"
	"errors"
	"net"
	"sync"

	"github.com/lishaoliang/klb/src/knet/klisten"
	"github.com/lishaoliang/klb/src/knet/kmnp"
	"github.com/lishaoliang/klb/src/kutil"
)

// NspCb nsp listen callback
// 监听回调函数
type NspCb func(ptr interface{}, conn net.Conn, protocol string, data []byte) int

// Nsp net server preprocess
// 网络连接预处理模块
// 识别连接类型
type Nsp struct {
	ctx    context.Context    // 上下文环境
	cancel context.CancelFunc // 取消
	wg     sync.WaitGroup     // 等待组

	callback NspCb
	cbPtr    interface{}
}

// CreateNsp CreateNsp
// 创建
func CreateNsp(cb NspCb, ptr interface{}) *Nsp {
	var m Nsp

	m.ctx, m.cancel = context.WithCancel(context.Background())

	m.callback = cb
	m.cbPtr = ptr

	return &m
}

// Destroy destroy
func (m *Nsp) Destroy() {

	// 通知所有正在预读的线程关闭
	// 并等待
	m.cancel()
	m.wg.Wait()

	m.callback = nil
	m.cbPtr = nil
}

// Listen Listen
func (m *Nsp) Listen(name, network, address string) error {
	return klisten.Listen(name, network, address, onListenConn, m)
}

// ListenTLS ListenTLS
func (m *Nsp) ListenTLS(name, network, address, certFile, keyFile string) error {
	return klisten.ListenTLS(name, network, address, certFile, keyFile, onListenConn, m)
}

// Close close listen
func (m *Nsp) Close(name string) error {
	return klisten.Close(name)
}

func onListenConn(ptr interface{}, conn net.Conn, tls bool) int {
	if m, ok := ptr.(*Nsp); ok {

		m.wg.Add(1)
		go m.worker(conn, tls)

		return 0
	}

	return 1
}

// nspCRLF "\r\n"
var nspCRLF = []byte{0x0D, 0x0A}

func (m *Nsp) isTxtProtocol(data []byte, tls bool) string {
	//
	n := bytes.Index(data, nspCRLF)
	if 0 <= n {
		first := data[0:n]

		if 0 <= bytes.LastIndex(first, []byte(HTTP)) {
			// 注册的HTTP路径协议转换
			regProtocol := PathIsProtocol(string(first))
			if 0 < len(regProtocol) {
				return regProtocol
			}

			// 标准 HTTP/HTTPS
			if tls {
				return HTTPS
			}
			return HTTP
		} else if 0 <= bytes.LastIndex(first, []byte(RTSP)) {
			if !tls {
				return RTSP
			}
		}

		return UNKOWN
	}

	return ""
}

func (m *Nsp) isProtocol(data []byte, tls bool) string {
	dlen := len(data)

	// 先检查二进制协议
	if kmnp.MnpHeadSize <= dlen {
		var mnp kmnp.Mnp
		mnp.Unpack(data)
		if mnp.IsOk() {
			if tls {
				return MNPS
			}
			return MNP
		}
	}

	return m.isTxtProtocol(data, tls)
}

func (m *Nsp) worker(conn net.Conn, tls bool) error {
	kutil.Assert(nil != m.callback)
	defer m.wg.Done()

	var lastErr error // 出现的最后一次错误

	buf := kutil.NewBuf(4096)

	bclose := true
	defer func() {
		if bclose {
			//fmt.Println("defer knet.nsp.worker conn.Close()", bclose, lastErr)
			conn.Close()
		}
	}()

	for {
		select {
		default:
			spare := buf.Spare()
			if len(spare) <= 0 {
				lastErr = errors.New("knet.nsp.worker error!out of memory")
				return lastErr
			}

			// todo. 进程要求关闭时, 如何快速退出?
			// 这里会被卡在 Read 一段时间
			n, err := conn.Read(spare)
			if nil != err {
				lastErr = err
				return err // 出现错误
			}

			if 0 < n {
				buf.Add(n)

				data := buf.Data()
				protocol := m.isProtocol(data, tls)

				if "" != protocol {
					if 0 == m.callback(m.cbPtr, conn, protocol, data) {
						bclose = false
					}
					return nil
				}
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
