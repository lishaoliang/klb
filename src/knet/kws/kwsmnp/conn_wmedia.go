package kwsmnp

import (
	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kws"
)

func (m *Conn) writeMedia(data *kconn.ConnWrite) error {
	cur := data.Buf
	for nil != cur {
		d := cur.Data()

		var ws kws.Ws
		ws.Fin = kws.WsFin
		ws.Opcode = kws.WsOpcodeBinary
		ws.PayloadLen = uint64(len(d))
		ws.Mask = 0 // 服务端无需对数据做Mask, 否则Google Chrome浏览器报错

		b := ws.Pack(nil)
		if err := m.conn.Write(b); nil != err {
			return err
		}

		if err := m.conn.Write(d); nil != err {
			return err
		}

		cur = cur.Next()
	}

	return nil
}
