package ksmcfw

import (
	"github.com/lishaoliang/klb/src/kframework/klsmfw"
)

// workerMedia workerMedia
func (m *smcfw) workerMedia() error {
	defer m.wg.Done()

	for {
		select {
		case md := <-m.ncm.RecvMedia():
			if nil != md.Buf {
				//d := md.Buf.Data()
				//var mnp kmnp.Mnp
				//mnp.Unpack(d)
				//fmt.Println("ksmcfw.media.workerMedia", mnp.Packtype, mnp.Opt, mnp.Size)

				m.bindMutex.Lock()
				push, chnn, sidx := m.bind.Modify(md.Name, md.Buf) // 修正数据, 并且得到绑定到的通道和流序号
				m.bindMutex.Unlock()

				if push {
					klsmfw.PushMedia(chnn, sidx, md.Buf)
				}

				md.Buf.UnrefNext()
			}
		case <-m.ctx.Done():
			return m.ctx.Err() // 超时或强制退出
		}
	}
}
