///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2019, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
/// @file   ser_cmd.go
/// @author lishaoliang
/// @brief	ser cmd
///////////////////////////////////////////////////////////////////////////

package kamdc

import (
	"fmt"

	"github.com/lishaoliang/klb/src/klua"
	"github.com/lishaoliang/klb/src/kutil"
)

// close
// 关闭连接
func (m *serMan) close(name string, err error) {

	m.connMutex.Lock()
	defer m.connMutex.Unlock()

	m.streamMutex.Lock()
	defer m.streamMutex.Unlock()

	m.streamMap.Remove(name)
	v := m.connMap.Remove(name)

	sc := m.toSerConn(v)
	if nil != sc {
		go func(conn *serConn) {
			conn.Destroy() // 关闭连接需要耗时
		}(sc)
	}
}

// workerCmd
// worker cmd
func (m *serMan) workerCmd(name, entry, preload, loader string) error {
	defer m.wg.Done()
	defer klua.Close(name)

	ctx, err := klua.Open(name, entry, preload, loader, false)
	if nil != err {
		fmt.Println("kamdc.ser_cmd.workerCmd", err)
		kutil.Assert(nil != ctx)
	}

	for {
		select {
		case data := <-m.rList:
			if nil != data.Err {
				m.close(data.Name, data.Err)
				msgex := fmt.Sprintf("{\"protocol\":\"%s\",\"status\":\"done\",\"sequence\":0,\"uid\":0}", data.Protocol)
				ctx.Post([]byte(data.Name), []byte(msgex), nil, nil)
			} else {
				msgex := fmt.Sprintf("{\"protocol\":\"%s\",\"status\":\"doing\",\"sequence\":%d,\"uid\":%d}", data.Protocol, data.Sequence, data.UID)
				ctx.Post([]byte(data.Name), []byte(msgex), data.Extra, data.Data)
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
