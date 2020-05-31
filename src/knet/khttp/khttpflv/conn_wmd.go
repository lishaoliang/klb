package khttpflv

import (
	"github.com/lishaoliang/klb/src/knet/kconn"
	"github.com/lishaoliang/klb/src/knet/kflv"
)

func (m *Conn) writeMedia(data *kconn.ConnWrite) error {

	cur := data.Buf
	for nil != cur {

		// 将数据打包成flv格式
		m.mdWrite.SetPos(0, 0)

		if err := kflv.PackByMnp(&m.parser, m.mdWrite, cur); nil != err {
			return err
		}

		if err := m.conn.Write(m.mdWrite.Data()); nil != err {
			return err
		}

		cur = cur.Next()
	}

	return nil
}
