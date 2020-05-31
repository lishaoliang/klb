package kwsflv

import (
	"bytes"
	"errors"

	"github.com/lishaoliang/klb/src/knet/khttp"
	"github.com/lishaoliang/klb/src/kutil"
)

// doReadHTTP doReadHTTP
// 读取http状态
func (m *Conn) doReadHTTP() error {

	if httpStatusHeader == m.httpStatus {
		if nil == m.httpBuf {
			m.httpBuf = kutil.NewBuf(khttp.HeaderMax)
		}

		buf := m.httpBuf

		spare := buf.Spare()
		if len(spare) <= 0 {
			return errors.New("kwsflv.conn_rhttp.doReadHTTP,out of memory,read header")
		}

		n, err := m.conn.Read(spare)
		if nil == err {
			buf.Add(n)

			for {
				d := buf.Data()

				ok, head, body, _ := khttp.Parser(d)
				if ok {
					fullLen := head + body
					if fullLen <= len(d) {
						// 完整的帧数据, 继续解析
						m.conn.PushTxt(0, 0, d[:head], d[head:head+body])
						buf.Memmove(head + body)

						if len(buf.Data()) <= 0 {
							// HTTP数据完毕了
							m.switchStatus = switchStatusSwitching
						}
					} else {
						// 只解析到了头
						frame0 := m.httpBuf.Drop()
						m.httpFrameSpare = fullLen - len(frame0) // 剩余需要读取数据长度

						m.httpHeadLen = head
						m.httpBodyLen = body

						m.httpFrame = append(m.httpFrame, frame0)

						// 简单处理, 直接申请需要剩余内存长度
						m.httpBuf = kutil.NewBuf(m.httpFrameSpare)
						kutil.Assert(0 < m.httpFrameSpare)

						m.httpStatus = httpStatusBody
						break
					}
				} else {
					break // 未解析到http头, 继续读取
				}
			}
		}

		return err
	}

	// httpStatusBody 状态
	// 接收Body 阶段
	buf := m.httpBuf
	kutil.Assert(nil != buf)

	spare := buf.Spare()
	if len(spare) <= 0 {
		return errors.New("kwsflv.conn_rhttp.doReadHTTP,out of memory,read body")
	}

	n, err := m.conn.Read(spare)
	if nil == err {
		buf.Add(n)
		d := buf.Data()

		m.httpFrameSpare -= len(d)
		if m.httpFrameSpare <= 0 {
			// 数据帧完整了
			frameN := buf.Drop()
			m.httpFrame = append(m.httpFrame, frameN)

			data := bytes.Join(m.httpFrame, nil)
			m.conn.PushTxt(0, 0, data[:m.httpHeadLen], data[m.httpHeadLen:m.httpHeadLen+m.httpBodyLen])

			m.httpFrame = nil
			m.httpFrameSpare = 0
			m.httpHeadLen = 0
			m.httpBodyLen = 0

			// 接收完整的帧之后, 转入切换协议阶段
			m.httpStatus = httpStatusHeader
			m.switchStatus = switchStatusSwitching
		}
	}

	return err
}
