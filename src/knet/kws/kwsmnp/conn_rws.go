package kwsmnp

import (
	"bytes"
	"errors"
	"fmt"

	"github.com/lishaoliang/klb/src/knet/kws"
	"github.com/lishaoliang/klb/src/kutil"
)

//func maskBytes(key [4]byte, pos int, b []byte) int {
//	for i := range b {
//		b[i] ^= key[pos&3]
//		pos++
//	}
//	return pos & 3
//}

func maskBytes(key []byte, pos int, b []byte) int {
	for i := range b {
		b[i] ^= key[pos&3]
		pos++
	}
	return pos & 3
}

func (m *Conn) doReadWebsocket() error {

	if wsStatusHeader == m.wsStatus {
		if nil == m.wsBuf {
			m.wsBuf = kutil.NewBuf(kws.WsHeadSizeMax)
		}

		buf := m.wsBuf

		spare := buf.Spare()
		if len(spare) <= 0 {
			return errors.New("kwsmnpconn.conn_rws.doReadWebsocket,out of memory,read header")
		}

		n, err := m.conn.Read(spare)
		if nil == err {
			buf.Add(n)

			// ws 头部最大长度14, 最小2字节
			// 缓存中可能接收了多个帧
			for {
				d := buf.Data()

				var ws kws.Ws
				if nil == ws.Unpack(d) {
					// 解析ws头成功
					fullLen := ws.HeadLen + int(ws.PayloadLen)

					if fullLen <= len(d) {
						// 完整的帧数据, 继续解析
						m.onReadFrame(ws, d[ws.HeadLen:fullLen])

						buf.Memmove(fullLen)
					} else {
						// 数据不完整
						m.wsHeader = ws

						m.wsFrame = append(m.wsFrame, d[ws.HeadLen:])
						m.wsFrameSpare = fullLen - len(d)

						kutil.Assert(0 < m.wsFrameSpare)
						m.wsNextBuf = kutil.NewBuf(m.wsFrameSpare)

						// 转入接收body阶段
						m.wsBuf.Reset()
						m.wsStatus = wsStatusBody
						break
					}
				} else {
					break
				}
			}
		}

		return err
	}

	buf := m.wsNextBuf

	spare := buf.Spare()
	if len(spare) <= 0 {
		return errors.New("kwsmnpconn.conn_rws.doReadWebsocket,out of memory,read body")
	}

	n, err := m.conn.Read(spare)
	if nil == err {
		buf.Add(n)
		d := buf.Data()

		m.wsFrameSpare -= len(d)
		if m.wsFrameSpare <= 0 {
			kutil.Assert(0 == m.wsFrameSpare)

			// 数据帧完整了
			frameN := m.wsNextBuf.Drop()
			m.wsFrame = append(m.wsFrame, frameN)

			data := bytes.Join(m.wsFrame, nil)
			m.onReadFrame(m.wsHeader, data)

			m.wsNextBuf = nil
			m.wsFrame = nil
			m.wsFrameSpare = 0

			// 转入接收ws头阶段
			m.wsStatus = wsStatusHeader
		}
	}

	return nil
}

func (m *Conn) onReadFrame(ws kws.Ws, b []byte) {

	// TODO...  非Fin包

	if kws.WsOpcodeText == ws.Opcode || kws.WsOpcodeBinary == ws.Opcode {
		if 0x1 == (0x1&ws.Mask) && 0 < ws.PayloadLen {
			maskBytes(ws.MaskingKey, 0, b)
		}
	}

	switch ws.Opcode {
	case kws.WsOpcodeText:
		m.conn.PushTxt(0, 0, nil, b)
	case kws.WsOpcodeBinary:
		// TODO...
		//fmt.Println("kwsconn.onReadFrame Binary", b)
		m.conn.SendBin(0, 0, nil, b)
	case kws.WsOpcodeClose:
		m.conn.PushDisconnect(errors.New("kwsmnpconn.conn_rws.onReadFrame,recv opcode close"))
	case kws.WsOpcodePing:
		fmt.Println("kwsmnpconn.onReadFrame Ping")
		//m.conn.SendHeart()
	case kws.WsOpcodePong:
		fmt.Println("kwsmnpconn.onReadFrame Pong")
	}
}
