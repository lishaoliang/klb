package kwsflv

import (
	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kflv"
	"github.com/lishaoliang/klb/src/knet/kws"
)

// writeData writeData
func (m *Conn) writeData(data *kconn.ConnWrite) error {
	switch data.Packtype {
	case kconn.DataHeart:
		// 服务端
		return m.writePing()
	case kconn.DataTxt:
		return m.writeTxt(data)
	case kconn.DataBin:
		return m.writeBin(data)
	case kconn.DataMedia:
		return m.writeMedia(data)
	default:
		// 其他不处理
	}

	return nil
}

func (m *Conn) writePong() error {
	var ws kws.Ws
	ws.Fin = 0x1
	ws.Opcode = kws.WsOpcodePong
	ws.Mask = 0x0
	ws.PayloadLen = 0

	b := ws.Pack(nil)

	//fmt.Println("kwsconn.writePong", b)
	if err := m.conn.Write(b); nil != err {
		return err
	}

	return nil
}

func (m *Conn) writePing() error {
	var ws kws.Ws
	ws.Fin = 0x1
	ws.Opcode = kws.WsOpcodePing
	ws.Mask = 0x0
	ws.PayloadLen = 0

	b := ws.Pack(nil)

	//fmt.Println("kwsconn.writePing", b)
	if err := m.conn.Write(b); nil != err {
		return err
	}

	return nil
}

// writeTxt write text
func (m *Conn) writeTxt(data *kconn.ConnWrite) error {
	if switchStatusHTTP == m.switchStatus || switchStatusSwitching == m.switchStatus {
		head := data.Extra
		body := data.Data

		// 第一次开始写时, 转换状态
		m.switchStatus = switchStatusWebsocket

		if err := m.conn.Write(head); nil != err {
			return err
		}

		if err := m.conn.Write(body); nil != err {
			return err
		}

		// 如果回复200, 则补发一个ws头 + flv头
		var flv kflv.Head
		flv.Video = 0x1
		//flv.Audio = 0x1

		bFlv := make([]byte, kflv.FlvHeadSize+4)
		flv.Pack(bFlv)

		var ws kws.Ws
		ws.Fin = kws.WsFin
		ws.Opcode = kws.WsOpcodeBinary
		ws.PayloadLen = uint64(len(bFlv))
		ws.Mask = 0 // 服务端无需对数据做Mask, 否则Google Chrome浏览器报错

		bWs := ws.Pack(nil)
		if err := m.conn.Write(bWs); nil != err {
			return err
		}

		if err := m.conn.Write(bFlv); nil != err {
			return err
		}

	} else {
		//head := data.Extra
		body := data.Data

		var ws kws.Ws
		ws.Fin = kws.WsFin
		ws.Opcode = kws.WsOpcodeText
		ws.PayloadLen = uint64(len(body))
		ws.Mask = 0 // 服务端无需对数据做Mask, 否则Google Chrome浏览器报错

		b := ws.Pack(nil)

		if err := m.conn.Write(b); nil != err {
			return err
		}

		if err := m.conn.Write(body); nil != err {
			return err
		}
	}

	return nil
}

// writeBin write bin
func (m *Conn) writeBin(data *kconn.ConnWrite) error {

	//head := data.Extra
	body := data.Data

	var ws kws.Ws
	ws.Fin = kws.WsFin
	ws.Opcode = kws.WsOpcodeBinary
	ws.Mask = kws.WsMask
	ws.PayloadLen = uint64(len(body))
	ws.MaskingKey = getRandMask() //make([]byte, 4)

	//ws.MaskingKey[0] = 0x1
	//ws.MaskingKey[1] = 0x2
	//ws.MaskingKey[2] = 0x3
	//ws.MaskingKey[3] = 0x4

	//fmt.Println("kwsconn.conn.wtxt.writeTxt", b, body)
	maskBytes(ws.MaskingKey[:4], 0, body)

	b := ws.Pack(nil)

	if err := m.conn.Write(b); nil != err {
		return err
	}

	if err := m.conn.Write(body); nil != err {
		return err
	}

	return nil
}
