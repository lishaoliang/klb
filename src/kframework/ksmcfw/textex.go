package ksmcfw

import (
	"errors"

	"github.com/lishaoliang/klb/src/knet/kconn"
)

// workerTextEx workerTextEx
func (m *smcfw) workerTextEx() error {
	defer m.wg.Done()

	errCom := errors.New("ksmcfw.textex.workerTextEx,common error")

	for {
		select {
		case txt := <-m.ncm.Recv():
			err := txt.Err
			if nil == err {
				err = errCom
			}

			switch txt.Packtype {
			case kconn.DataTxt:
				m.env.CallReceiver(txt.Name, txt.Protocol, txt.Sequence, txt.UID, txt.Extra, txt.Data)
			case kconn.Disconnect:
				m.env.CallDisconnect(txt.Name, txt.Protocol, err.Error())
				m.ncm.Close(txt.Name)
			case kconn.NewConnectError:
				m.env.CallNewConn(txt.Name, txt.Protocol, false, err.Error())
			case kconn.NewConnect:
				m.env.CallNewConn(txt.Name, txt.Protocol, true, "")
			}

			if nil != txt.Buf {
				txt.Buf.RefNext()
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
